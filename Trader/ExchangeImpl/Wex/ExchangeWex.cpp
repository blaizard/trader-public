#include <string>
#include <memory>
#include <chrono>
#include <regex>
#include "IrStd/IrStd.hpp"

#include "Trader/ExchangeImpl/Wex/ExchangeWex.hpp"

IRSTD_TOPIC_REGISTER(Trader, Wex);
IRSTD_TOPIC_USE_ALIAS(TraderWex, Trader, Wex);

#define WEX_API_URL "https://wex.nz"

// ---- Trader::ExchangeWex --------------------------------------------------

Trader::ExchangeWex::ExchangeWex(
		const char* const key,
		const char* const secret)
		: Exchange(Id("Wex"), ConfigurationExchange({
			{"ratesPollingPeriodMs", 1000},
			{"balanceIncludeReserve", false}
		}))
		, m_terminate(false)
		, m_key(key)
		, m_secret(secret)
		, m_nonce(1)
{
}

/**
 * Authorization is performed by sending the following HTTP Headers:
 * Key — API key. The example of API key: 46G9R9D6-WJ77XOIP-XH9HH5VQ-A3XN3YOZ-8T1R8I8T;
 * Sign — POST data (?param=val&param1=val1) signed by a secret key according to HMAC-SHA512 method;
 * Sent on https://wex.nz/tapi .
 * All requests must also include a special nonce POST parameter with increment integer. (>0)
 */
void Trader::ExchangeWex::makeFetchForPrivateAPI(
		IrStd::FetchUrl& fetch,
		const char* const command)
{
	fetch.addPost("method", command);
	fetch.addPost("nonce", m_nonce++);

	IrStd::Crypto crypto(fetch.getPost().c_str());
	auto hex = crypto.HMACSHA512(m_secret)->hex();

	fetch.addHeader("Key", m_key);
	fetch.addHeader("Sign", hex.c_str());
}

IrStd::Json Trader::ExchangeWex::sanityCheckPrivateAPIResponse(const std::string& response)
{
	IrStd::Json json(response.c_str());

	auto& success = json.getNumber("success");
	if (success.val() == 0)
	{
		auto& error = json.getString("error");
		// If the error is the synchronization with nonce, update nonce and
		// throw an exception with retry.
		{
			const std::regex re(".*you should send:([0-9]+).*");
			const std::string errorStr(error.val());
			std::smatch base_match;
			if (std::regex_match(errorStr, base_match, re))
			{
				if (base_match.size() == 2)
				{
					const std::ssub_match base_sub_match = base_match[1];
					m_nonce = std::stoi(base_sub_match.str());
					IRSTD_THROW_RETRY("Adjusting nonce value to '" << m_nonce << "' (" << errorStr << ")");
				}
			}
		}
		IRSTD_THROW(error.val() << ", dump=" << response);
	}

	IRSTD_THROW_ASSERT(json.isObject("return"));
	return json;
}

void Trader::ExchangeWex::updateBalanceImpl(Balance& balance)
{
	std::string data;
	IrStd::FetchUrl fetch(WEX_API_URL "/tapi", data);
	makeFetchForPrivateAPI(fetch, "getInfo");
	fetch.processSync();

	IrStd::Json json = sanityCheckPrivateAPIResponse(data);
	{
		for (auto elt : json.getObject("return", "funds"))
		{
			const auto amount = elt.second.getNumber().val();
			const auto currency = Currency::discover(elt.first);
			if (currency)
			{
				balance.set(currency, amount);
			}
		}
	}
}

/**
 * Example of response:
 * {
 *     "success":1,
 *     "return":{
 *         "343152":{                          // Order ID.
 *             "pair":"btc_usd",               // The pair on which the order was created.
 *             "type":"sell",                  // Order type, buy/sell.
 *             "amount":1.00000000,            // The amount of currency to be bought/sold.
 *             "rate":3.00000000,              // Sell/Buy price.
 *             "timestamp_created":1342448420, // The time when the order was created.
 *             "status":0                      // Deprecated, is always equal to 0.
 *         }
 *     }
 * }
 */
