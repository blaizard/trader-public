#pragma once

#include "IrStd/IrStd.hpp"

#include "Trader/Manager/Manager.hpp"

namespace Trader
{
	namespace EndPoint
	{
		class Exchange
		{
		public:
			Exchange(Trader::Manager& trader)
					: m_trader(trader)
			{
			}

			void setup(IrStd::ServerREST& server);

			/**
			 * \brief Get information about the exchanges
			 *
			 * Endpoint: GET /api/v1/exchange/list
			 * Response: json
			 * {
			 *     list: [
			 *         {
			 *             name: "BTC-E#0" // name of the exchange
			 *             status: 1       // status of the exchange
			 *         }
			 *     ]
			 * }
			 */
			void setupList(IrStd::ServerREST& server);

			/**
			 * \brief Get configuration of a specific exchange
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/configuration
			 * Response: json
			 * {
			 *     // List of parameters
			 * }
			 */
			void setupConfiguration(IrStd::ServerREST& server);

			/**
			 * \brief Get the balance of a specific exchange
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/balance
			 * Response: json
			 * {
			 *     // List of currency and their amount
			 * }
			 */
			void setupBalance(IrStd::ServerREST& server);

			/**
			 * \brief Get the initial balance of a specific exchange
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/initial/balance
			 * Response: json
			 * {
			 *     // List of currency and their amount
			 * }
			 */
			void setupInitialBalance(IrStd::ServerREST& server);

			/**
			 * \brief Get the latest 1 min rates for a specific pair of a specific exchange
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/rates/{STRING}/{STRING}
			 * or        GET /api/v1/exchange/{UINT}/rates/{STRING}/{STRING}/{UINT}/{UINT}
			 * Response: json
			 * {
			 *     // Rates and timestamps
			 * }
			 */
			void setupRates(IrStd::ServerREST& server);

			/**
			 * \brief Get the list of currencies supported by the exchange
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/currencies
			 * Response: json
			 * {
			 *     // Rates and timestamps
			 * }
			 */
			void setupCurrencies(IrStd::ServerREST& server);

			/**
			 * \brief Get the list of transactions supported by the exchange
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/transactions
			 * Response: json
			 * {
			 *     // Rates and timestamps
			 * }
			 */
			void setupTransactions(IrStd::ServerREST& server);

			/**
			 * \brief Get the list of active orders
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/orders/active
			 */
			void setupActiveOrders(IrStd::ServerREST& server);

			/**
			 * \brief Get the list of the last orders
			 *
			 * Endpoint: GET /api/v1/exchange/{UINT}/orders
			 * Response: json
			 * {
			 *     // Orders
			 * }
			 */
			void setupOrders(IrStd::ServerREST& server);

		private:
			Trader::Manager& m_trader;
		};
	}
}
