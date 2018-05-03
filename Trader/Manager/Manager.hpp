#pragma once

#include <vector>

#include "Trader/Exchange/Exchange.hpp"
#include "Trader/Exchange/ExchangeReadOnly.hpp"
#include "Trader/Exchange/ExchangeMock.hpp"
#include "Trader/Strategy/Strategy.hpp"
#include "Trader/Server/Server.hpp"

namespace Trader
{
	class Manager;
	class ManagerEnvironment : IrStd::SingletonImpl<ManagerEnvironment>
	{
	public:
		ManagerEnvironment();

	protected:
		friend class Manager;
		std::string m_outputDirectory;
	};

	class ManagerTrace : IrStd::SingletonImpl<ManagerTrace>
	{
	public:
		static void format(std::ostream&, const IrStd::Logger::Info&, const std::string&);

		// Save important traces
		struct TraceInfo
		{
			IrStd::Logger::Level m_level;
			const IrStd::TopicImpl* m_pTopic;
			std::string m_message;
		};
		typedef IrStd::Type::RingBufferSorted<IrStd::Type::Timestamp, TraceInfo, 32> RingBufferSorted;

	private:
		friend class Manager;
		RingBufferSorted m_traceInfoList;
	};

	class Manager
	{
	public:
		Manager(const int port = 8080, const std::string& outputDirectory = "");

		template<class T, class ... Args>
		Id registerExchange(Args&& ... args)
		{
			auto pExchange = std::shared_ptr<Exchange>(new T(std::forward<Args>(args)...));
			m_exchangeList.push_back(std::move(pExchange));
			return m_exchangeList.back()->getId();
		}

		template<class T, class ... Args>
		Id registerExchangeReadOnly(Args&& ... args)
		{
			auto pExchange = std::shared_ptr<Exchange>(new ExchangeReadOnly<T>(std::forward<Args>(args)...));
			m_exchangeList.push_back(std::move(pExchange));
			return m_exchangeList.back()->getId();
		}

		template<class T, class ... Args>
		Id registerExchangeMock(Args&& ... args)
		{
			auto pExchange = std::shared_ptr<Exchange>(new ExchangeMock<T>(std::forward<Args>(args)...));
			m_exchangeList.push_back(std::move(pExchange));
			return m_exchangeList.back()->getId();
		}

		template<class T>
		Id registerStrategy(const IrStd::Type::Gson::Map& config)
		{
			auto pStrategy = std::unique_ptr<Strategy>(new T(config));
			m_strategyList.push_back(std::move(pStrategy));
			return m_strategyList.back()->getId();
		}

		/**
		 * Loop through each exchanges
		 */
		void eachExchanges(std::function<void(Exchange&)> callback);

		/**
		 * Loop through each strategies
		 */
		void eachStrategies(std::function<void(Strategy&)> callback);

		/**
		 * Get a specific exchange
		 */
		Exchange& getExchange(const size_t index);
		const Exchange& getExchange(const size_t index) const;

		/**
		 * Get a specific strategy
		 */
		Strategy& getStrategy(const size_t index);
		const Strategy& getStrategy(const size_t index) const;

		Exchange& getLastExchange();

		const Trader::ManagerTrace::RingBufferSorted& getTraces() const noexcept;

		IrStd::Type::Timestamp getStartedTimestamp() const noexcept
		{
			return m_startedSince;
		}

		void start();

		static const std::string& getGlobalOutputDirectory();
		const std::string& getOutputDirectory() const noexcept;

	private:
		friend Server;

		Trader::Server m_server;
		std::vector<std::shared_ptr<Exchange>> m_exchangeList;
		std::vector<std::unique_ptr<Strategy>> m_strategyList;
		IrStd::Type::Timestamp m_startedSince;

		std::string m_outputDirectory;
	};
}
