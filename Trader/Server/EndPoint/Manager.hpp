#pragma once

#include "IrStd/IrStd.hpp"

#include "Trader/Manager/Manager.hpp"

namespace Trader
{
	namespace EndPoint
	{
		class Manager
		{
		public:
			Manager(Trader::Manager& trader)
					: m_trader(trader)
			{
			}

			void setup(IrStd::ServerREST& server);

			/**
			 * \brief Get traces of warning or higher level
			 *
			 * Endpoint: GET /api/v1/manager/traces
			 */
			void setupTraces(IrStd::ServerREST& server);

			/**
			 * \brief Get infromation about running threads
			 *
			 * Endpoint: GET /api/v1/manager/threads
			 */
			void setupThreads(IrStd::ServerREST& server);

		private:
			Trader::Manager& m_trader;
		};
	}
}
