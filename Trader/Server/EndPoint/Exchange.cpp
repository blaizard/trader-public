#include "Trader/Server/EndPoint/Exchange.hpp"
#include "Trader/Generic/Event/Context.hpp"

// ---- Trader::EndPoint::Exchange --------------------------------------------

void Trader::EndPoint::Exchange::setup(IrStd::ServerREST& server)
{
	setupList(server);
	setupConfiguration(server);
	setupBalance(server);
	setupInitialBalance(server);
	setupRates(server);
	setupCurrencies(server);
	setupTransactions(server);
	setupActiveOrders(server);
	setupOrders(server);
}

void Trader::EndPoint::Exchange::setupList(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/list", [&](IrStd::ServerREST::Context& context) {
		IrStd::Json json({
			{"list", {}}
		});
		m_trader.eachExchanges([&](Trader::Exchange& exchange) {
			const IrStd::Json jsonExchange({
				{"name", exchange.getId().c_str()},
				{"status", static_cast<size_t>(exchange.getStatus())},
				{"timestamp", static_cast<uint64_t>(IrStd::Type::Timestamp::now())},
				{"timestampServer", static_cast<uint64_t>(exchange.getServerTimestamp())},
				{"timestampConnected", static_cast<uint64_t>(exchange.getConnectedTimestamp())}
			});
			json.getArray("list").add(json, jsonExchange);
		});
		context.getResponse().setData(json);
	});
}

void Trader::EndPoint::Exchange::setupConfiguration(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/configuration", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);
		const auto& configuration = exchange.getConfiguration();
		context.getResponse().setData(configuration.getJson());
	});
}

