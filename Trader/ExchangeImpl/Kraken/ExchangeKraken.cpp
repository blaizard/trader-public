#include "Trader/ExchangeImpl/Kraken/ExchangeKraken.hpp"

IRSTD_TOPIC_REGISTER(Trader, Kraken);
IRSTD_TOPIC_USE_ALIAS(TraderKraken, Trader, Kraken);

#define KRAKEN_API_URL "https://api.kraken.com"

// ---- Trader::ExchangeCoinbase --------------------------------------------------

Trader::ExchangeKraken::ExchangeKraken(
	const char* const key,
	const char* const secret,
	const std::initializer_list<std::pair<CurrencyPtr, const std::string>> withdraw)
		: Exchange(Id("Kraken"), ConfigurationExchange({
			{"ratesPollingPeriodMs", 1000},
			{"orderPollingPeriodMs", /*Every 20 seconds*/20000},
			{"orderRegisterTimeoutMs", /*Max time 2 min*/60000 * 2},
			{"balanceIncludeReserve", true}
		}))
		, m_key(key)
		, m_decodedSecret(IrStd::Type::Buffer(secret).base64Decode())
		, m_withdrawList(withdraw)
{
}

void Trader::ExchangeKraken::sanityCheckResponse(const IrStd::Json& json, const std::function<void(std::stringstream&)>& onError) const
{
	if (json.isArray("error") && json.getArray("error").size())
	{
		const auto pMessage = json.getArray("error").getString(0).val();
		// Retry on some cases
		if (std::strstr(pMessage, "Invalid nonce"))
		{
			IRSTD_THROW_RETRY(TraderKraken, pMessage);
		}
		{
			std::stringstream stream;
			stream << pMessage;
			// Append a custom message
			if (onError)
			{
				stream << " (";
				onError(stream);
				stream << ")";
			}
			// Ignore
			IRSTD_THROW(TraderKraken, stream.str());
		}
	}
	IRSTD_THROW_ASSERT(TraderKraken, json.isObject("result"), "The result field of the response is missing");
}

Trader::CurrencyPtr Trader::ExchangeKraken::getCurrencyFromId(const char* const currencyId) const
{
	IRSTD_ASSERT(TraderKraken, m_lockProperties.isScope(), "The getCurrencyFromId call must be under a scope");
	const auto it = m_currencyMap.find(currencyId);
	IRSTD_THROW_ASSERT(TraderKraken, it != m_currencyMap.end(), "The currency Id " << currencyId << " is not registered");
	return it->second.m_currency;
}

std::string Trader::ExchangeKraken::getIdFromCurrency(Trader::CurrencyPtr currency) const
{
	IRSTD_ASSERT(TraderKraken, m_lockProperties.isScope(), "The getIdFromCurrency call must be under a scope");
	// Look for it by looping through existing currencies
	for (const auto& it : m_currencyMap)
	{
		if (it.second.m_currency == currency)
		{
			return it.first;
		}
	}
	IRSTD_THROW_ASSERT(TraderKraken, false, "The currency " << currency << " is not registered");
}

std::shared_ptr<Trader::PairTransaction> Trader::ExchangeKraken::getTransactionFromId(const char* const pairId) const
{
	IRSTD_ASSERT(TraderKraken, m_lockProperties.isScope(), "The getPairFromId call must be under a scope");
	const auto it = m_pairMap.find(pairId);
	IRSTD_THROW_ASSERT(TraderKraken, it != m_pairMap.end(), "The pair Id " << pairId << " is not registered");
	return it->second;
}

