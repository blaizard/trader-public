#pragma once

#include <list>

#include "Trader/Exchange/Exchange.hpp"
#include "Trader/Exchange/Order/Order.hpp"
#include "Trader/Generic/Id/Id.hpp"

#include "Trader/Strategy/ConfigurationStrategy.hpp"

namespace Trader
{
	namespace EndPoint
	{
		class Strategy;
	}

	class Strategy
	{
	private:
		/**
		 * \brief Defines the number of previous operation updates to be recorded
		 */
		static constexpr size_t NB_RECORDS = 256;

	public:
		Strategy(const Id type, ConfigurationStrategy&& config = ConfigurationStrategy());

		/**
		 * \brief return the type of the strategy
		 */
		Id getType() const noexcept;

		/**
		 * \brief return the unique identifier of the strategy
		 */
		Id getId() const noexcept;

		virtual void initializeImpl() = 0;

		/**
		 * Process the strategy
		 *
		 * \param counter Number of time this function has been
		 *                called since its initialization
		 */
		virtual void processImpl(const size_t counter) = 0;

		/**
		 * Return specific data to this strategy
		 */
		virtual const IrStd::Json& getJson() const;

		/**
		 * \brief Place a long position or sell operation
		 *
		 * To create a limit order, set the rate of the order to a specific value,
		 * for a market order, leave it untouched.
		 */
		OperationContextHandle sell(Exchange& exchange, const Order& order, const IrStd::Type::Decimal amount,
				const size_t nbRetries = 0);

		/**
		 * \brief Withdraw an amount of a specific currency
		 */
		OperationContextHandle withdraw(Exchange& exchange, const CurrencyPtr currency, const IrStd::Type::Decimal amount,
				const size_t nbRetries = 0);

		/**
		 * Get the list of exchange Ids associated with this strategy
		 * Note, it can through to make sure exceptions thrown from the callback can be caught
		 */
		void eachExchanges(const std::function<void(const Id)>& callback) const;

		/**
		 * Return the instance of the exchange
		 */
		Exchange& getExchange(const Id id) noexcept;

		/**
		 * Return the exchange associated with the strategy
		 * note, this will work only if a single exchange is registered
		 */
		Exchange& getExchange() noexcept;

		/**
		 * Return the configuration associated with this strategy
		 */
		const ConfigurationStrategy& getConfiguration() const noexcept;

		/**
		 * Get the processign time in percent of this strategy
		 */
		double getProcessTimePercent() const noexcept;

		/**
		 * Returns records related to operations
		 */
		void getRecordedOperations(const std::function<void(const IrStd::Type::Timestamp, const char* const,
				const size_t, const IrStd::Type::Decimal, const char* const, const char* const)>& callback) const noexcept;

	private:
		friend class Manager;
		friend class EndPoint::Strategy;

		IrStd::Type::Stopwatch m_totalTime;
		IrStd::Type::Stopwatch::Counter m_processTime;

		/**
		 * Setup the strategy and assing the exchanges
		 */
		void setup(std::vector<std::shared_ptr<Exchange>>& exchangeList);
		void initialize();
		void process(const size_t counter);

		/**
		 * \brief Generate an id from the type
		 */
		Id generateUniqueId(const Id type);

		/**
		 * Get the list of exchanges associated with this strategy
		 */
		void getExchangeList(const std::function<void(const Exchange& exchange)>& callback) const;

		/**
		 * Return the profit made by this strategy
		 */
		void getProfitInJson(IrStd::Json& json) const;

		enum class OperationStatus
		{
			SUCCESS,
			TIMEOUT,
			FAILED
		};
		void recordOperation(const OperationStatus type, const OperationContext& context,
				const IrStd::Type::Decimal profit, const CurrencyPtr currency) noexcept;

		/**
		 * Callback used to monitor the profit from the strategies
		 */
		static void monitorProfit(const Strategy& strategy, const size_t contextId,
				const CurrencyPtr currency, const IrStd::Type::Decimal profit,
				const CurrencyPtr currencyEstimate, const IrStd::Type::Decimal profitEstimate);

		// Strategy identifier
		Id m_type;
		Id m_id;
		ConfigurationStrategy m_configuration;

		/**
		 * Statuses of the strategy
		 */
		enum class Status : size_t
		{
			UNINITIALIZED = 0,
			INITIALIZED = 1
		};
		Status m_status;

		/**
		 * Structure coresponding to the exchange
		 */
		struct ExchangeInfo
		{
			explicit ExchangeInfo(std::shared_ptr<Trader::Exchange> pExchange)
					: m_pExchange(pExchange)
					, m_profit()
					, m_nbSuccess(0)
					, m_nbFailedTimeout(0)
					, m_nbFailedPlaceOrder(0)
			{
			}

			std::shared_ptr<Trader::Exchange> m_pExchange;
			Balance m_profit;
			/// Successfull orders
			size_t m_nbSuccess;
			/// Failed because of timeout issue (cannot match the strategy)
			size_t m_nbFailedTimeout;
			/// Failed because of server issue (cannot place the order)
			size_t m_nbFailedPlaceOrder;
		};
		std::map<Id, ExchangeInfo> m_exchangeList;

		// To record latests operations
		struct OperationState
		{
			OperationStatus m_status;
			size_t m_id;
			IrStd::Type::Decimal m_profit;
			CurrencyPtr m_currency;
			std::string m_description;
		};
		IrStd::Type::RingBufferSorted<IrStd::Type::Timestamp, OperationState, NB_RECORDS> m_operationRecordList;
	};
}
