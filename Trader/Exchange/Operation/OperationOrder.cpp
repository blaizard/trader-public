#include <map>
#include <mutex>

#include "Trader/Exchange/Operation/OperationOrder.hpp"
#include "Trader/Exchange/Operation/OperationContext.hpp"
#include "Trader/Strategy/Strategy.hpp"

IRSTD_TOPIC_USE_ALIAS(TraderOperation, Trader, Operation);

namespace
{
	class ContextMonitorProfit : public Trader::OperationContext
	{
	public:
		ContextMonitorProfit(
			const Trader::CurrencyPtr initialCurrency,
			const IrStd::Type::Decimal amount,
			const Trader::Strategy& strategy)
				: OperationContext(strategy)
				, m_initialCurrency(initialCurrency)
				, m_initialAmount(amount)
				, m_final{0, 0}
		{
			// Initial amount counts for 100%
			auto scope = m_lock.writeScope();
			m_converted[m_initialCurrency] = ConvertedInfo{m_initialAmount, 1.};
		}

		/**
		 * Calculate the profit and generate the string
		 */
		void postConvert() 
		{
			IRSTD_ASSERT(TraderOperation, m_lock.isWriteScope(), "This operation must be under a write scope");

			std::stringstream tempStream;
			IrStd::Type::Decimal profitAmount = 0.;
			{
				IrStd::Type::Decimal ratioSum = 0.;
				bool isStreamEmpty = true;

				// Handle the profit
				if (m_final.m_amount)
				{
					const auto initialAmountFullyProcessed = m_initialAmount * m_final.m_ratio;
					tempStream << initialAmountFullyProcessed
							<< " " << m_initialCurrency << " -> " << m_final.m_amount
							<< " " << m_initialCurrency;
					profitAmount = m_final.m_amount - initialAmountFullyProcessed;
					ratioSum += m_final.m_ratio;
					isStreamEmpty = false;
				}

				for (const auto elt : m_converted)
				{
					// Ignore null amounts and if the currency is identical
					// to the inital (this is handled before)
					if (elt.second.m_amount && elt.first != m_initialCurrency)
					{
						tempStream << ((isStreamEmpty) ? "" : ", ") << (m_initialAmount * elt.second.m_ratio)
								<< " " << m_initialCurrency << " -> " << elt.second.m_amount
								<< " " << elt.first;
						isStreamEmpty = false;
					}
					ratioSum += elt.second.m_ratio;
				}

				// Calculate the inital amount NOT processed
				const auto initialAmountNotProcessed = m_converted.at(m_initialCurrency).m_amount;
				if (initialAmountNotProcessed != 0)
				{
					tempStream << ((isStreamEmpty) ? "" : ", ") << "not processed: " << initialAmountNotProcessed
							<< " " << m_initialCurrency;
					isStreamEmpty = false;
				}

				const auto initialAmountProcessed = (m_initialAmount - initialAmountNotProcessed);
				const auto feeAmount = initialAmountProcessed * (1. - ratioSum);

				// Subtract the fees from the profit (money loss)
				profitAmount -= feeAmount;

				tempStream << ((isStreamEmpty) ? "" : ", ") << "profit/loss=" << profitAmount << " " << m_initialCurrency
						<< " (" << (profitAmount / initialAmountProcessed * 100.) << "%)"
						<< ", fee=" << (feeAmount / initialAmountProcessed * 100) << "% of "
						<< initialAmountProcessed << " " << m_initialCurrency;
			}

			// Assign the values
			m_description = tempStream.str();
			setProfit(m_initialCurrency, profitAmount);
		}

		void convert(const Trader::Order& order, const IrStd::Type::Decimal amount1) noexcept
		{
			const auto currency1 = order.getInitialCurrency();
			const auto currency2 = order.getFirstOrderFinalCurrency();
			const auto amount2 = order.getFirstOrderFinalAmount(amount1);

			// Fees for this transaction, as a ratio
			const auto amount2NoFee = order.getFirstOrderFinalAmount(amount1, /*includeFee*/false);
			const auto feeRatio = (amount2NoFee - amount2) / amount2NoFee;

			{
				auto scope = m_lock.writeScope();

				// Make sure the inital currency is logged, if not we have a problem
				const auto it = m_converted.find(currency1);
				IRSTD_ASSERT(TraderOperation, it != m_converted.end(), "The currency " << currency1
						<< " is not registered within this context monitor");
				const auto registeredRatio = it->second.m_ratio;
				const auto registeredAmount = it->second.m_amount;

				// Find a better way to deal with this
				if (registeredAmount < amount1)
				{
					// Display warnign message only if amount is signification enough
					if (order.isValid(amount1 - registeredAmount))
					{
						IRSTD_LOG_FATAL(TraderOperation, "Registered amount (" << registeredAmount
								<< ") is lower than the processed amount (" << amount1
								<< "), ignoring the difference (" << (amount1 - registeredAmount) << ")");
					}
					scope.release();
					convert(order, registeredAmount);
					return;
				}

				// If the new currency does not exists yet, create it
				if (m_converted.find(currency2) == m_converted.end())
				{
					m_converted[currency2] = ConvertedInfo{0, 0};
				}

				// Take some of this amount and update the records
				const auto proceedRatioOfRegistered = amount1 / registeredAmount;
				const auto proceedRatio = proceedRatioOfRegistered * registeredRatio;

				IRSTD_ASSERT(TraderOperation, proceedRatio <= registeredRatio, "amount1=" << amount1
						<< ", registeredAmount=" << registeredAmount
						<< ", proceedRatioOfRegistered=" << proceedRatioOfRegistered
						<< ", proceedRatio=" << proceedRatio << ", registeredRatio=" << registeredRatio);

				m_converted[currency1].m_ratio -= proceedRatio;
				m_converted[currency1].m_amount -= amount1;

				if (currency2 == m_initialCurrency)
				{
					m_final.m_ratio += proceedRatio * (1 - feeRatio);
					m_final.m_amount += amount2;
				}
				else
				{
					m_converted[currency2].m_ratio += proceedRatio * (1 - feeRatio);
					m_converted[currency2].m_amount += amount2;
				}

				// Calculate the profit
				postConvert();
			}
		}

	private:
		struct ConvertedInfo
		{
			IrStd::Type::Decimal m_amount; /// Amount converted
			IrStd::Type::Decimal m_ratio; /// Ratio of the amount converted
		};

		const Trader::CurrencyPtr m_initialCurrency;
		const IrStd::Type::Decimal m_initialAmount;
		ConvertedInfo m_final;
		std::map<Trader::CurrencyPtr, ConvertedInfo> m_profit;
		std::map<Trader::CurrencyPtr, ConvertedInfo> m_converted;
		mutable IrStd::RWLock m_lock;
	};
}

Trader::OperationOrder::OperationOrder(
	const Order& order,
	const IrStd::Type::Decimal amount,
	const Strategy& strategy)
		: Operation(order, amount, Context::create<::ContextMonitorProfit>(order.getInitialCurrency(), amount, strategy).cast<OperationContext>())
{
	onOrderComplete("monitorProfit", Trader::OperationOrder::monitorProfit, EventManager::Lifetime::OPERATION);
}

void Trader::OperationOrder::monitorProfit(
		ContextHandle& context,
		const TrackOrder& trackOrder,
		const IrStd::Type::Decimal amount)
{
	auto contextMonitorProfit = context.cast<::ContextMonitorProfit>();

	// Update the amount and ratio converted in the various currencies
	contextMonitorProfit->convert(trackOrder.getOrder(), amount);

	IRSTD_LOG_INFO(TraderOperation, "Proceed order with context " << context);
}
