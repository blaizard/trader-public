#include "Trader/Server/EndPoint/Manager.hpp"

// ---- Trader::EndPoint::Manager ---------------------------------------------

void Trader::EndPoint::Manager::setup(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/manager", [&](IrStd::ServerREST::Context& context) {
		IrStd::Json json({
			{"version", IrStd::Main::getInstance().getVersion()},
			{"time", IrStd::Type::Timestamp::now()},
			{"startTime", m_trader.getStartedTimestamp()},
			{"memoryAllocCurrent", IrStd::Memory::getInstance().getStatCurrent()},
			{"memoryAllocPeak", IrStd::Memory::getInstance().getStatPeak()},
			{"memoryRAMTotal", IrStd::Memory::getInstance().getRAMTotal()},
			{"memoryRAMTotalUsed", IrStd::Memory::getInstance().getRAMTotalUsed()},
			{"memoryRAMCurrent", IrStd::Memory::getInstance().getRAMCurrent()},
			{"memoryVirtualMemoryTotal", IrStd::Memory::getInstance().getVirtualMemoryTotal()},
			{"memoryVirtualMemoryTotalUsed", IrStd::Memory::getInstance().getVirtualMemoryTotalUsed()},
			{"memoryVirtualMemoryCurrent", IrStd::Memory::getInstance().getVirtualMemoryCurrent()}
		});
		context.getResponse().setData(json);
	});

	// Add other endpoints
	setupTraces(server);
	setupThreads(server);
}

void Trader::EndPoint::Manager::setupTraces(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/manager/traces", [&](IrStd::ServerREST::Context& context) {
		IrStd::Json json({{"list", {}}});
		m_trader.getTraces().read([&](const IrStd::Type::Timestamp timestamp, const Trader::ManagerTrace::TraceInfo& info) {
			const char level[2] = {IrStd::Logger::levelToChar(info.m_level), '\0'};
			const IrStd::Json jsonTrace({
					{"timestamp", timestamp},
					{"level", level},
					{"topic", info.m_pTopic->getStr()},
					{"message", info.m_message}
			});
			json.getArray("list").add(json, jsonTrace);
		});

		context.getResponse().setData(json);
	});
}

void Trader::EndPoint::Manager::setupThreads(IrStd::ServerREST& server)
{
	server.addRoute(IrStd::HTTPMethod::GET, "/api/v1/manager/threads", [&](IrStd::ServerREST::Context& context) {
		IrStd::Json json({{"list", {}}});
		 IrStd::Threads::each([&](const IrStd::Thread& thread) {
			const IrStd::Json jsonThread({
					{"name", thread.getName()},
					{"isActive", thread.isActive()}
			});
			json.getArray("list").add(json, jsonThread);
		 });
		context.getResponse().setData(json);
	});
}
