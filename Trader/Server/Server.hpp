#pragma once

#include <thread>

#include "IrStd/IrStd.hpp"

namespace Trader
{
	class Manager;
	class Server
	{
	public:
		Server(Manager& trader, const int port);

		void start();
		void stop();

	private:
		void serverThread();

		class ServerREST : public IrStd::ServerREST
		{
		public:
			ServerREST(const int port);
			void routeNotFound(IrStd::ServerHTTP::Context& context) override;

		private:
			friend Trader::Server;
			std::string m_rootPath;
		};

		ServerREST m_server;
		std::thread::id m_threadId;
		Manager& m_trader;
	};
}
