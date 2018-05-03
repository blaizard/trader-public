#include "Trader/Server/EndPoint/Strategy.hpp"

// ---- Trader::EndPoint::Strategy --------------------------------------------

void Trader::EndPoint::Strategy::setup(IrStd::ServerREST& server)
{
	setupList(server);
	setupStatus(server);
	setupProfit(server);
	setupConfiguration(server);
	setupData(server);
	setupOperations(server);
}

void Trader::EndPoint::Strategy::setupList(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/strategy/list", [&](IrStd::ServerREST::Context& context) {
		IrStd::Json json({
			{"list", {}}
		});
		m_trader.eachStrategies([&](Trader::Strategy& strategy) {
			const IrStd::Json jsonStrategy({
				{"name", strategy.getId().c_str()},
				{"type", strategy.getType().c_str()}
			});
			json.getArray("list").add(json, jsonStrategy);
		});
		context.getResponse().setData(json);
	});
}

void Trader::EndPoint::Strategy::setupStatus(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/strategy/{UINT}/status", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& strategy = m_trader.getStrategy(index);

		{
			const IrStd::Json json({
				{"processTime", strategy.getProcessTimePercent()}
			});
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Strategy::setupProfit(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/strategy/{UINT}/profit", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& strategy = m_trader.getStrategy(index);

		{
			IrStd::Json json;
			strategy.getProfitInJson(json);
			context.getResponse().setData(json);
		}
	});
}

void Trader::EndPoint::Strategy::setupConfiguration(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/strategy/{UINT}/configuration", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& strategy = m_trader.getStrategy(index);
		const auto& configuration = strategy.getConfiguration();
		context.getResponse().setData(configuration.getJson());
	});
}

void Trader::EndPoint::Strategy::setupData(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/strategy/{UINT}/data", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& strategy = m_trader.getStrategy(index);
		context.getResponse().setData(strategy.getJson());
	});
}

void Trader::EndPoint::Strategy::setupOperations(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/strategy/{UINT}/operations", [&](IrStd::ServerREST::Context& context) {
		const size_t index = context.getMatchAsUInt(0);
		const auto& strategy = m_trader.getStrategy(index);

		{
			IrStd::Json json({
				{"list", {}}
			});
			strategy.getRecordedOperations(
					[&](const IrStd::Type::Timestamp timestamp, const char* const pStatus,
							const size_t id, const IrStd::Type::Decimal profit,
							const char* const currency, const char* const pDescription) {
						const IrStd::Json jsonOrder({
							{"timestamp", timestamp},
							{"status", pStatus},
							{"id", id},
							{"profit", profit},
							{"currency", currency},
							{"description", pDescription}
						});
						json.getArray("list").add(json, jsonOrder);
				});
			context.getResponse().setData(json);
		}
	});
}
