#pragma once

#include "IrStd/IrStd.hpp"

#include "Trader/Manager/Manager.hpp"

namespace Trader
{
	namespace EndPoint
	{
		class Strategy
		{
		public:
			Strategy(Trader::Manager& trader)
					: m_trader(trader)
			{
			}

			void setup(IrStd::ServerREST& server);

			/**
			 * \brief Get information about the strategies
			 *
			 * Endpoint: GET /api/v1/strategy/list
			 * Response: json
			 * {
			 *     list: [
			 *         {
			 *             name: "Dummy" // name of the strategy
			 *         }
			 *     ]
			 * }
			 */
			void setupList(IrStd::ServerREST& server);

			/**
			 * \brief Return the current status of the strategy
			 *
			 * Endpoint: GET /api/v1/strategy/{UINT}/status
			 * Response: json
			 * {
			 * }
			 */
			void setupStatus(IrStd::ServerREST& server);

			/**
			 * \brief Get the profit registered by a specific strategy
			 *
			 * Endpoint: GET /api/v1/strategy/{UINT}/profit
			 * Response: json
			 * {
			 * }
			 */
			void setupProfit(IrStd::ServerREST& server);

			/**
			 * \brief Returns configuration of the strategy
			 *
			 * Endpoint: GET /api/v1/strategy/{UINT}/configuration
			 * Response: json
			 * {
			 * }
			 */
			void setupConfiguration(IrStd::ServerREST& server);

			/**
			 * \brief Returns specific information about the strategy
			 *
			 * Endpoint: GET /api/v1/strategy/{UINT}/data
			 * Response: json
			 * {
			 * }
			 */
			void setupData(IrStd::ServerREST& server);

			/**
			 * \brief Get the list of the last operations
			 *
			 * Endpoint: GET /api/v1/strategy/{UINT}/operations
			 * Response: json
			 * {
			 *     // Operations
			 * }
			 */
			void setupOperations(IrStd::ServerREST& server);

		private:
			Trader::Manager& m_trader;
		};
	}
}