void Trader::ExchangeWex::updateOrdersImpl(std::vector<TrackOrder>& trackOrders)
{
	std::string data;
	IrStd::FetchUrl fetch(WEX_API_URL "/tapi", data);
	makeFetchForPrivateAPI(fetch, "ActiveOrders");
	fetch.processSync();

	try
	{
		sanityCheckPrivateAPIResponse(data);
	}
	catch (const IrStd::Exception& e)
	{
		if (strstr(e.what(), "no orders"))
		{
			return;
		}
		IrStd::Exception::rethrow();
	}

	IrStd::Json json(data.c_str());
	{
		for (auto elt : json.getObject("return"))
		{
			const Id id(std::stoi(elt.first));
			const auto pair = elt.second.getString("pair").val();
			const auto type = elt.second.getString("type").val();
			const auto timestamp = elt.second.getNumber("timestamp_created").val();

			// Identify the transaction
			IRSTD_THROW_ASSERT(TraderWex, !strcmp(type, "sell") || !strcmp(type, "buy"), "type=" << type);
			const auto ticker = Currency::tickerToCurrency(pair, /*delimiter*/'_');
			if (!ticker.first)
			{
				continue;
			}

			IrStd::Type::Decimal amount = 0;
			IrStd::Type::Decimal rate = 0;
			std::shared_ptr<PairTransaction> pTransaction;

			// If non inverted transaction
			if (!strcmp(type, "sell"))
			{
				amount = elt.second.getNumber("amount").val();
				rate = elt.second.getNumber("rate").val();
				pTransaction = getTransactionMap().getTransactionForWrite(ticker.first, ticker.second);
			}
			// This is an inverted transaction
			else
			{
				amount = elt.second.getNumber("amount").val() * elt.second.getNumber("rate").val();
				rate = 1. / elt.second.getNumber("rate").val();
				pTransaction = getTransactionMap().getTransactionForWrite(ticker.second, ticker.first);
			}

			IRSTD_ASSERT(amount && rate && pTransaction);
			trackOrders.push_back({id, pTransaction, rate, amount, IrStd::Type::Timestamp::s(timestamp)});
		}
	}
}

/**
 * Request:
 *     pair: pair btc_usd (example)
 *     type: order type (buy or sell)
 *     rate: the rate at which you need to buy/sell
 *     amount: the amount you need to buy / sell
 *
 * Example of response:
 * {
 *     "success":1,
 *     "return":{
 *         "received":0.1,
 *         "remains":0,
 *         "order_id":0,
 *         "funds":{
 *             "usd":325,
 *             "btc":2.498,
 *             "ltc":0,
 *             ...
 *         }
 *     }
 * }
 * With:
 *     received: The amount of currency bought/sold.
 *     remains: The remaining amount of currency to be bought/sold
 *              (and the initial order amount).
 *     order_id: Is equal to 0 if the request was fully “matched”
 *               by the opposite orders, otherwise the ID of the
 *               executed order will be returned.
 *     funds: Balance after the request.
 */
void Trader::ExchangeWex::setOrderImpl(
		const Order& order,
		const IrStd::Type::Decimal amount,
		std::vector<Id>& idList)
{
	std::string data;
	IrStd::FetchUrl fetch(WEX_API_URL "/tapi", data);

	const auto* pTransaction = static_cast<const PairTransaction*>(order.getTransaction());
	const char* type = (pTransaction->isInvertedTransaction()) ? "buy" : "sell";
	const auto& pair = pTransaction->getData().getString();
	const auto formatedData = pTransaction->getAmountAndRateForOrder(amount, order.getRate());

	fetch.addPost("pair", pair);
	fetch.addPost("type", type);
	fetch.addPost("rate", formatedData.second);
	fetch.addPost("amount", formatedData.first);
	makeFetchForPrivateAPI(fetch, "Trade");

	IRSTD_LOG_DEBUG(TraderWex, "AddOrder: pair=" << pair << ", type=" << type
			<< ", volume=" << formatedData.first << ", price=" << formatedData.second);

	fetch.processSync();
	sanityCheckPrivateAPIResponse(data);

	// Return the Id of the remaining order if any
	{
		IrStd::Json json(data.c_str());
		const auto orderId = json.getNumber("return", "order_id").val();
		if (orderId)
		{
			idList.push_back(Id(orderId));
		}
	}
}

