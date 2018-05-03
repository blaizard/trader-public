#pragma once

#include "Trader/Exchange/Exchange.hpp"

IRSTD_TOPIC_USE(Trader, Exchange, Mock);

namespace Trader
{
	static constexpr size_t AVG_SERVER_LAG_MS = 1000;
	static constexpr size_t AVG_SERVER_LAG_REGISTER_ORDER_MS = 1000;
	static constexpr size_t INTIAL_BALANCE_VALUE = 2000;
	static constexpr size_t SERVER_FAILURE_PERCENT = 20;

	template<class T>
	class ExchangeMock : public T
	{
	public:
		template<class ... Args>
		ExchangeMock(Args&& ... args)
				: T(std::forward<Args>(args)...)
				, m_id(123456)
		{
			// Note: no need to terminate the thread, it is done automatically by disconnect
			this->createThread("Mock", &ExchangeMock::updateMockOrdersThread, this);
			// In this mode, the balance does include the reserve
			this->m_configuration.setBalanceIncludeReserve(true);
		}

		~ExchangeMock()
		{
			this->terminateThread("Mock");
		}

		void setOrderImpl(const Order& order, const IrStd::Type::Decimal amount, std::vector<Id>& idList) override final
		{
			sleepForAvgDelay(AVG_SERVER_LAG_MS);
			emulateRandomServerFailure(SERVER_FAILURE_PERCENT, /*throwRetry*/false);

			{
				std::lock_guard<std::mutex> lock(m_mockOrderLock);
				// Make sure the amount is significant enough
				IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, Exchange, Mock), order.isFirstValid(amount),
						"Order is invalid, amount=" << amount << ", order=" << order);

