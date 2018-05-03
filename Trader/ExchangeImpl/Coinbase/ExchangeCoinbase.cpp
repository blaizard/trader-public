#include "Trader/ExchangeImpl/Coinbase/ExchangeCoinbase.hpp"

IRSTD_TOPIC_REGISTER(Trader, Coinbase);
IRSTD_TOPIC_USE_ALIAS(TraderCoinbase, Trader, Coinbase);

// ---- Trader::ExchangeCoinbase --------------------------------------------------

Trader::ExchangeCoinbase::ExchangeCoinbase()
		: Exchange(Id("Coinbase"), ConfigurationExchange({
			{"ratesPollingPeriodMs", 5000},
			{"ratesPolling", IrStd::Type::toIntegral(ConfigurationExchange::RatesPolling::UPDATE_RATES_SPECIFIC_CURRENCY_IMPL)}
		}))
{
}

void Trader::ExchangeCoinbase::updatePropertiesImpl(PairTransactionMap& transactionMap)
{
	auto scope = m_lockProperties.writeScope();

	// Clear the currency list
	m_stringToCurrency.clear();
	m_currencyToString.clear();
	std::map<CurrencyPtr, IrStd::Type::Decimal> currencyProperties;

	try
	{
		// List all availabel currencies
		std::string data;
		IrStd::FetchUrl fetch("https://api.coinbase.com/v2/currencies", data);
		fetch.processSync();

		// Parse a data json
		IrStd::Json json(data.c_str());
		auto& array = json.getArray("data");
		for (size_t i=0; i<array.size(); i++)
		{
			auto& obj = array.getObject(i);
			const auto currencyStr = obj.getString("id").val();
			const auto currency = Currency::discover(currencyStr);

			if (currency)
			{
				IRSTD_ASSERT(TraderCoinbase, m_stringToCurrency.find(currencyStr) == m_stringToCurrency.end(),
						"Currency '" << currencyStr << "' as already been registered");
				IRSTD_ASSERT(TraderCoinbase, m_currencyToString.find(currency) == m_currencyToString.end(),
						"Currency '" << currency << "' as already been registered");
				{
					m_stringToCurrency[currencyStr] = currency;
					m_currencyToString[currency] = currencyStr;
				}
				// Save the minimum size of the transaction
				const auto minSize = obj.getString("min_size").val();
				currencyProperties[currency] = IrStd::Type::Decimal::fromString(minSize);
			}
			else
			{
				IRSTD_LOG_WARNING(TraderCoinbase, "Unknown currency '" << currencyStr
						<< "' (" << obj.getString("name").val() << "), ignoring");
			}
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}

	// Define the pair associated with the currencies
	for (const auto& currencyFrom : m_stringToCurrency)
	{
		std::string data;

		// Build the url
		std::stringstream urlStream;
		urlStream << "https://api.coinbase.com/v2/exchange-rates?currency=";
		urlStream << currencyFrom.first;

		// List all available transactions for this pair
		IRSTD_HANDLE_RETRY({
			IrStd::FetchUrl fetch(urlStream.str().c_str(), data);
			fetch.processSync();
		}, 3);

		try
		{
			IrStd::Json json(data.c_str());
			for (const auto& pair : json.getObject("data", "rates"))
			{
				const auto it = m_stringToCurrency.find(pair.first);

				if (it != m_stringToCurrency.end() && currencyFrom.second != it->second)
				{
					const auto currency1 = currencyFrom.second;
					const auto currency2 = it->second;

					PairTransactionImpl transaction(currency1, currency2);
					{
						Boundaries boundaries;
						boundaries.setInitialAmount(currencyProperties[currency1]);
						boundaries.setFinalAmount(currencyProperties[currency2]);
						transaction.setBoundaries(boundaries);
					}

					transactionMap.registerPair<PairTransactionImpl>(transaction);
				}
			}
		}
		catch (...)
		{
			IrStd::Exception::rethrowRetry();
		}
	}
}

void Trader::ExchangeCoinbase::updateRatesSpecificCurrencyImpl(const CurrencyPtr currency)
{
	auto scope = m_lockProperties.readScope();

	// Identify the transaction and build the url address
	std::stringstream urlStream;
	{
		const auto it = m_currencyToString.find(currency);
		IRSTD_THROW_ASSERT(TraderCoinbase, it != m_currencyToString.end(), "This currency (" << currency
				<< ") is not supported");

		urlStream << "https://api.coinbase.com/v2/exchange-rates?currency=";
		urlStream << it->second;
	}

	const auto timestamp = IrStd::Type::Timestamp::now();
	std::string data;
	// Once the data is available, parse it
	try
	{
		// Read the 
		IrStd::FetchUrl fetch(urlStream.str().c_str(), data);
		fetch.processSync();

		IrStd::Json json(data.c_str());
		for (const auto& pair : json.getObject("data", "rates"))
		{
			const auto it = m_stringToCurrency.find(pair.first);
			if (it != m_stringToCurrency.end() && currency != it->second)
			{
				const auto currency1 = currency;
				const auto currency2 = it->second;
				const auto pRateStr = pair.second.getString().val();
				const auto rate = IrStd::Type::Decimal::fromString(pRateStr);

				// Look for the transaction
				{
					auto scope = m_lockRates.writeScope();
					const auto pTransaction = getTransactionMap().getTransactionForWrite(currency1, currency2);
					pTransaction->setRate(rate, timestamp);
				}
			}
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}
}