void Trader::EndPoint::Exchange::setupBalance(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/balance", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);

		{
			IrStd::Json json;
			exchange.getBalance().toJson(json);
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Exchange::setupInitialBalance(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/initial/balance", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);

		{
			IrStd::Json json;
			exchange.getInitialBalance().toJson(json);
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Exchange::setupActiveOrders(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/orders/active", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);

		{
			IrStd::Json json({
				{"list", {}}
			});
			exchange.getTrackOrderList().each([&](const TrackOrder& track) {
				const IrStd::Json jsonOrder({
					{"id", track.getId().c_str()},
					{"orderType", track.getTypeToString()},
					{"initialCurrency", track.getOrder().getInitialCurrency()->getId()},
					{"finalCurrency", track.getOrder().getFirstOrderFinalCurrency()->getId()},
					{"amount", track.getAmount()},
					{"rate", track.getRate()},
					{"context", ((track.getContext()) ? IrStd::Type::ShortString(track.getContext()->getId()).c_str() : "-")},
					{"strategy", ((track.getContext()) ? track.getContext().cast<OperationContext>()->getStrategyId().c_str() : "-")},
					{"timeout", IrStd::Type::ShortString(track.getOrder().getTimeout()).c_str()},
					{"creationTime", track.getCreationTime()}
				});
				json.getArray("list").add(json, jsonOrder);
			});
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Exchange::setupOrders(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/orders", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);

		{
			IrStd::Json json({
				{"list", {}}
			});
			exchange.getTrackOrderList().getRecords(
					[&](const IrStd::Type::Timestamp timestamp, const char* const pType,
							const Id id, const char* const pOrderType, const CurrencyPtr initialCurrency,
							const CurrencyPtr finalCurrency, const IrStd::Type::Decimal amount,
							const IrStd::Type::Decimal rate, const size_t context,
							const char* const pMessage, const Id strategyId) {
						const IrStd::Json jsonOrder({
							{"timestamp", timestamp},
							{"type", pType},
							{"id", id.c_str()},
							{"orderType", pOrderType},
							{"initialCurrency", initialCurrency->getId()},
							{"finalCurrency", finalCurrency->getId()},
							{"amount", amount},
							{"rate", rate},
							{"context", ((context) ? IrStd::Type::ShortString(context).c_str() : "-")},
							{"message", pMessage},
							{"strategy", strategyId.c_str()}
						});
						json.getArray("list").add(json, jsonOrder);
				});
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Exchange::setupRates(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/rates", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);

		{
			IrStd::Json json({
				{"list", {}}
			});
			exchange.getTransactionMap().getTransactions([&](const CurrencyPtr from, const CurrencyPtr to,
					const PairTransactionMap::PairTransactionPointer pTransaction) {
				const IrStd::Json jsonRate({
						{"initialCurrency", from->getId()},
						{"finalCurrency", to->getId()},
						{"rate", pTransaction->getRate()}
				});
				json.getArray("list").add(json, jsonRate);
			});
			context.getResponse().setData(json);
		}
	});

	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/rates/{STRING}/{STRING}", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const std::string currencyInitial = context.getMatchAsString(1);
		const std::string currencyFinal = context.getMatchAsString(2);
		const auto currencies = Currency::tickerToCurrency(currencyInitial.c_str(), currencyFinal.c_str());
		const auto& exchange = m_trader.getExchange(index);

		IRSTD_THROW_ASSERT(currencies.first && currencies.second, "Unrecognized currencies "
				<< currencyInitial << "and/or " << currencyFinal);

		const auto pTransaction = exchange.getTransactionMap().getTransaction(currencies.first, currencies.second);

		IRSTD_THROW_ASSERT(pTransaction, "This transaction pair " << currencies.first << "/"
				<< currencies.second << ", is not available on this exchange");

		{
			IrStd::Json json({
				{"list", {}}
			});
			const IrStd::Type::Timestamp curTimestamp = IrStd::Type::Timestamp::now();
			pTransaction->getRates(curTimestamp, curTimestamp - IrStd::Type::Timestamp::min(15),
					[&](const IrStd::Type::Timestamp t, const IrStd::Type::Decimal rate) {
						const IrStd::Json jsonRate({
							{"t", t},
							{"r", rate}
						});
						json.getArray("list").add(json, jsonRate);
			});
			context.getResponse().setData(json);
		}
	});

	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/rates/{STRING}/{STRING}/{UINT}/{UINT}", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const std::string currencyInitial = context.getMatchAsString(1);
		const std::string currencyFinal = context.getMatchAsString(2);
		const auto currencies = Currency::tickerToCurrency(currencyInitial.c_str(), currencyFinal.c_str());
		const auto& exchange = m_trader.getExchange(index);

		IRSTD_THROW_ASSERT(currencies.first, "Unrecognized currencies " << currencyInitial
				<< "and/or " << currencyFinal);

		const auto pTransaction = exchange.getTransactionMap().getTransaction(currencies.first, currencies.second);

		IRSTD_THROW_ASSERT(pTransaction, "This transaction pair " << currencyInitial << "/"
				<< currencyFinal << ", is not available on this exchange");

		const IrStd::Type::Timestamp timestampFrom = context.getMatchAsUInt(3);
		const IrStd::Type::Timestamp timestampTo = context.getMatchAsUInt(4);

		{
			IrStd::Json json({
				{"list", {}}
			});
			pTransaction->getRates(timestampFrom, timestampTo,
					[&](const IrStd::Type::Timestamp t, const IrStd::Type::Decimal rate) {
						const IrStd::Json jsonRate({
							{"t", t},
							{"r", rate}
						});
						json.getArray("list").add(json, jsonRate);
			});
			context.getResponse().setData(json);
		}
	});
}


void Trader::EndPoint::Exchange::setupCurrencies(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/currencies", [&](IrStd::ServerREST::Context& context) {
		{
			IrStd::Json json({
				{"list", {}}
			});
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Exchange::setupTransactions(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/exchange/{UINT}/transactions", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& exchange = m_trader.getExchange(index);

		{
			IrStd::Json json({
				{"list", {}}
			});
			exchange.getTransactionMap().getTransactions([&](const CurrencyPtr from, const CurrencyPtr to, const PairTransactionMap::PairTransactionPointer pTransaction) {
				const auto decimal = pTransaction->getDecimalPlace();
				const auto decimalOrder = pTransaction->getOrderDecimalPlace();
				// Compute the fee
				std::string fee;
				{
					const auto feeFixed = pTransaction->getFeeFixed();
					const auto feePercent = pTransaction->getFeePercent();
					fee.assign(IrStd::Type::ShortString(feeFixed));
					fee.append(" + ");
					fee.append(IrStd::Type::ShortString(feePercent));
					fee.append("%");
				}

				// Compute the boundaries
				std::stringstream boundariesStream;
				pTransaction->getBoundaries().toStream(boundariesStream);

				const IrStd::Json jsonTransaction({
					{"initialCurrency", from->getId()},
					{"finalCurrency", to->getId()},
					{"decimal", (decimal == 14) ? "-" : IrStd::Type::ShortString(decimal).c_str()},
					{"decimalOrder", (decimalOrder == 14) ? "-" : IrStd::Type::ShortString(decimalOrder).c_str()},
					{"fee", fee},
					{"boundaries", boundariesStream.str()}

				});
				json.getArray("list").add(json, jsonTransaction);
			});
			context.getResponse().setData(json);
		}
	});
}
