#include "Trader/ExchangeImpl/Test/ExchangeTest.hpp"

IRSTD_TOPIC_REGISTER(Trader, ExchangeTest);
IRSTD_TOPIC_USE_ALIAS(TraderExchangeTest, Trader, ExchangeTest);

// ---- Trader::ExchangeTest --------------------------------------------------

constexpr double Trader::ExchangeTest::FEE_PERCENT;

Trader::ExchangeTest::ExchangeTest()
		: Exchange(Id("ExchangeTest"), ConfigurationExchange({
			{"ratesPollingPeriodMs", 1},
			{"orderPollingPeriodMs", 100},
			{"ratesRecording", false}}))
{
}

void Trader::ExchangeTest::updatePropertiesImpl(PairTransactionMap& transactionMap)
{
	typedef std::pair<CurrencyPtr, CurrencyPtr> CurrencyPair;

	for (auto& currencyPair : {
			CurrencyPair(Currency::USD, Currency::EUR),
			CurrencyPair(Currency::USD, Currency::BTC),
			CurrencyPair(Currency::EUR, Currency::BTC)
		})
	{
		const auto currency1 = currencyPair.first;
		const auto currency2 = currencyPair.second;

		PairTransactionImpl transaction(currency1, currency2);
		transaction.setFeePercent(0.26);

		transactionMap.registerPair<PairTransactionImpl>(transaction);
		transactionMap.registerInvertPair<InvertPairTransactionImpl>(transaction);
	}
}

void Trader::ExchangeTest::updateRatesImpl()
{
	const auto timestamp = IrStd::Type::Timestamp::now();
	auto rand = IrStd::Rand();

	{
		auto scope = m_lockRates.writeScope();
		getTransactionMap().getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2) {
			auto pTransaction = getTransactionMap().getTransactionForWrite(currency1, currency2);
			if (!pTransaction->isInvertedTransaction())
			{
				const IrStd::Type::Decimal prevRate = pTransaction->getRate();
				const IrStd::Type::Decimal refRate = (prevRate < 0.1) ? IrStd::Type::Decimal(1) : prevRate;
				const IrStd::Type::Decimal direction = (rand.getNumber<double>(-refRate, 1) > 0) ? 1 : -1;

				const auto bidPrice = refRate + direction * rand.getNumber<double>(0, 0.001);
				const auto askPrice = bidPrice - 0.01 + rand.getNumber<double>(-0.001, 0.001);

				pTransaction->setBidPrice(bidPrice, timestamp);
				pTransaction->setAskPrice(askPrice, timestamp);
			}
		});
	}
}

