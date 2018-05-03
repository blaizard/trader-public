#include "IrStd/IrStd.hpp"
#include "Trader/Strategy/SwingTrading/SwingTrading.hpp"

IRSTD_TOPIC_REGISTER(Trader, Strategy, SwingTrading);
IRSTD_TOPIC_USE_ALIAS(TraderSwingTrading, Trader, Strategy, SwingTrading);

Trader::SwingTrading::SwingTrading(const IrStd::Type::Gson::Map& config)
		: Strategy(Id("SwingTrading"), ConfigurationStrategy({
			{"trigger", IrStd::Type::toIntegral(ConfigurationStrategy::Trigger::ON_RATE_CHANGE)},
			{"minProfitPercent", 1}
		}, config))
{
#ifdef DEBUG_SWING_TRADING
	IrStd::Logger::getDefault().addTopic(TraderSwingTrading);		
#endif
}

void Trader::SwingTrading::initializeImpl()
{
}

void Trader::SwingTrading::processImpl(const size_t /*counter*/)
{
	eachExchanges([this](const Id id) {
		auto& exchange = getExchange(id);

		// Only operates on EUR/BTC pair
		const auto pTransaction = exchange.getTransactionMap().getTransaction(Currency::EUR, Currency::BTC);

		if (pTransaction && pTransaction->getNbRates() >= 7 && exchange.getFund(Currency::EUR) > 20)
		{
			const auto curRate = pTransaction->getRate();

			// Look for the last 5 samples
			const auto avgCurrent = (curRate
					+ pTransaction->getRate(-1)
					+ pTransaction->getRate(-2)
					+ pTransaction->getRate(-3)
					+ pTransaction->getRate(-4)) / 5.;
			const auto avgMinus1 = (pTransaction->getRate(-1)
					+ pTransaction->getRate(-2)
					+ pTransaction->getRate(-3)
					+ pTransaction->getRate(-4)
					+ pTransaction->getRate(-5)) / 5.;
			const auto avgMinus2 = (pTransaction->getRate(-2)
					+ pTransaction->getRate(-3)
					+ pTransaction->getRate(-4)
					+ pTransaction->getRate(-5)
					+ pTransaction->getRate(-6)) / 5.;

			// Detect a low swing value
			if (avgMinus2 < avgMinus1 && avgMinus1 > avgCurrent)
			{
				IRSTD_LOG_INFO(TraderSwingTrading, "Detected a swing low: avg(-2)=" << avgMinus2
						<< ", avg(-1)=" << avgMinus1 << ", avg(now)=" << avgCurrent);

				// Create the order
				Order order(pTransaction, curRate);
				{
					order.setTimeout(300);
					const auto pTransactionInverse = exchange.getTransactionMap().getTransaction(Currency::BTC, Currency::EUR);
					Order orderInverse(pTransactionInverse, (1. / curRate) * 1.01);
					orderInverse.setTimeout(99999999999);
					order.addNext(orderInverse);
				}

				// Create the sell operation
				const auto amount = exchange.getFund(Currency::EUR);
				sell(exchange, order, amount);
			}
		}
	});
}
