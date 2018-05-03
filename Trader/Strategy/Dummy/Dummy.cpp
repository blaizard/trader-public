#include "IrStd/IrStd.hpp"
#include "Trader/Strategy/Dummy/Dummy.hpp"

IRSTD_TOPIC_REGISTER(Trader, Strategy, Dummy);
IRSTD_TOPIC_USE_ALIAS(TraderDummy, Trader, Strategy, Dummy);

#define SELL_PROFIT_PERCENT 1.
#define BUY_MIN_SPREAD_PERCENT 0.1

Trader::Dummy::Dummy(const IrStd::Type::Gson::Map& config)
		: Strategy(Id("Dummy"), ConfigurationStrategy({
			{"trigger", IrStd::Type::toIntegral(ConfigurationStrategy::Trigger::ON_RATE_CHANGE)}
		}, config))
{
#ifdef DEBUG_DUMMY
	IrStd::Logger::getDefault().addTopic(TraderDummy);		
#endif
}

void Trader::Dummy::initializeImpl()
{
}

void Trader::Dummy::processImpl(const size_t /*counter*/)
{
	// Loop through the available currency
	getExchange().getCurrencies([&](const CurrencyPtr currency) {

		// Loop through all transactions
		getExchange().getTransactionMap().getTransactions(currency, [&](const CurrencyPtr,
				const PairTransactionMap::PairTransactionPointer pTransaction)
		{
			const auto amount = getExchange().getAmountToInvest(pTransaction.get());
			Order order(pTransaction, pTransaction->getRate());

			if (order.isFirstValid(amount))
			{
				size_t nbData = 0;
				IrStd::Type::Decimal maxRate = IrStd::Type::Decimal::min();
				IrStd::Type::Decimal minRate = IrStd::Type::Decimal::max();
				// Check if the current rate is the highest of the last x samples
				const bool isComplete = pTransaction->getRates(
					IrStd::Type::Timestamp::now(), IrStd::Type::Timestamp::now() - 60 * 1000,
					[&](const IrStd::Type::Timestamp /*timestamp*/, const IrStd::Type::Decimal rate) {
						nbData++;
						maxRate = (rate > maxRate) ? rate : maxRate;
						minRate = (rate < minRate) ? rate : minRate;
					});

				const auto spreadPrecent = (maxRate - minRate) / maxRate * 100;

				// If an opportunity is detected...
				if (!isComplete || spreadPrecent < BUY_MIN_SPREAD_PERCENT || pTransaction->getRate() != maxRate)
				{
					return;
				}

				IRSTD_LOG_INFO(TraderDummy, "Identified opportunity " << pTransaction->getInitialCurrency()
						<< "/" << pTransaction->getFinalCurrency() << " isComplete=" << isComplete << ", nbData="
						<< nbData << ", maxRate=" << maxRate << ", minRate=" << minRate << ", rate="
						<< pTransaction->getRate() << ", spreadPrecent=" << spreadPrecent);

				// Get the inverse transaction
				auto pInverseTransaction = getExchange().getTransactionMap().getTransactionForWrite(order.getFinalCurrency(), order.getInitialCurrency());
				if (pInverseTransaction)
				{
					// Sell with 1% profit (exculding fee)
					Order inverseOrder(pInverseTransaction, (1. / order.getRate()) * (1. + SELL_PROFIT_PERCENT / 100.));
					order.addNext(inverseOrder);

					// Create the sell operation
					sell(getExchange(), order, amount);
				}
			}
		});
	});
}