/**
 *  https://api.kraken.com/0/public/AssetPairs
 *
 * {
 *     "error":[],
 *     "result":{
 *         "BCHEUR":{
 *             "altname":"BCHEUR",           // alternate pair name
 *             "aclass_base":"currency",     // asset class of base component
 *             "base":"BCH",                 // asset id of base component
 *             "aclass_quote":"currency",    // asset class of quote component
 *             "quote":"ZEUR",               // asset id of quote component
 *             "lot":"unit",                 // volume lot size
 *             "pair_decimals":4,            // scaling decimal places for pair
 *             "lot_decimals":8,             // scaling decimal places for volume
 *             "lot_multiplier":1,           // amount to multiply lot volume by to get currency volume
 *             "leverage_buy":[],            // array of leverage amounts available when buying
 *             "leverage_sell":[],           // array of leverage amounts available when selling
 *             "fees":[[0,0.26],[50000,0.24],[100000,0.22],[250000,0.2],[500000,0.18],[1000000,0.16],[2500000,0.14],[5000000,0.12],[10000000,0.1]], // fee schedule array in [volume, percent fee] tuples
 *             "fees_maker":[[0,0.16],[50000,0.14],[100000,0.12],[250000,0.1],[500000,0.08],[1000000,0.06],[2500000,0.04],[5000000,0.02],[10000000,0]], // maker fee schedule array in [volume, percent fee] tuples (if on maker/taker)
 *             "fee_volume_currency":"ZUSD", // volume discount currency
 *             "margin_call":80,             // margin call level
 *             "margin_stop":40},            // stop-out/liquidation margin level
 *         "BCHUSD":{
 *             "altname":"BCHUSD"
 *             ...
 */
void Trader::ExchangeKraken::updatePropertiesImpl(PairTransactionMap& transactionMap)
{
	auto scope = m_lockProperties.writeScope();
	m_tickerUrl.clear();
	m_currencyMap.clear();
	m_pairMap.clear();

	// Set server time
	{
		std::string data;
		IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/public/Time", data);
		fetch.processSync();

		try
		{
			// Parse a data json
			IrStd::Json json(data.c_str());
			sanityCheckResponse(json);

			const size_t timestamp = IrStd::Type::Timestamp::s(json.getNumber("result", "unixtime").val());
			setServerTimestamp(timestamp);
		}
		catch (...)
		{
			IrStd::Exception::rethrowRetry();
		}
	}

	// List and identify all currencies
	{
		std::string data;
		IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/public/Assets", data);
		fetch.processSync();

		try
		{
			// Parse a data json
			IrStd::Json json(data.c_str());
			sanityCheckResponse(json);
			{
				for (const auto& pair : json.getObject("result"))
				{
					const auto currencyId = pair.first;
					std::string currencyName(pair.second.getString("altname"));
					const size_t decimal = pair.second.getNumber("decimals").val();
					const auto currency = Currency::discover(currencyName.c_str());
					if (!currency)
					{
						IRSTD_LOG_WARNING(TraderKraken, "Unknown currency " << currencyName << ", ignoring");
						continue;
					}
					m_currencyMap[currencyId] = CurrencyInfo{currency, currencyName, decimal};
				}
			}
		}
		catch (...)
		{
			IrStd::Exception::rethrowRetry();
		}
	}

	// List and identify all available pairs
	{
		std::string data;
		IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/public/AssetPairs", data);
		fetch.processSync();

		try
		{
			// Parse a data json
			IrStd::Json json(data.c_str());
			sanityCheckResponse(json);
			{
				for (const auto& pair : json.getObject("result"))
				{
					const char* const pairId = pair.first;
					const auto& item = pair.second;
					const std::string pairName(item.getString("altname"));
					const IrStd::Type::Decimal percentFee = item.getArray("fees").getArray(0).getNumber(1);
					const size_t decimalPlace = item.getNumber("pair_decimals");

					// Try to identify the pair
					for (const auto it1 : m_currencyMap)
					{
						const auto pos = pairName.find(it1.second.m_name);
						if (pos == 0)
						{
							for (const auto it2 : m_currencyMap)
							{
								const auto pos2 = pairName.find(it2.second.m_name, pos + it1.second.m_name.size());
								if (pos2 == pos + it1.second.m_name.size() && pos2 + it2.second.m_name.size() == pairName.size())
								{
									const auto currency1 = it1.second.m_currency;
									const auto currency2 = it2.second.m_currency;

									PairTransactionImpl transaction(currency1, currency2, pairId);
									transaction.setOrderDecimalPlace(decimalPlace);
									transaction.setDecimalPlace(it2.second.m_decimal);
									transaction.setFeePercent(percentFee);
									auto pTransaction = transactionMap.registerPair<PairTransactionImpl>(transaction);

									// Set the decimal place
									auto pInvertTransaction = transactionMap.registerInvertPair<InvertPairTransactionImpl>(transaction);
									pInvertTransaction->setDecimalPlace(it1.second.m_decimal);

									// Save the properties
									m_tickerUrl.append((m_tickerUrl.empty()) ? KRAKEN_API_URL "/0/public/Ticker?pair=" : ",");
									m_tickerUrl.append(pairId);
									m_pairMap[pairId] = pTransaction;
									m_pairMap[pairName] = pTransaction;

									break;
								}
							}
							break;
						}
					}
				}
			}
		}
		catch (...)
		{
			IrStd::Exception::rethrowRetry();
		}
	}
}