/**
 * Cancel an order
 * Request:
 *     order_id
 */
void Trader::ExchangeWex::cancelOrderImpl(const TrackOrder& order)
{
	std::string data;
	{
		IrStd::FetchUrl fetch(WEX_API_URL "/tapi", data);
		const auto orderId = IrStd::Type::Numeric<uint64_t>::fromString(order.getId());
		fetch.addPost("order_id", orderId);
		makeFetchForPrivateAPI(fetch, "CancelOrder");
		fetch.processSync();
	}
	sanityCheckPrivateAPIResponse(data);

	IRSTD_LOG_TRACE(TraderWex, "Order id=" << order.getId() << " canceled");
}

void Trader::ExchangeWex::updateRatesImpl()
{
	std::string data;
	IrStd::FetchUrl fetch(m_tickerUrl.c_str(), data);
	fetch.processSync();

	try
	{
		// Parse a data json
		IrStd::Json json(data.c_str());
		// Activate the scope
		auto scope = m_lockRates.writeScope();
		for (auto elt : json.getObject())
		{
			const auto currencies = Currency::tickerToCurrency(elt.first, /*delimiter*/'_');
			const auto currency1 = currencies.first;
			const auto currency2 = currencies.second;
			IRSTD_THROW_ASSERT(TraderWex, currency1 && currency2, "Unable to identify one of the currencies");
			const auto timestamp = elt.second.getNumber("updated").val();

			// Update currency1 -> currency2 transaction
			{
				auto transaction = getTransactionMap().getTransactionForWrite(currency1, currency2);
				transaction->setRate(elt.second.getNumber("sell").val(), IrStd::Type::Timestamp::s(timestamp));
			}

			// Update currency2 -> currency1 transaction
			{
				auto transaction = getTransactionMap().getTransactionForWrite(currency2, currency1);
				transaction->setRate(1. / elt.second.getNumber("buy").val(), IrStd::Type::Timestamp::s(timestamp));
			}
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}
}

void Trader::ExchangeWex::updatePropertiesImpl(PairTransactionMap& transactionMap)
{
	std::string data;

	// To build the ticker URL
	std::string pairs;

	try
	{
		// List all the pairs
		IrStd::FetchUrl fetch(WEX_API_URL "/api/3/info", data);
		fetch.processSync();

		// Parse a data json
		IrStd::Json json(data.c_str());
		for (auto elt : json.getObject("pairs"))
		{
			const auto currencies = Currency::tickerToCurrency(elt.first, /*delimiter*/'_');
			const auto currency1 = currencies.first;
			const auto currency2 = currencies.second;
			if (!currency1 || !currency2)
			{
				IRSTD_LOG_WARNING(TraderWex, "Ignoring ticker '" << elt.first << "'");
				continue;
			}

			PairTransactionImpl transaction(
					currency1,
					currency2,
					elt.first);

			{
				Boundaries boundaries;
				boundaries.setInitialAmount(elt.second.getNumber("min_amount").val());
				boundaries.setRate(elt.second.getNumber("min_price").val(),
						elt.second.getNumber("max_price").val());
				transaction.setBoundaries(boundaries);
			}
			transaction.setFeePercent(elt.second.getNumber("fee").val());
			transaction.setDecimalPlace(elt.second.getNumber("decimal_places").val());
			transaction.setOrderDecimalPlace(elt.second.getNumber("decimal_places").val());

			transactionMap.registerPair<PairTransactionImpl>(transaction);
			transactionMap.registerInvertPair<InvertPairTransactionImpl>(transaction);

			// Add it to the tickers
			pairs += ((pairs.empty()) ? "" : "-");
			pairs += elt.first;
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}

	// Build the ticker URL
	m_tickerUrl = WEX_API_URL "/api/3/ticker/" + pairs;
}
