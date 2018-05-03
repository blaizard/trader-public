#include <algorithm>
#include <string>

#include "Trader/ExchangeImpl/Bitfinex/ExchangeBitfinex.hpp"

IRSTD_TOPIC_REGISTER(Trader, Bitfinex);
IRSTD_TOPIC_USE_ALIAS(TraderBitfinex, Trader, Bitfinex);

// ---- Trader::ExchangeBitfinex --------------------------------------------------

Trader::ExchangeBitfinex::ExchangeBitfinex()
		: Exchange(Id("Bitfinex"), ConfigurationExchange({
			{"ratesPollingPeriodMs", 5000}
		}))
{
}

/**
 * "pair":"btcusd",
 * "price_precision":5,
 * "initial_margin":"30.0",
 * "minimum_margin":"15.0",
 * "maximum_order_size":"2000.0",
 * "minimum_order_size":"0.01",
 * "expiration":"NA"
 */
void Trader::ExchangeBitfinex::updatePropertiesImpl(PairTransactionMap& transactionMap)
{
	std::string data;
	std::string pairs;

	IrStd::FetchUrl fetch("https://api.bitfinex.com/v1/symbols_details", data);
	fetch.processSync();

	try
	{
		// Parse a data json
		IrStd::Json json(data.c_str());
		auto& array = json.getArray();
		for (size_t i=0; i<array.size(); i++)
		{
			auto& elt = array.getObject(i);
			auto ticker = std::string(elt.getString("pair").val());
			const auto currencies = Currency::tickerToCurrency(ticker.c_str(), /*symbolLength*/static_cast<size_t>(3));
			const auto currency1 = currencies.first;
			const auto currency2 = currencies.second;
			if (!currency1 || !currency2)
			{
				continue;
			}

			PairTransactionImpl transaction(
					currency1,
					currency2,
					ticker);

			{
				Boundaries boundaries;
				const auto minimumOrderSize = std::atof(elt.getString("minimum_order_size").val());
				const auto maximumOrderSize = std::atof(elt.getString("maximum_order_size").val());
				boundaries.setInitialAmount(minimumOrderSize, maximumOrderSize);
				boundaries.setFinalAmount(minimumOrderSize, maximumOrderSize);
				transaction.setBoundaries(boundaries);
			}
			transaction.setFeePercent(0.2);
			transaction.setDecimalPlace(elt.getNumber("price_precision").val());

			transactionMap.registerPair<PairTransactionImpl>(transaction);
			transactionMap.registerInvertPair<InvertPairTransactionImpl>(transaction);

			// Add it to the tickers
			std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);
			pairs += ((pairs.empty()) ? "" : ",");
			pairs += "t" + std::string(ticker);
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}

	// Build the ticker URL
	m_tickerUrl = "https://api.bitfinex.com/v2/tickers?symbols=" + pairs;
}

/**
 * https://docs.bitfinex.com/v2/reference#rest-public-tickers
 *[
 * // on trading pairs (ex. tBTCUSD)
 * [
 *   SYMBOL,
 *   BID,               float	Price of last highest bid
 *   BID_SIZE,          float	Size of the last highest bid
 *   ASK,               float	Price of last lowest ask
 *   ASK_SIZE,          float	Size of the last lowest ask
 *   DAILY_CHANGE,      float	Amount that the last price has changed since yesterday
 *   DAILY_CHANGE_PERC, float	Amount that the price has changed expressed in percentage terms
 *   LAST_PRICE,        float	Price of the last trade
 *   VOLUME,            float	Daily volume
 *   HIGH,              float	Daily high
 *   LOW                float	Daily low
 * ],
 */
void Trader::ExchangeBitfinex::updateRatesImpl()
{
	std::string data;
	const IrStd::Type::Timestamp timestamp = IrStd::Type::Timestamp::now();

	IrStd::FetchUrl fetch(m_tickerUrl.c_str(), data);
	fetch.processSync();

	try
	{
		// Parse a data json
		IrStd::Json json(data.c_str());
		// Activate the scope
		auto scope = m_lockRates.writeScope();
		auto& array = json.getArray();
		for (size_t i=0; i<array.size(); i++)
		{
			auto& elt = array.getArray(i);
			auto ticker = elt.getString(0).val();

			const auto currencies = Currency::tickerToCurrency(&ticker[1], /*delimiter*/static_cast<size_t>(3));
			const auto currency1 = currencies.first;
			const auto currency2 = currencies.second;
			IRSTD_THROW_ASSERT(TraderBitfinex, currency1 && currency2, "Unable to identify the currencies");

			// Set bid and ask prices
			{
				const auto bidPrice = elt.getNumber(1).val();
				const auto askPrice = elt.getNumber(3).val();
				auto pTransaction = getTransactionMap().getTransactionForWrite(currency1, currency2);
				pTransaction->setBidPrice(bidPrice, timestamp);
				pTransaction->setAskPrice(askPrice, timestamp);
			}
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}
}
