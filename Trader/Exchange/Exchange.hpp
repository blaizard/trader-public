#pragma once

#include <mutex>
#include <iomanip>
#include <map>
#include <list>
#include <thread>
#include "IrStd/IrStd.hpp"

#include "Trader/Generic/Id/Id.hpp"
#include "Trader/Exchange/ConfigurationExchange.hpp"
#include "Trader/Exchange/Balance/Balance.hpp"
#include "Trader/Exchange/Currency/Currency.hpp"
#include "Trader/Exchange/Transaction/PairTransaction.hpp"
#include "Trader/Exchange/Transaction/PairTransactionMap.hpp"
#include "Trader/Exchange/Order/Order.hpp"
#include "Trader/Exchange/Order/TrackOrderList.hpp"
#include "Trader/Exchange/Event/EventManager.hpp"
#include "Trader/Exchange/Operation/Operation.hpp"

IRSTD_TOPIC_USE(Trader, Exchange);

namespace Trader
{
	class Exchange
	{
	public:
		Exchange(const Id id, ConfigurationExchange&& config = ConfigurationExchange());

		/**
		 * Return the identifier of the exchange
		 */
		Id getId() const noexcept;

		/**
		 * Set the output directory, this cannot be done when the exchange is connected
		 */
		void setOutputDirectory(const std::string& outputDirectory);

		/**
		 * \brief Identify an order chain to go from \b to to \b from
		 *
		 * \return the order chain in case of success, nullptr otherwise
		 */
		const std::shared_ptr<Order> identifyOrderChain(CurrencyPtr from, CurrencyPtr to) const noexcept;

		/**
		 * \brief Get the configuration of the exchange
		 */
		const ConfigurationExchange& getConfiguration() const noexcept;

		/**
		 * Return the current balance of the server
		 */
		const Balance& getBalance() const noexcept;

		/**
		 * Return the initial balance of the server
		 */
		const Balance& getInitialBalance() const noexcept;

		/**
		 * Return the pair transaction map
		 */
		PairTransactionMap& getTransactionMap() noexcept;
		const PairTransactionMap& getTransactionMap() const noexcept;

		/**
		 * Status fo the exchange
		 */
		enum class Status : size_t
		{
			DISCONNECTED = 0,
			DISCONNECTING = 1,
			CONNECTING = 2,
			CONNECTED = 3
		};

		/**
		 * Start and stop the exchange
		 */
		void start();
		void stop();
		Status getStatus() const noexcept;

		/**
		 * Get the number of availabel currencies
		 */
		size_t getNbCurrencies() const noexcept;

		/**
		 * Return a set containing th ecurrency list available with this exchange
		 */
		void getCurrencies(const std::function<void(const CurrencyPtr)>& callback) const noexcept;

		/**
		 * Return the amount of fund for a specific currency
		 */
		IrStd::Type::Decimal getFund(CurrencyPtr currency) const noexcept;

		/**
		 * Return the maxiumu amount that should be invested on this transaction
		 * if an opportunity is detected. This might be different from the available
		 * amount as it takes care of fund diversification.
		 */
		IrStd::Type::Decimal getAmountToInvest(const Transaction* pTransaction) const noexcept;

		/**
		 * Get the estimate in the estimate currency of the value of the balance
		 */
		IrStd::Type::Decimal getEstimate() const noexcept;

		/**
		 * Get the estimate in the estimate currency of the fund + reserve of a specific currency from the balance
		 */
		IrStd::Type::Decimal getEstimate(const CurrencyPtr currency) const noexcept;

		/**
		 * Get the estimate in the estimate currency of the amount a specific currency
		 */
		IrStd::Type::Decimal getEstimate(const CurrencyPtr currency, const IrStd::Type::Decimal amount) const noexcept;

		/**
		 * Return the currency used for the estimate
		 */
		CurrencyPtr getEstimateCurrency() const noexcept;

		/**
		 * \brief Process an operation
		 */
		void process(const Operation& operation, const size_t nbRetries, const std::string message);

		/**
		 * Print information about the rates
		 */
		void toStreamRates(std::ostream& out);

		/**
		 * Return the current timestamp of the server
		 * (an estimation based on the time deltas between the server and the local time)
		 */
		IrStd::Type::Timestamp getServerTimestamp() const noexcept;

		/**
		 * Set the server timestamp.
		 * This updates the server timestamp delta.
		 */
		void setServerTimestamp(const IrStd::Type::Timestamp timestamp) noexcept;

		/**
		 * Return the timestamp of the last time when the exchange connected to its server
		 */
		IrStd::Type::Timestamp getConnectedTimestamp() const noexcept;

		/**
		 * Available scopes
		 * \{
		 */
		mutable IrStd::RWLock m_lockProperties;
		mutable IrStd::RWLock m_lockRates;
		mutable IrStd::RWLock m_lockBalance;
		mutable IrStd::RWLock m_lockOrders;
		/// \}

		/**
		 * Return a reference to the event manager
		 */
		EventManager& getEventManager() noexcept;

		/**
		 * Return a reference to the track order list
		 */
		const TrackOrderList& getTrackOrderList() const noexcept;

		/**
		 * Get event instance
		 */
		const IrStd::Event& getEventRates() const noexcept;

