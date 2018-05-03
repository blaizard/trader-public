#include <algorithm>
#include <string>

#include "Trader/ExchangeImpl/Bitstamp/ExchangeBitstamp.hpp"

IRSTD_TOPIC_REGISTER(Trader, Bitstamp);
IRSTD_TOPIC_USE_ALIAS(TraderBitstamp, Trader, Bitstamp);

// ---- Trader::ExchangeBitfinex --------------------------------------------------

Trader::ExchangeBitstamp::ExchangeBitstamp()
		: Exchange(Id("Bitstamp"), ConfigurationExchange({
			{"ratesPolling", IrStd::Type::toIntegral(ConfigurationExchange::RatesPolling::NONE)},
		}))
		, m_pusher("de504dc5763aeef9ff52")
		, m_availablePairs{
			CurrencyPair{Currency::EUR, Currency::USD, "eurusd", "live_trades_eurusd"},
			CurrencyPair{Currency::BTC, Currency::USD, "btcusd", "live_trades"},
			CurrencyPair{Currency::BTC, Currency::EUR, "btceur", "live_trades_btceur"},
			CurrencyPair{Currency::XRP, Currency::USD, "xrpusd", "live_trades_xrpusd"},
			CurrencyPair{Currency::XRP, Currency::EUR, "xrpeur", "live_trades_xrpeur"},
			CurrencyPair{Currency::XRP, Currency::BTC, "xrpbtc", "live_trades_xrpbtc"},
			CurrencyPair{Currency::LTC, Currency::USD, "ltcusd", "live_trades_ltcusd"},
			CurrencyPair{Currency::LTC, Currency::EUR, "ltceur", "live_trades_ltceur"},
			CurrencyPair{Currency::LTC, Currency::BTC, "ltcbtc", "live_trades_ltcbtc"},
			CurrencyPair{Currency::ETH, Currency::USD, "ethusd", "live_trades_ethusd"},
			CurrencyPair{Currency::ETH, Currency::EUR, "etheur", "live_trades_etheur"},
			CurrencyPair{Currency::ETH, Currency::BTC, "ethbtc", "live_trades_ethbtc"},
			CurrencyPair{Currency::BCH, Currency::USD, "bchusd", "live_trades_bchusd"},
			CurrencyPair{Currency::BCH, Currency::EUR, "bcheur", "live_trades_bcheur"},
			CurrencyPair{Currency::BCH, Currency::BTC, "bchbtc", "live_trades_bchbtc"}
		}
{
	for (const auto& currencyPair : m_availablePairs)
	{
		const auto initialCurrency = currencyPair.m_currency1;
		const auto finalCurrency = currencyPair.m_currency2;

		m_pusher.subscribe(currencyPair.m_webscoketChannel, [this, initialCurrency, finalCurrency](const std::string& event, const std::string& data) {
			websocketReceive(initialCurrency, finalCurrency, event, data);
		});
	}
}

void Trader::ExchangeBitstamp::updatePropertiesImpl(PairTransactionMap& transactionMap)
{
	for (const auto& currencyPair : m_availablePairs)
	{
		PairTransactionImpl transaction(currencyPair.m_currency1, currencyPair.m_currency2);
		transactionMap.registerPair<PairTransactionImpl>(transaction);
		transactionMap.registerInvertPair<InvertPairTransactionImpl>(transaction);
	}
}

void Trader::ExchangeBitstamp::websocketReceive(
		const CurrencyPtr initialCurrency,
		const CurrencyPtr finalCurrency,
		const std::string& event,
		const std::string& data) noexcept
{
	// Only consider trade event
	if (event != "trade")
	{
		return;
	}

	try
	{
		const IrStd::Json json(data.c_str());
		const auto rate = json.getNumber("price").val();
		const auto type = json.getNumber("type").val();
		if (type == 1)
		{
			getTransactionMap().getTransactionForWrite(initialCurrency, finalCurrency)->setRate(rate, IrStd::Type::Timestamp::now());
		}
		else
		{
			getTransactionMap().getTransactionForWrite(finalCurrency, initialCurrency)->setRate(1. / rate, IrStd::Type::Timestamp::now());
		}

		m_eventRates.trigger();
	}
	catch (const IrStd::Exception& e)
	{
		IRSTD_LOG_ERROR(TraderBitstamp, "Exception while parsing response: " << e.what());
	}
}

void Trader::ExchangeBitstamp::updateRatesStartImpl()
{
	m_pusher.connect();
}

void Trader::ExchangeBitstamp::updateRatesStopImpl()
{
	m_pusher.disconnect();
}