/**
 * {
 *     "error":[],
 *     "result":{
 *         "BCHEUR":{
 *             "a":["571.499900","1","1.000"],         // ask array(<price>, <whole lot volume>, <lot volume>),
 *             "b":["571.000000","1","1.000"],         // bid array(<price>, <whole lot volume>, <lot volume>),
 *             "c":["571.000000","0.06515420"],        // last trade closed array(<price>, <lot volume>),
 *             "v":["4284.25902022","14047.50964711"], // volume array(<today>, <last 24 hours>),
 *             "p":["574.288754","569.610680"],        // volume weighted average price array(<today>, <last 24 hours>),
 *             "t":[4786,13281],                       // number of trades array(<today>, <last 24 hours>),
 *             "l":["559.000000","530.000000"],        // low array(<today>, <last 24 hours>),
 *             "h":["598.800000","599.899900"],        // high array(<today>, <last 24 hours>),
 *             "o":"581.000000"},                      // today's opening price
 *         "BCHUSD":{
 *             "a":["669.799900","1","1.000"],
 *             ...
 */
void Trader::ExchangeKraken::updateRatesImpl()
{
	std::string data;
	IrStd::FetchUrl fetch(m_tickerUrl.c_str(), data);
	fetch.processSync();

	const auto timestamp = IrStd::Type::Timestamp::now();
	try
	{
		// Parse a data json
		IrStd::Json json(data.c_str());
		sanityCheckResponse(json);
		const auto& result = json.getObject("result");
		getTransactionMap().getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2,
				const PairTransactionMap::PairTransactionPointer pTransaction) {
			if (!pTransaction->isInvertedTransaction())
			{
				const auto& pairId = pTransaction->getData().getString();
				IRSTD_THROW_ASSERT(TraderKraken, result.isObject(pairId.c_str()),
						"The pair '" << pairId << "' is not registered");
				{
					const auto ask = IrStd::Type::Decimal::fromString(result.getArray(pairId.c_str(), "a").getString(0));
					const auto bid = IrStd::Type::Decimal::fromString(result.getArray(pairId.c_str(), "b").getString(0));
					auto pTransactionForWrite = getTransactionMap().getTransactionForWrite(currency1, currency2);
					if (ask && bid)
					{
						pTransactionForWrite->setBidPrice(bid, timestamp);
						pTransactionForWrite->setAskPrice(ask, timestamp);
					}
				}
			}
		});
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}
}

/**
 * Authorization is performed by sending the following HTTP Headers:
 * API-Key = API key
 * API-Sign = Message signature using HMAC-SHA512 of (URI path + SHA256(nonce + POST data)) and base64 decoded secret API key
 * Sent using POST on https://api.kraken.com./tapi .
 * All requests must also include a special nonce POST parameter with increment integer. (>0)
 */
void Trader::ExchangeKraken::makeFetchForPrivateAPI(
		IrStd::FetchUrl& fetch,
		const char* const pUri) const
{
	// Add a rate limit here to limit private api requests due to API call rate limit,
	// see: https://support.kraken.com/hc/en-us/articles/206548367-What-is-the-API-call-rate-limit-
	IRTSD_LIMIT_RATE_MS(1000);

	static uint64_t nonce = IrStd::Type::Timestamp::now();
	fetch.addPost("nonce", ++nonce);

	std::string payloadForSignature;
	payloadForSignature.assign(IrStd::Type::ShortString(nonce));
	payloadForSignature.append(fetch.getPost().c_str());
	const auto pSHA256 = IrStd::Crypto({payloadForSignature, /*copy*/false}).SHA256();

	IrStd::Type::Buffer buffer(strlen(pUri) + pSHA256->size());
	buffer.memcpy(0, pUri);
	buffer.memcpy(strlen(pUri), *pSHA256);

	const IrStd::Crypto crypto2(buffer, /*copy*/false);
	const auto pHMACSHA512 = crypto2.HMACSHA512({m_decodedSecret, /*copy*/false});
	const auto apiSign = IrStd::Type::Buffer(*pHMACSHA512).base64Encode();

	fetch.addHeader("API-Key", m_key);
	fetch.addHeader("API-Sign", apiSign);
}