		/**
		 * This function freeze the current thread until new rates are available
		 */
		bool waitForNewRates(const uint64_t timeoutMs = 0) const noexcept;

		// This function must be the only entry point to create a thread
		template<class Function, class ... Args>
		void createThread(const char* const pName, Function&& f, Args&& ... args)
		{
			std::lock_guard<std::mutex> lock(m_threadIdMapLock);
			// Make sure it does not exists already
			IRSTD_ASSERT(m_threadIdMap.find(pName) == m_threadIdMap.end(), "Thread '"
					<< pName << "' is already registered");
			// Create the name
			std::string name(getId());
			name += "::";
			name += pName;
			// Create the thread and register it
			const auto id = IrStd::Threads::create(name.c_str(),
					std::forward<Function>(f), std::forward<Args>(args)...);
			m_threadIdMap.insert({pName, id});
		}

		// Stop and delete a thread
		void terminateThread(const char* const pName, const bool mustExist = true);

		/**
		 * Make sure that the data make sense.
		 */
		void sanityCheck();

	private:
		void updatePropertiesStart();
		void updatePropertiesThread();

		void updateRatesStart();
		void updateRatesStop();
		void updateRatesThread();

		void updateBalanceAndOrdersStart();
		void updateBalanceAndOrdersThread();
		bool updateBalanceAndOrders();

		void ratesRecorderThread();

		/**
		 * The watchdog is used to ensure that events are regularly triggered.
		 * It is made to detect potential deadlocks or exchange server issues.
		 *
		 * \{
		 */
		void watchdogStart();
		void watchdogStop();
		void watchdogThread();
		/// \}

		/**
		 * Return the currency that is the most appropriate for estimates
		 */
		CurrencyPtr identifyEstimateCurrency() const;

		/**
		 * Build the order chain map
		 */
		bool buildOrderChainMap();

		/**
		 * \breif Update all transactiosn minimal amounts based on minimal
		 * amounts of known currencies
		 */
		void updateTransactionsMinimalAmount();

		/**
		 * Add a job to the list
		 */
		void addJob(const std::function<void()>& job);

		/**
		 * Add a job to the list
		 */
		void waitForAllJobsToBeCompleted();

		/**
		 * Private part of the process function. This cannot be called externally.
		 */
		void process(const Operation& operation, const std::string message);

		/**
		 * Generate a unique Id for the Exchange
		 */
		Id generateUniqueId(const Id type);

	protected:
		friend EventManager;

		/**
		 * Virtual functions to be overwritten by the implementation
		 */
		virtual void updatePropertiesImpl(PairTransactionMap& transactionMap) = 0;
		virtual void updateRatesStartImpl();
		virtual void updateRatesStopImpl();
		virtual void updateRatesImpl();
		virtual void updateRatesSpecificCurrencyImpl(const CurrencyPtr currency);
		/**
		 * \brief Enumerates all pairs.
		 * \note Only non-inverted pairs are called.
		 */
		virtual void updateRatesSpecificPairImpl(const CurrencyPtr currency1, const CurrencyPtr currency2);

		virtual void updateBalanceImpl(Balance& balance) = 0;
		virtual void updateOrdersImpl(std::vector<TrackOrder>& trackOrders) = 0;
		/**
		 * Return the amount of the order immediatly processed
		 */
		virtual void setOrderImpl(const Order& order, const IrStd::Type::Decimal amount, std::vector<Id>& idList) = 0;
		virtual void cancelOrderImpl(const TrackOrder& order) = 0;
		virtual void withdrawImpl(const CurrencyPtr /*currency*/, const IrStd::Type::Decimal /*amount*/)
		{
			IRSTD_UNREACHABLE(IRSTD_TOPIC(Trader, Exchange));
		}

		/**
		 * Connect/Disconnect function
		 */
		void connect();
		void disconnect();
		std::mutex m_connectMutex;

		/**
		  * Reset the state of the exchange
		  */
		void reset();

		Id m_id;
		Status m_status;

		/**
		 * \brief Connected until this timestamp
		 */
		IrStd::Type::Timestamp m_connectedTimestamp;

		/**
		 * This currency will be used for estimates
		 */
		CurrencyPtr m_estimateCurrency;

		// Record the status of the initial balance
		Balance m_balance;
		Balance m_initialBalance;
		std::set<CurrencyPtr> m_currencyList;

		// Configuration
		ConfigurationExchange m_configuration;

		// Various events
		IrStd::Event m_eventProperties;
		IrStd::Event m_eventRates;
		IrStd::Event m_eventOrders;
		IrStd::Event m_eventBalance;
		IrStd::Event m_eventUpdateBalanceAndOrders;

		/**
		 * Transaction pair map
		 * \{
		 */
		PairTransactionMap m_transactionMap;
		/// \}

		// Order chains
		std::map<CurrencyPtr, std::map<CurrencyPtr, std::shared_ptr<Order>>> m_orderChainMap;

		// Ids of registered threads
		std::map<const char*, std::thread::id> m_threadIdMap;
		std::mutex m_threadIdMapLock;

		// Difference of the server time and the local time
		int64_t m_timestampDelta;

		// Event Manager
		EventManager m_eventManager;

		// Active order list
		TrackOrderList m_orderTrackList;
	};
}