				// Make sure there is enough fund
				const auto availableFund = m_mockBalance.get(order.getInitialCurrency());
				IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, Exchange, Mock), availableFund >= amount,
						"Insufficient funds, availableFund=" << availableFund << ", amount=" << amount);

				// Make sure the rate is ok
				if (order.getTransaction()->getRate() < order.getRate())
				{
					const auto delayMs = IrStd::Rand().getNumber<size_t>(0, AVG_SERVER_LAG_REGISTER_ORDER_MS * 2);
					const auto orderId = Id(m_id++);
					IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Exchange, Mock), "Rate is too low for " << order << ", creating order #"
							<< orderId << " with " << delayMs << "ms delay");
					TrackOrder track(orderId, order, amount, IrStd::Type::Timestamp::now() + delayMs);
					m_mockTrackOrderListLag.push_back(std::move(track));

					idList.push_back(orderId);
					return;
				}

				// Process immediatly
				processOrder(order, amount);
			}
		}

		void updateOrdersImpl(std::vector<TrackOrder>& trackOrders) override final
		{
			sleepForAvgDelay(AVG_SERVER_LAG_MS);
			emulateRandomServerFailure(SERVER_FAILURE_PERCENT, /*throwRetry*/true);

			{
				std::lock_guard<std::mutex> lock(m_mockOrderLock);
				for (const auto& track : m_mockTrackOrderList)
				{
					TrackOrder trackOrder(track.getId(), track.getOrder(), track.getAmount(), track.getCreationTime());
					trackOrders.push_back(std::move(trackOrder));
				}
			}
		}

		void withdrawImpl(const CurrencyPtr currency, const IrStd::Type::Decimal amount) override final
		{
			sleepForAvgDelay(AVG_SERVER_LAG_MS);
			emulateRandomServerFailure(SERVER_FAILURE_PERCENT, /*throwRetry*/true);

			{
				std::lock_guard<std::mutex> lock(m_mockOrderLock);
				const auto amountAvailable = m_mockBalance.get(currency);
				IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, Exchange, Mock), amount <= amountAvailable,
						"Amount request to withdraw (" << amount << " " << currency
						<< "), is higher than the amount available (" << amountAvailable
						<< " " << currency << ")");
				m_mockBalance.add(currency, -amount);
				IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Exchange, Mock), "Withdrawing "
						<< amount << " " << currency << " from " << this->getId());
			}
		}

		void cancelOrderImpl(const TrackOrder& order) override final
		{
			sleepForAvgDelay(AVG_SERVER_LAG_MS);
			emulateRandomServerFailure(SERVER_FAILURE_PERCENT, /*throwRetry*/false);

			{
				std::lock_guard<std::mutex> lock(m_mockOrderLock);
				for (auto it = m_mockTrackOrderList.begin(); it != m_mockTrackOrderList.end(); it++)
				{
					if (it->getId() == order.getId())
					{
						m_mockTrackOrderList.erase(it);
						return;
					}
				}
			}
		}

		void updateBalanceImpl(Balance& balance) override final
		{
			sleepForAvgDelay(AVG_SERVER_LAG_MS);
			emulateRandomServerFailure(SERVER_FAILURE_PERCENT, /*throwRetry*/true);

			{
				std::lock_guard<std::mutex> lock(m_mockOrderLock);
				// Initial balance
				if (m_mockBalance.empty())
				{
					// There must be at least some rates available
					this->m_eventRates.waitForAtLeast(1);
					const auto nbCurrencies = this->getNbCurrencies();
					const auto baseCurrency = this->getEstimateCurrency();
					const IrStd::Type::Decimal amountPerCurrency = IrStd::Type::Decimal(INTIAL_BALANCE_VALUE) / nbCurrencies;
					this->getCurrencies([&](const CurrencyPtr currency) {
						const auto pOrder = this->identifyOrderChain(baseCurrency, currency);
						const auto amount = pOrder->getFinalAmount(amountPerCurrency, /*includeFee*/false);
						m_mockBalance.set(currency, amount);
					});
					IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Exchange, Mock), "Mocking balance for " << this->getId());
				}
				balance.setFunds(m_mockBalance);
			}
		}

	private:
		void updateMockOrdersThread()
		{
			while (IrStd::Threads::isActive())
			{
				IrStd::Threads::setIdle();
				if (this->m_eventRates.waitForNext(/*timeoutMs*/100))
				{
					IrStd::Threads::setActive();
					std::lock_guard<std::mutex> lock(m_mockOrderLock);

					// Add orders if some are pending
					for (auto it = m_mockTrackOrderListLag.begin(); it != m_mockTrackOrderListLag.end();)
					{
						const auto curTimestamp = IrStd::Type::Timestamp::now();
						const auto timestamp = it->getCreationTime();
						if (curTimestamp > timestamp)
						{
							m_mockTrackOrderList.push_back(std::move(*it));
							it = m_mockTrackOrderListLag.erase(it);
						}
						else
						{
							it++;
						}
					}

					// Process the orders if they can be
					for (auto it = m_mockTrackOrderList.begin(); it != m_mockTrackOrderList.end();)
					{
						const auto& order = it->getOrder();
						const auto rate = order.getRate();
						const auto transactionRate = order.getTransaction()->getRate();
						if (rate <= transactionRate)
						{
							// Process the order
							processOrder(order, it->getAmount());
							it = m_mockTrackOrderList.erase(it);
						}
						else
						{
							it++;
						}
					}
				}
			}
		}

		void processOrder(const Order& order, const IrStd::Type::Decimal amount)
		{
			IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Exchange, Mock), "Processing order "
					<< order << " with amount " << amount << " and rate "
					<< order.getTransaction()->getRate());

			// Make sure there is enough funds in the balance
			{
				const auto balanceAmount = m_mockBalance.get(order.getInitialCurrency());
				if (balanceAmount < amount)
				{
					IRSTD_LOG_ERROR(IRSTD_TOPIC(Trader, Exchange, Mock), "Insufficient funds ("
							<< balanceAmount << " " << order.getInitialCurrency() << "), cancel");
					return;
				}
			}

			m_mockBalance.add(order.getInitialCurrency(), -amount);
			m_mockBalance.add(order.getFirstOrderFinalCurrency(), order.getFirstOrderFinalAmount(amount));
		}

		void sleepForAvgDelay(const size_t avgDelayMs) const noexcept
		{
			IrStd::Rand rand;
			const auto delayMs = rand.getNumber<size_t>(0, avgDelayMs * 2);
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
		}

		void emulateRandomServerFailure(const size_t ratePercent, const bool throwRetry) const
		{
			const auto serverFailure = (IrStd::Rand().getNumber<size_t>(0, 100) < ratePercent);
			if (serverFailure)
			{
				if (throwRetry)
				{
					IRSTD_THROW_RETRY(IRSTD_TOPIC(Trader, Exchange, Mock),
							"Server busy (failure.rate=" << ratePercent << "%)");
				}
				else
				{
					IRSTD_THROW(IRSTD_TOPIC(Trader, Exchange, Mock),
							"Server busy (failure.rate=" << ratePercent << "%)");
				}
			}
		}

		Balance m_mockBalance;
		size_t m_id;
		std::mutex m_mockOrderLock;
		std::vector<TrackOrder> m_mockTrackOrderList;
		std::vector<TrackOrder> m_mockTrackOrderListLag;
	};
}