/**
 * https://api.kraken.com/0/private/Balance
 * Array
 * (
 *     [error] => Array
 *         (
 *         )
 * 
 *     [result] => Array
 *         (
 *             [ZUSD] => 3415.8014
 *             [ZEUR] => 155.5649
 *             [XBTC] => 149.9688412800
 *             [XXRP] => 499889.51600000
 *         )
 * 
 * )
 */
void Trader::ExchangeKraken::updateBalanceImpl(Balance& balance)
{
	std::string data;
	IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/private/Balance", data);
	makeFetchForPrivateAPI(fetch, "/0/private/Balance");
	fetch.processSync();

	try
	{
		IrStd::Json json(data.c_str());
		sanityCheckResponse(json);

		{
			auto scope = m_lockProperties.readScope();
			for (const auto& pair : json.getObject("result"))
			{
				const auto currency = getCurrencyFromId(pair.first);
				const auto funds = IrStd::Type::Decimal::fromString(pair.second.getString());

				balance.set(currency, funds);
			}
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}
}

/**
 * https://api.kraken.com/0/private/OpenOrders
 *
 * open: {
 *    orderId: {
 *        refid = Referral order transaction id that created this order
 *        userref = user reference id
 *        status = status of order:
 *                 pending = order pending book entry
 *                 open = open order
 *                 closed = closed order
 *                 canceled = order canceled
 *                 expired = order expired
 *        opentm = unix timestamp of when order was placed
 *        starttm = unix timestamp of order start time (or 0 if not set)
 *        expiretm = unix timestamp of order end time (or 0 if not set)
 *        descr = order description info
 *                pair = asset pair
 *                type = type of order (buy/sell)
 *                ordertype = order type (See Add standard order)
 *                price = primary price
 *                price2 = secondary price
 *                leverage = amount of leverage
 *                order = order description
 *                close = conditional close order description (if conditional close set)
 *        vol = volume of order (base currency unless viqc set in oflags)
 *        vol_exec = volume executed (base currency unless viqc set in oflags)
 *        cost = total cost (quote currency unless unless viqc set in oflags)
 *        fee = total fee (quote currency)
 *        price = average price (quote currency unless viqc set in oflags)
 *        stopprice = stop price (quote currency, for trailing stops)
 *        limitprice = triggered limit price (quote currency, when limit based order type triggered)
 *        misc = comma delimited list of miscellaneous info
 *               stopped = triggered by stop price
 *               touched = triggered by touch price
 *               liquidated = liquidation
 *               partial = partial fill
 *        oflags = comma delimited list of order flags
 *                 viqc = volume in quote currency
 *                 fcib = prefer fee in base currency (default if selling)
 *                 fciq = prefer fee in quote currency (default if buying)
 *                 nompp = no market price protection
 *        trades = array of trade ids related to order (if trades info requested and data available)
 *    }
 * }
 *
 * For BUY of pair XRP/EUR (amount 50, rate 0.15)
 {
    "error": [],
    "result": {
        "open": {
            "OSP6NM-XWGKW-NO5QH5": {
                "refid": null,
                "userref": null,
                "status": "open",
                "opentm": 1505391938.2446,
                "starttm": 0,
                "expiretm": 0,
                "descr": {
                    "pair": "XRPEUR",
                    "type": "buy",
                    "ordertype": "limit",
                    "price": "0.15000",
                    "price2": "0",
                    "leverage": "none",
                    "order": "buy 50.00000000 XRPEUR @ limit 0.15000"
                },
                "vol": "50.00000000",
                "vol_exec": "0.00000000",
                "cost": "0.00000000",
                "fee": "0.00000000",
                "price": "0.00000000",
                "misc": "",
                "oflags": "fciq"
            }
        }
    }
 */
void Trader::ExchangeKraken::updateOrdersImpl(std::vector<TrackOrder>& trackOrders)
{
	std::string data;
	IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/private/OpenOrders", data);
	makeFetchForPrivateAPI(fetch, "/0/private/OpenOrders");
	fetch.processSync();

	try
	{
		IrStd::Json json(data.c_str());
		sanityCheckResponse(json);

		auto scope = m_lockProperties.readScope();
		for (const auto& pair : json.getObject("result", "open"))
		{
			IrStd::Type::Decimal amount = 0;
			IrStd::Type::Decimal rate = 0;
			std::shared_ptr<PairTransaction> pTransaction = nullptr;

			const auto orderId = Id(pair.first);

			const auto& orderInfo = pair.second;
			const auto& orderDesc = orderInfo.getObject("descr");
			const auto timestamp = IrStd::Type::Timestamp::s(orderInfo.getNumber("opentm").val());

			// Get information of the transaction
			const auto orderSummary = orderDesc.getString("order").val();
			const auto pairId = orderDesc.getString("pair").val();
			const auto type = orderDesc.getString("type").val();
			const auto pTrans = getTransactionFromId(pairId);
			const auto orderType = orderDesc.getString("ordertype").val();

			if (::strcmp(orderType, "limit") == 0)
			{
				amount = IrStd::Type::Decimal::fromString(orderInfo.getString("vol"))
						- IrStd::Type::Decimal::fromString(orderInfo.getString("vol_exec"));
				rate = IrStd::Type::Decimal::fromString(orderDesc.getString("price"));
			}
			else
			{
				IRSTD_LOG_WARNING(TraderKraken, "Ignoring this order, unknown type: "
						<< orderType << " (" << orderSummary << ")");
			}

			if (::strcmp(type, "sell") == 0)
			{
				pTransaction = pTrans->getSellTransaction();
			}
			else if (::strcmp(type, "buy") == 0)
			{
				// Fees are already removed, need to add them back
				pTransaction = pTrans->getBuyTransaction();
				amount = amount * rate;
				rate = 1. / rate;
				amount += pTransaction->getFeeInitialCurrency(amount, rate);
			}
			else
			{
				IRSTD_THROW(TraderKraken, "Unknown order type " << type);
			}

			IRSTD_THROW_ASSERT(TraderKraken, rate > 0 && amount > 0 && pTransaction, "Invalid data, rate="
					<< rate << ", amount=" << amount << ", pTransaction=" << *pTransaction);

			trackOrders.push_back({orderId, pTransaction, rate, amount, timestamp});
		}
	}
	catch (...)
	{
		IrStd::Exception::rethrowRetry();
	}
}

/**
URL: https://api.kraken.com/0/private/AddOrder

Input:

pair = asset pair
type = type of order (buy/sell)
ordertype = order type:
    market
    limit (price = limit price)
    stop-loss (price = stop loss price)
    take-profit (price = take profit price)
    stop-loss-profit (price = stop loss price, price2 = take profit price)
    stop-loss-profit-limit (price = stop loss price, price2 = take profit price)
    stop-loss-limit (price = stop loss trigger price, price2 = triggered limit price)
    take-profit-limit (price = take profit trigger price, price2 = triggered limit price)
    trailing-stop (price = trailing stop offset)
    trailing-stop-limit (price = trailing stop offset, price2 = triggered limit offset)
    stop-loss-and-limit (price = stop loss price, price2 = limit price)
    settle-position
price = price (optional.  dependent upon ordertype)
price2 = secondary price (optional.  dependent upon ordertype)
volume = order volume in lots
leverage = amount of leverage desired (optional.  default = none)
oflags = comma delimited list of order flags (optional):
    viqc = volume in quote currency (not available for leveraged orders)
    fcib = prefer fee in base currency
    fciq = prefer fee in quote currency
    nompp = no market price protection
    post = post only order (available when ordertype = limit)
starttm = scheduled start time (optional):
    0 = now (default)
    +<n> = schedule start time <n> seconds from now
    <n> = unix timestamp of start time
expiretm = expiration time (optional):
    0 = no expiration (default)
    +<n> = expire <n> seconds from now
    <n> = unix timestamp of expiration time
userref = user reference id.  32-bit signed number.  (optional)
validate = validate inputs only.  do not submit order (optional)

optional closing order to add to system when order gets filled:
    close[ordertype] = order type
    close[price] = price
    close[price2] = secondary price
 *
 Return:
 {
    "error": [],
    "result": {
        "descr": {
            "order": "buy 0.41004261 BCHXBT @ limit 0.11549"
        },
        "txid": [
            "O3VU5C-VEHIK-BK5N2P"
        ]
    }
}

 */
void Trader::ExchangeKraken::setOrderImpl(
		const Order& order,
		const IrStd::Type::Decimal amount,
		std::vector<Id>& idList)
{
	std::string data;
	IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/private/AddOrder", data);

	const auto* pTransaction = static_cast<const PairTransaction*>(order.getTransaction());
	const auto& pair = pTransaction->getData().getString();
	const char* type = (pTransaction->isInvertedTransaction()) ? "buy" : "sell";
	const auto formatedData = pTransaction->getAmountAndRateForOrder(amount, order.getRate());

	fetch.addPost("pair", pair);
	fetch.addPost("type", type);
	fetch.addPost("ordertype", "limit");
	fetch.addPost("volume", formatedData.first);
	fetch.addPost("price", formatedData.second);

	// This gives: EAPI:Feature disabled:viqc
	// fetch.addPost("oflags", "viqc"); // The volume is expressed in the quote currency

	IRSTD_LOG_DEBUG(TraderKraken, "AddOrder: pair=" << pair << ", type=" << type
			<< ", volume=" << formatedData.first << ", price=" << formatedData.second);

	makeFetchForPrivateAPI(fetch, "/0/private/AddOrder");
	fetch.processSync();

	IrStd::Json json(data.c_str());
	sanityCheckResponse(json, [&](std::stringstream& stream){
		stream << "pair=" << pair << ", type=" << type
			<< ", volume=" << formatedData.first << ", price=" << formatedData.second;
	});

	// Return the order Id if a new order is created
	if (json.isArray("result", "txid"))
	{
		const auto& txidArray = json.getArray("result", "txid");
		for (size_t i=0; i<txidArray.size(); i++)
		{
			idList.push_back(Id(txidArray.getString(i).val()));
		}
	}
}

void Trader::ExchangeKraken::cancelOrderImpl(const TrackOrder& order)
{
	std::string data;
	IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/private/CancelOrder", data);
	fetch.addPost("txid", order.getId());

	IRSTD_LOG_DEBUG(TraderKraken, "CancelOrder: id=" << order.getId());

	makeFetchForPrivateAPI(fetch, "/0/private/CancelOrder");
	fetch.processSync();

	IrStd::Json json(data.c_str());
	sanityCheckResponse(json);
}

/**
 * URL: https://api.kraken.com/0/private/Withdraw
 * Input:
 *    aclass = asset class (optional): currency (default)
 *    asset = asset being withdrawn
 *    key = withdrawal key name, as set up on your account
 *    amount = amount to withdraw, including fees
 * Result:
 *    associative array of withdrawal transaction:
 *    refid = reference id
 */
void Trader::ExchangeKraken::withdrawImpl(const CurrencyPtr currency, const IrStd::Type::Decimal amount)
{
	// Make sure this withdraw is available
	for (const auto& withdraw : m_withdrawList)
	{
		if (withdraw.first == currency)
		{
			auto scope = m_lockProperties.readScope();

			std::string data;
			IrStd::FetchUrl fetch(KRAKEN_API_URL "/0/private/Withdraw", data);
			fetch.addPost("asset", getIdFromCurrency(currency));
			fetch.addPost("key", withdraw.second);
			fetch.addPost("amount", amount);

			IRSTD_LOG_DEBUG(TraderKraken, "Withdraw: asset=" << getIdFromCurrency(currency) << ", amount=" << amount);

			makeFetchForPrivateAPI(fetch, "/0/private/Withdraw");
			fetch.processSync();

			IrStd::Json json(data.c_str());
			sanityCheckResponse(json);

			return;
		}
	}

	IRSTD_THROW(TraderKraken, "Withdraw for " << currency << " is not supported");
}
