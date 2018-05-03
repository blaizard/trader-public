#include <iomanip>
#include <algorithm>
#include <fstream>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Exchange.hpp"

IRSTD_TOPIC_REGISTER(Trader, Exchange);
IRSTD_TOPIC_REGISTER(Trader, Exchange, Mock);
IRSTD_TOPIC_USE_ALIAS(TraderExchange, Trader, Exchange);

namespace
{
	struct KnownCurrency
	{
		Trader::CurrencyPtr m_currency;
		IrStd::Type::Decimal m_minAmount;
	};
	const KnownCurrency knownCurrencies[] = {
		{Trader::Currency::USD, 1},
		{Trader::Currency::EUR, 1},
		{Trader::Currency::BTC, 0.0002}
	};
}

// ---- Trader::Exchange ------------------------------------------------------

Trader::Exchange::Exchange(
		const Id id,
		ConfigurationExchange&& config)
		: m_id(generateUniqueId(id))
		, m_status(Status::DISCONNECTED)
		, m_connectedTimestamp(0)
		, m_estimateCurrency(Currency::USD)
		, m_balance(*this)
		, m_initialBalance(*this)
		, m_configuration(std::move(config))
		, m_eventProperties("PropertiesTrigger")
		, m_eventRates("RatesTrigger")
		, m_eventOrders("OrdersTrigger")
		, m_eventBalance("BalanceTrigger")
		, m_eventUpdateBalanceAndOrders("Balance&OrdersTrigger")
		, m_timestampDelta(0)
		, m_eventManager()
		, m_orderTrackList(m_eventManager, m_configuration.getOrderRegisterTimeoutMs())
{
}

Trader::Id Trader::Exchange::generateUniqueId(const Id type)
{
	static std::mutex mutex;
	static std::map<Id, size_t> typeIdCounter;
	std::lock_guard<std::mutex> lock(mutex);

	if (typeIdCounter.find(type) == typeIdCounter.end())
	{
		typeIdCounter[type] = 0;
		return type;
	}

	typeIdCounter[type]++;
	auto str = std::string(type.c_str());
	str += '-';
	str += IrStd::Type::ShortString(typeIdCounter[type]);
	return Id(str.c_str());
}

Trader::Id Trader::Exchange::getId() const noexcept
{
	return m_id;
}

void Trader::Exchange::setOutputDirectory(const std::string& outputDirectory)
{
	IRSTD_ASSERT(TraderExchange, m_status == Status::DISCONNECTED,
			getId() << ": the output directory can be changed only when disconnected, m_status="
			<< IrStd::Type::toIntegral(m_status));

	// Set the directory
	m_configuration.setOutputDirectory(outputDirectory);
}

void Trader::Exchange::terminateThread(const char* const pName, const bool mustExist)
{
	std::thread::id id;
	{
		std::lock_guard<std::mutex> lock(m_threadIdMapLock);
		const auto it = m_threadIdMap.find(pName);
		if (it == m_threadIdMap.end())
		{
			if (mustExist)
			{
				IRSTD_LOG_FATAL(TraderExchange, getId() << ": thread '" << pName
						<< "' is not registered, but it should be");
			}
			return;
		}
		id = it->second;
		m_threadIdMap.erase(it);
	}

	IRSTD_LOG_TRACE(TraderExchange, getId() << ": terminating " << id);
	IrStd::Threads::terminate(id);
}

Trader::EventManager& Trader::Exchange::getEventManager() noexcept
{
	return m_eventManager;
}

const Trader::TrackOrderList& Trader::Exchange::getTrackOrderList() const noexcept
{
	return m_orderTrackList;
}

const Trader::ConfigurationExchange& Trader::Exchange::getConfiguration() const noexcept
{
	return m_configuration;
}

const Trader::Balance& Trader::Exchange::getBalance() const noexcept
{
	return m_balance;
}

const Trader::Balance& Trader::Exchange::getInitialBalance() const noexcept
{
	return m_initialBalance;
}

// ---- Trader::Exchange (events) ---------------------------------------------

const IrStd::Event& Trader::Exchange::getEventRates() const noexcept
{
	return m_eventRates;
}

bool Trader::Exchange::waitForNewRates(const uint64_t timeoutMs) const noexcept
{
	return m_eventRates.waitForNext(timeoutMs);
}

// ---- Trader::Exchange (rates recording) ------------------------------------

void Trader::Exchange::ratesRecorderThread()
{
	IrStd::Type::Timestamp previousTimestamp = 0;

	// Set the output directory
	std::string outputDirectoryRoot(m_configuration.getOutputDirectory());
	IrStd::FileSystem::mkdir(outputDirectoryRoot);

	while (IrStd::Threads::isActive())
	{
		IrStd::Threads::setIdle();
		// Makes sure there are enough (but not too many) new rates to be
		// filled. This will ensure that data aer not missed
		for (size_t counter = 0; counter<Trader::Transaction::NB_RECORDS / 2
				&& IrStd::Threads::isActive(); counter++)
		{
			waitForNewRates(/*timeoutMs*/4000);
		}
		IrStd::Threads::setActive();

		if (!IrStd::Threads::isActive())
		{
			return;
		}

		IRSTD_LOG_TRACE(TraderExchange, getId() << ": recording rates for " << getId());

		const auto currentTimestamp = IrStd::Type::Timestamp::now();

		// Loop through all existing transactions
		getTransactionMap().getTransactions([&](const CurrencyPtr, const CurrencyPtr,
				const PairTransactionMap::PairTransactionPointer pTransaction) {
			// Generate the filename
			std::string filePath = outputDirectoryRoot;
			filePath += "/pair-";
			filePath += pTransaction->getInitialCurrency()->getId();
			filePath += "-";
			filePath += pTransaction->getFinalCurrency()->getId();
			filePath += ".csv";
			// Open the file and populate it with the data
			{
				IrStd::FileSystem::FileCsv file(filePath);
				IrStd::Type::Decimal previousRate = -1;
				size_t nbRecordings = 0;
				const bool isComplete = pTransaction->getRates(previousTimestamp + 1, currentTimestamp, [&](
						const IrStd::Type::Timestamp timestamp,
						const IrStd::Type::Decimal rate)
				{
					// Only write values when they are different, to prevent
					// writing useless information
					if (previousRate != rate)
					{
						file.write(static_cast<uint64_t>(timestamp), IrStd::Type::ShortString(rate, pTransaction->getDecimalPlace()));
						previousRate = rate;
						nbRecordings++;
					}
				});
				if (previousTimestamp && isComplete == false)
				{
					IRSTD_LOG_ERROR(TraderExchange, getId() << ": circular buffer looped before all rates were recorded in "
							<< filePath << ", expected data loss. Recording within ]" << previousTimestamp
							<< ", " << currentTimestamp << "]: " << nbRecordings << " data point(s).");
				}
			}
		});

		previousTimestamp = currentTimestamp;
	}
}

// ---- Trader::Exchange::*transaction ----------------------------------------

Trader::PairTransactionMap& Trader::Exchange::getTransactionMap() noexcept
{
	return m_transactionMap;
}

const Trader::PairTransactionMap& Trader::Exchange::getTransactionMap() const noexcept
{
	return m_transactionMap;
}

bool Trader::Exchange::buildOrderChainMap()
{
	IRSTD_ASSERT(TraderExchange, m_lockProperties.isWriteScope(),
			getId() << ": Exchange::buildOrderChainMap must be called under the properties write scope");

	// Cleanup the map
	m_orderChainMap.clear();

	std::function<size_t(std::shared_ptr<Order>&, CurrencyPtr, CurrencyPtr, std::set<CurrencyPtr>&, const size_t)>
		identifyOrderChainRec = [&](std::shared_ptr<Order>& pOrder, CurrencyPtr currency, CurrencyPtr finalCurrency, std::set<CurrencyPtr>& ignoredCurrencies, const size_t depth) {

			// Check if the transaction exists
			{
				const auto pTransaction = getTransactionMap().getTransaction(currency, finalCurrency);
				if (pTransaction)
				{
					pOrder = std::make_shared<Order>(pTransaction);
					return depth;
				}
			}

			// Add current currency in the ignored list
			ignoredCurrencies.insert(currency);

			size_t minDepth = std::numeric_limits<size_t>::max();

			getTransactionMap().getTransactions(currency, [&](const CurrencyPtr newCurrency, const PairTransactionMap::PairTransactionPointer pTransaction) {
				// Ignore if level is reached or if the currency is within the excluded ones
				if (ignoredCurrencies.find(newCurrency) != ignoredCurrencies.end())
				{
					return;
				}

				{
					// Recursive call
					std::shared_ptr<Order> pRemaingOrder;
					const size_t curDepth = identifyOrderChainRec(pRemaingOrder, newCurrency, finalCurrency, ignoredCurrencies, depth + 1);

					if (pRemaingOrder && curDepth < minDepth)
					{
						pOrder = std::make_shared<Order>(pTransaction);
						pOrder->addNext(std::move(*pRemaingOrder));

						minDepth = curDepth;
					}
				}
			});

			// Remove the current currency from the ignored list
			IRSTD_ASSERT(TraderExchange, ignoredCurrencies.erase(currency) == 1, getId() << ": error while erasing the currency");

			return minDepth;
	};

	size_t nbOrderChains = 0;
	std::stringstream missingStream;
	bool isAnyMissing = false;
	for (auto currency1 : m_currencyList)
	{
		m_orderChainMap[currency1] = std::map<CurrencyPtr, std::shared_ptr<Order>>();
		for (auto currency2 : m_currencyList)
		{
			// If same currency, for simplicity create a NOP transaction for the order
			if (currency1 == currency2)
			{
				auto pOrder = std::make_shared<Order>(PairTransactionNop::getInstance().get(currency1));
				m_orderChainMap[currency1][currency2] = pOrder;
				++nbOrderChains;
				continue;
			}

			std::shared_ptr<Order> pOrder;
			std::set<CurrencyPtr> ignoredCurrencies;
			identifyOrderChainRec(pOrder, currency1, currency2, ignoredCurrencies, 1);
			if (pOrder)
			{
				m_orderChainMap[currency1][currency2] = pOrder;
				++nbOrderChains;
			}
			else
			{
				missingStream << ((isAnyMissing) ? ", " : "") << currency1 << "/" << currency2;
				isAnyMissing = true;
			}
		}
	}

	IRSTD_LOG_INFO(TraderExchange, "Pre-built " << nbOrderChains << " order chains for " << getId());

	if (isAnyMissing)
	{
		IRSTD_LOG_WARNING(TraderExchange, "Missing order chains for the following pair(s): " << missingStream.str());
		return false;
	}

	return true;
}

const std::shared_ptr<Trader::Order> Trader::Exchange::identifyOrderChain(
		CurrencyPtr initialCurrency,
		CurrencyPtr finalCurrency) const noexcept
{
	auto scope = m_lockProperties.readScope();

	const auto it1 = m_orderChainMap.find(initialCurrency);
	if (it1 != m_orderChainMap.end())
	{
		const auto it2 = it1->second.find(finalCurrency);
		if (it2 != it1->second.end())
		{
			return it2->second;
		}
	}
	return std::shared_ptr<Order>();
}

// ---- Trader::Exchange (currency) -------------------------------------------

Trader::CurrencyPtr Trader::Exchange::getEstimateCurrency() const noexcept
{
	return m_estimateCurrency;
}

size_t Trader::Exchange::getNbCurrencies() const noexcept
{
	auto scope = m_lockProperties.readScope();
	return m_currencyList.size();
}

void Trader::Exchange::getCurrencies(const std::function<void(const CurrencyPtr)>& callback) const noexcept
{
	auto scope = m_lockProperties.readScope();
	for (const auto& currency : m_currencyList)
	{
		callback(currency);
	}
}

IrStd::Type::Decimal Trader::Exchange::getEstimate() const noexcept
{
	auto scope = m_lockProperties.readScope();
	return m_balance.estimate(this);
}

IrStd::Type::Decimal Trader::Exchange::getEstimate(const CurrencyPtr currency) const noexcept
{
	auto scope = m_lockProperties.readScope();
	return m_balance.estimate(currency, m_balance.getWithReserve(currency), this);
}

IrStd::Type::Decimal Trader::Exchange::getEstimate(
		const CurrencyPtr currency,
		const IrStd::Type::Decimal amount) const noexcept
{
	auto scope = m_lockProperties.readScope();
	return m_balance.estimate(currency, amount, this);
}

IrStd::Type::Decimal Trader::Exchange::getFund(CurrencyPtr currency) const noexcept
{
	return m_balance.get(currency);
}

IrStd::Type::Decimal Trader::Exchange::getAmountToInvest(const Transaction* pTransaction) const noexcept
{
	auto scope = m_lockBalance.readScope();
	{
		const auto currency = pTransaction->getInitialCurrency();
		const auto availableAmount = getFund(currency);

		// If the transaction is invalid, return 0
		if (!pTransaction->isValid(availableAmount, pTransaction->getRate()))
		{
			return 0.;
		}

		// If order diversification is not set, return the maxium amount
		if (!m_configuration.isOrderDiversification())
		{
			return availableAmount;
		}

		// Forbid transaction between 2 fiat currencies
		if (pTransaction->getInitialCurrency()->isFiat() && pTransaction->getFinalCurrency()->isFiat())
		{
			return false;
		}

		// Check if this transaction is already active, if this is the case, return 0
		// Also check if the currency we intent to buy is already part of an active order
		{
			bool isTransactionActive = false;
			bool isCurrencyUsed = false;
			const auto finalCurrency = pTransaction->getFinalCurrency();
			getTrackOrderList().each([&](const TrackOrder& trackOrder) {
				const auto* pActiveTransaction = trackOrder.getOrder().getTransaction();
				if (pTransaction == pActiveTransaction)
				{
					isTransactionActive = true;
				}
				if ((currency == pActiveTransaction->getInitialCurrency() && finalCurrency == pActiveTransaction->getFinalCurrency())
						|| (currency == pActiveTransaction->getFinalCurrency() && finalCurrency == pActiveTransaction->getInitialCurrency()))
				{
					isCurrencyUsed = true;
				}
			});
			if (isTransactionActive || isCurrencyUsed)
			{
				return 0.;
			}
		}

		// Diversify the amount
		{
			const auto diversityFactor = std::min(6., std::ceil(m_currencyList.size() / 2.));
			const auto totalEstimate = m_balance.estimate(this);
			const auto amountEstimate = m_balance.estimate(currency, availableAmount, this);
			const auto maxAmountEstimate = totalEstimate / diversityFactor;

			if (amountEstimate > maxAmountEstimate)
			{
				return availableAmount * (maxAmountEstimate / amountEstimate);
			}

			return availableAmount;
		}
	}

	return 0.;
}

// ---- Trader::Exchange::start & stop ----------------------------------------

void Trader::Exchange::start()
{
	IRSTD_ASSERT(m_status == Status::DISCONNECTED);
	watchdogStart();
}

void Trader::Exchange::stop()
{
	disconnect();
	watchdogStop();
	IRSTD_ASSERT(m_status == Status::DISCONNECTED);
}

Trader::Exchange::Status Trader::Exchange::getStatus() const noexcept
{
	return m_status;
}

// ---- Trader::Exchange::connect & disconnect & reset ------------------------

void Trader::Exchange::reset()
{
	// Reset all events
	m_eventRates.reset();
	m_eventProperties.reset();
	m_eventOrders.reset();
	m_eventBalance.reset();
	m_eventUpdateBalanceAndOrders.reset();

	{
		auto scope = m_lockOrders.writeScope();
		m_orderTrackList.initialize(/*keepOrders*/true);
	}

	{
		auto scope = m_lockProperties.writeScope();
		m_transactionMap.clear();
		m_currencyList.clear();
		m_orderChainMap.clear();
	}

	{
		auto scope = m_lockBalance.writeScope();
		m_balance.clear();
	}
}

void Trader::Exchange::connect()
{
	constexpr size_t CONNECTION_TIMEOUT_S = 120;

	std::lock_guard<std::mutex> lock(m_connectMutex);

	// Reset everything
	reset();

	// Set the status
	m_status = Status::CONNECTING;

	IRSTD_LOG_INFO(TraderExchange, "Connecting to " << getId()
			<< " with configuration " << m_configuration);

	// Read properties before anything else
	updatePropertiesStart();
	if (!m_eventProperties.waitForAtLeast(1, CONNECTION_TIMEOUT_S * 1000))
	{
		IRSTD_THROW(TraderExchange, "Detected no activity on " << getId()
				<< ", timeout exceeded " << CONNECTION_TIMEOUT_S << "s, abort.");
	}

	updateRatesStart();
	if (!m_eventRates.waitForAtLeast(1, CONNECTION_TIMEOUT_S * 1000))
	{
		IRSTD_THROW(TraderExchange, "Detected no activity on " << getId()
				<< ", timeout exceeded " << CONNECTION_TIMEOUT_S << "s, abort.");
	}
	// Wait for all rates to be set
	{
		getTransactionMap().getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2, const PairTransactionMap::PairTransactionPointer pTransaction) {
			size_t timeout_s = CONNECTION_TIMEOUT_S;
			while (!pTransaction->getNbRates() && timeout_s)
			{
				IrStd::Threads::sleep(1000);
				--timeout_s;
			}
			if (!pTransaction->getNbRates())
			{
				IRSTD_THROW(TraderExchange, "Rate " << currency1 << "/" << currency2 << " for " << getId()
						<< " was not updated within " << CONNECTION_TIMEOUT_S << "s, abort.");
			}
		});
	}

	// Switch on the rates recorder if any
	if (m_configuration.isRatesRecording())
	{
		createThread("RatesRecorder", &Exchange::ratesRecorderThread, this);
	}

	// Identify the prefered currency for estimates
	{
		m_estimateCurrency = identifyEstimateCurrency();
		IRSTD_LOG_INFO(TraderExchange, getId() << ": identified " << m_estimateCurrency
				<< " as the currency used for estimates");
	}

	// Set the minimal amount for all transactions
	updateTransactionsMinimalAmount();

	// Only if orders are enable
	if (!m_configuration.isReadOnly())
	{
		updateBalanceAndOrdersStart();

		// Wait for the first data to be available
		for (auto& event : {std::ref(m_eventOrders), std::ref(m_eventBalance)})
		{
			if (!event.get().waitForAtLeast(1, CONNECTION_TIMEOUT_S * 1000))
			{
				IRSTD_THROW(TraderExchange, "Connection to " << getId()
						<< " exceeded " << CONNECTION_TIMEOUT_S << "s (timeout), abort.");
			}
		}
	}

	// Make sure that the data make sense
	try
	{
		sanityCheck();
	}
	catch (const IrStd::Exception& e)
	{
		IRSTD_LOG_ERROR(TraderExchange, "Sanity check failed for " << getId() << ": " << e);
	}

	// Print
	toStreamRates(std::cout);

	// Set connected status
	m_connectedTimestamp = IrStd::Type::Timestamp::now();
	m_status = Status::CONNECTED;
}

void Trader::Exchange::disconnect()
{
	std::lock_guard<std::mutex> lock(m_connectMutex);

	m_status = Status::DISCONNECTING;

	IRSTD_LOG_INFO(TraderExchange, "Disconnecting from " << getId() << "...");

	// Wait until all jobs are done
	waitForAllJobsToBeCompleted();

	// Stop services
	updateRatesStop();

	// Terminate all registered threads except watchdog
	{
		std::vector<const char*> threadIdList
		;
		{
			std::lock_guard<std::mutex> lock(m_threadIdMapLock);
			for (const auto& it : m_threadIdMap)
			{
				if (strcmp(it.first, "Watchdog"))
				{
					threadIdList.push_back(it.first);
				}
			}
		}
		// Terminate them all
		for (const auto idStr : threadIdList)
		{
			terminateThread(idStr);
		}
	}

	IRSTD_LOG_INFO(TraderExchange, getId() << ": disconnected");

	m_status = Status::DISCONNECTED;
}

void Trader::Exchange::updateTransactionsMinimalAmount()
{
	std::map<CurrencyPtr, IrStd::Type::Decimal> minAmountMap;

	getCurrencies([&](const CurrencyPtr currency) {
		minAmountMap[currency] = 0;
		for (const auto& known : ::knownCurrencies)
		{
			const auto order = identifyOrderChain(known.m_currency, currency);
			if (order)
			{
				const auto minAmount = order->getFinalAmount(known.m_minAmount, /*includeFee*/false); 
				minAmountMap[currency] = minAmount;
				break;
			}
		}
		// Display the minimal amount proceed for each currency
		if (minAmountMap[currency])
		{
			IRSTD_LOG_TRACE(TraderExchange, getId() << ": minimal amount for " << currency << " is " << minAmountMap[currency]);
		}
		else
		{
			IRSTD_LOG_WARNING(TraderExchange, getId() << ": no minimal amount identified for the currency " << currency);
		}
	});

	// Set the minimal amounts
	{
		auto scope = m_lockProperties.writeScope();

		// Loop through all transactions
		getTransactionMap().getTransactions([&](const CurrencyPtr currencyFrom, const CurrencyPtr currencyTo) {
			auto pTransaction = getTransactionMap().getTransactionForWrite(currencyFrom, currencyTo);
			auto pBoundaries = pTransaction->getBoundariesForWrite();
			if (pBoundaries)
			{
				pBoundaries->setInitialAmount(minAmountMap[currencyFrom]);
				pBoundaries->setFinalAmount(minAmountMap[currencyTo]);
			}
		});
	}
}

Trader::CurrencyPtr Trader::Exchange::identifyEstimateCurrency() const
{
	std::map<CurrencyPtr, size_t> distanceList;
	std::pair<CurrencyPtr, size_t> maxDistance{Currency::USD, 0};

	getCurrencies([&](const CurrencyPtr currency1) {
		// Count how many transactions are possible to this currency
		size_t distance = 0;
		getCurrencies([&](const CurrencyPtr currency2) {
			if (identifyOrderChain(currency1, currency2))
			{
				distance++;
			}
		});
		distanceList[currency1] = distance;
		if (distance > maxDistance.second)
		{
			maxDistance = std::make_pair(currency1, distance);
		}
	});

	// Check first from the prefered currencies
	for (const auto known : ::knownCurrencies)
	{
		if (distanceList[known.m_currency] == maxDistance.second)
		{
			return known.m_currency;
		}
	}

	// If no match return the one that has the most matches (highest distance)
	return maxDistance.first;
}

void Trader::Exchange::sanityCheck()
{
	// Check some known rates and make sure the spread is within the limit
	{
		constexpr double ACCEPTABLE_VARIATION = 0.5; // 50%
		struct KnownRates
		{
			const CurrencyPtr m_currency1;
			const CurrencyPtr m_currency2;
			const IrStd::Type::Decimal m_approximatedRate;
		};

		for (const auto& currencyPair : {
				KnownRates{Currency::USD, Currency::EUR, 0.85}
			})
		{
			const auto pOrderChain = identifyOrderChain(currencyPair.m_currency1, currencyPair.m_currency2);
			if (pOrderChain)
			{
				const auto rate = pOrderChain->getFinalAmount(1.);
				IRSTD_THROW_ASSERT(TraderExchange, rate > 0, "Procesed rate must be greater than zero in "
						<< *pOrderChain << ", rate=" << rate);
				const auto variation = std::abs((currencyPair.m_approximatedRate - rate) / rate);
				IRSTD_THROW_ASSERT(TraderExchange, variation <= ACCEPTABLE_VARIATION, "Variation is out of bound in "
						<< *pOrderChain << ", variation=" << variation
						<< ", rate=" << rate << ", expected.rate=" << currencyPair.m_approximatedRate);
			}
		}
	}

	// Make sure that the spread between pairs and inverted pairs is within acceptabel limits
	{
		constexpr double ACCEPTABLE_SPREAD = 0.5; // 50%
		getTransactionMap().getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2,
				const PairTransactionMap::PairTransactionPointer pTransaction1) {

			const auto rate = pTransaction1->getRate();
			IRSTD_THROW_ASSERT(TraderExchange, rate > 0, "Rate for " << *pTransaction1
					<< " must be positive, rate=" << rate);

			const auto pTransaction2 = getTransactionMap().getTransaction(currency2, currency1);
			if (pTransaction2)
			{
				IRSTD_THROW_ASSERT(TraderExchange, pTransaction2->getRate() > 0,
						"Rate for " << *pTransaction2 << " must be positive, rate="
						<< pTransaction2->getRate());
				const IrStd::Type::Decimal rateBack = IrStd::Type::doubleFormat(1. / pTransaction2->getRate(),
						pTransaction1->getDecimalPlace());
				const auto spread = rateBack - rate;
				if (spread < 0)
				{
					IRSTD_LOG_WARNING(TraderExchange, "Spread for " << *pTransaction1
							<< " is negative which can happen but is unlikely, spread="
							<< spread << ", rate=" << rate << ", rate.back=" << rateBack);
				}
				const auto spreadRatio = spread / rate;
				IRSTD_THROW_ASSERT(TraderExchange, spreadRatio <= ACCEPTABLE_SPREAD,
						"Spreead ratio is out of bound for " << *pTransaction1 << ", spread.ratio=" << spreadRatio
						<< ", spread=" << spread << ", rate=" << rate << ", rate.back=" << rateBack);

				// Create the order back and forth and make sure it is negative
				{
					Order order(pTransaction1);
					order.addNext(Order(pTransaction2));
					IRSTD_THROW_ASSERT(TraderExchange, order.getFinalAmount(1.) < 1.,
							"The order " << order << " should give a negative gain order.getFinalAmount(1.)="
							<< order.getFinalAmount(1.));
				}
			}
		});
	}

	IRSTD_LOG_INFO(TraderExchange, "Sanity check passed for " << getId());
}

// ---- Trader::Exchange (properties) -----------------------------------------

void Trader::Exchange::updatePropertiesStart()
{
	IRSTD_LOG_TRACE(TraderExchange, "Starting propeties update for " << getId());
	createThread("Properties", &Exchange::updatePropertiesThread, this);
}

void Trader::Exchange::updatePropertiesThread()
{
	do
	{
		try
		{
			IRSTD_LOG_TRACE(TraderExchange, "Updating properties for " << getId());

			// Update a local version of the properties map
			PairTransactionMap transactionMap;
			IRSTD_HANDLE_RETRY(updatePropertiesImpl(transactionMap), 3);

			if (m_transactionMap != transactionMap)
			{
				IRSTD_LOG_INFO(TraderExchange, "Properties updated for " << getId());

				auto scope = m_lockProperties.writeScope();

				// Copy transaction into the current one
				m_transactionMap = transactionMap;

				// Update the currency list
				size_t nbTransactionPairs = 0;
				{
					m_currencyList.clear();
					getTransactionMap().getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2) {
						m_currencyList.insert(currency1);
						m_currencyList.insert(currency2);
						nbTransactionPairs++;
					});
				}
				IRSTD_LOG_INFO(TraderExchange, "Identified " << m_currencyList.size() << " currencies and "
						<< nbTransactionPairs << " transaction pairs for " << getId());

				// Build the order chains
				{
					m_orderChainMap.clear();
					buildOrderChainMap();
				}

				// Notify that the properties have been updated
				m_eventProperties.trigger();
			}
		}
		catch (const IrStd::Exception& e)
		{
			// Any unhandled exception shall break the thread
			IRSTD_LOG_FATAL(TraderExchange, getId() << ": unhandled error: " << e
						<< ", trace=" << e.trace() << ", ignore.");
		}
	} while (IrStd::Threads::sleep(m_configuration.getPropertiesPollingPeriodMs()));
}

// ---- Trader::Exchange (rates) ----------------------------------------------

void Trader::Exchange::updateRatesStart()
{
	IRSTD_LOG_TRACE(TraderExchange, "Starting rates update for " << getId());
	createThread("Rates", &Exchange::updateRatesThread, this);
}

void Trader::Exchange::updateRatesStop()
{
	IRSTD_LOG_TRACE(TraderExchange, "Stopping rates update for " << getId());
	// Use periodic rates polling or a specific implmentation
	if (m_configuration.isRatesPolling())
	{
		terminateThread("Rates");
	}
	else
	{
		IRSTD_HANDLE_RETRY(updateRatesStopImpl(), 3);
	}
}

void Trader::Exchange::updateRatesThread()
{
	// If this is not a rate polling, run
	if (!m_configuration.isRatesPolling())
	{
		IRSTD_HANDLE_RETRY(updateRatesStartImpl(), 3);
		return;
	}

	// Update every X seconds
	do
	{
		try
		{
			switch (m_configuration.getRatesPolling())
			{
			case ConfigurationExchange::RatesPolling::UPDATE_RATES_IMPL:
				IRSTD_HANDLE_RETRY(updateRatesImpl(), 3);
				break;

			case ConfigurationExchange::RatesPolling::UPDATE_RATES_SPECIFIC_CURRENCY_IMPL:
				{
					// Launch all futures asynchronously
					std::vector<std::future<bool>> asyncFetchList;
					getCurrencies([&](const CurrencyPtr currency) {
						std::future<bool> future = std::async(std::launch::async, [=]{
							IRSTD_HANDLE_RETRY(updateRatesSpecificCurrencyImpl(currency), 3);
							return true;
						});
						asyncFetchList.push_back(std::move(future));
					});
					// Wait until all of them are completed
					for (auto& future : asyncFetchList)
					{
						IRSTD_THROW_ASSERT(TraderExchange, future.get() == true,
								"An error occured while fetching the data");
					}
				}
				break;

			case ConfigurationExchange::RatesPolling::UPDATE_RATES_SPECIFIC_PAIR_IMPL:
				{
					// Launch all futures asynchronously
					std::vector<std::future<bool>> asyncFetchList;
					getTransactionMap().getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2,
							const PairTransactionMap::PairTransactionPointer pTransaction) {
						if (!pTransaction->isInvertedTransaction())
						{
							std::future<bool> future = std::async(std::launch::async, [=]{
								IRSTD_HANDLE_RETRY(updateRatesSpecificPairImpl(currency1, currency2), 3);
								return true;
							});
							asyncFetchList.push_back(std::move(future));
						}
					});
					// Wait until all of them are completed
					for (auto& future : asyncFetchList)
					{
						IRSTD_THROW_ASSERT(TraderExchange, future.get() == true,
								"An error occured while fetching the data");
					}
				}
				break;

			case ConfigurationExchange::RatesPolling::NONE:
			default:
				IRSTD_UNREACHABLE();
			}

			// Notify that the rates have been updated
			m_eventRates.trigger();
		}
		catch (const IrStd::Exception& e)
		{
			// Any unhandled exception shall break the thread
			IRSTD_LOG_FATAL(TraderExchange, getId() << ": unhandled error: " << e
						<< ", trace=" << e.trace() << ", ignore.");
		}
	} while (IrStd::Threads::sleep(m_configuration.getRatesPollingPeriodMs()));
}

void Trader::Exchange::updateRatesSpecificPairImpl(
		const CurrencyPtr /*currency1*/,
		const CurrencyPtr /*currency2*/)
{
	IRSTD_CRASH(TraderExchange, "Exchange::updateRatesSpecificPairImpl is not implemented on " << getId());
}

void Trader::Exchange::updateRatesSpecificCurrencyImpl(const CurrencyPtr /*currency*/)
{
	IRSTD_CRASH(TraderExchange, "Exchange::updateRatesSpecificCurrencyImpl is not implemented on " << getId());
}

void Trader::Exchange::updateRatesImpl()
{
	IRSTD_CRASH(TraderExchange, "Exchange::updateRatesImpl is not implemented on " << getId());
}

void Trader::Exchange::updateRatesStartImpl()
{
	IRSTD_CRASH(TraderExchange, "Exchange::updateRatesStartImpl is not implemented on " << getId());
}

void Trader::Exchange::updateRatesStopImpl()
{
	IRSTD_CRASH(TraderExchange, "Exchange::updateRatesStopImpl is not implemented on " << getId());
}

// ---- Trader::Exchange (jobs) -----------------------------------------------

namespace
{
	IrStd::ThreadPool<8>& getJobPool()
	{
		static IrStd::ThreadPool<8> pool("jobPool");
		return pool;
	}
}

void Trader::Exchange::addJob(const std::function<void()>& job)
{
	::getJobPool().addJob(job);
}

void Trader::Exchange::waitForAllJobsToBeCompleted()
{
	::getJobPool().waitForAllJobsToBeCompleted();
}

// ---- Trader::Exchange (process) --------------------------------------------

void Trader::Exchange::process(
		const Operation& operation,
		const size_t nbRetries,
		const std::string message)
{
	auto copyOperation = operation;

	if (nbRetries)
	{
		copyOperation.onOrderError("retryOnFailure", [this, operation, nbRetries](
				ContextHandle& /*contextProceed*/, const TrackOrder& track) {
			IRSTD_LOG_INFO(TraderExchange, getId() << ": " << track.getIdForTrace()
					<< " failed, retrying (left: " << (nbRetries - 1) << ")...");
			std::stringstream retryMessageStream;
			retryMessageStream << "Retrying (left: " << (nbRetries - 1) << ")";

			auto updatedOperation = operation;
			// Check if the amount of the order needs to be adjusted
			{
				const auto availableAmount = m_balance.getWithReserve(operation.m_order.getInitialCurrency());
				if (availableAmount < operation.m_amount)
				{
					IRSTD_LOG_INFO(TraderExchange, getId() << ": amount available for failed " << track.getIdForTrace()
							<< " is lower than the amount requested: " << availableAmount
							<< " vs " << operation.m_amount << ", adjusting");
					retryMessageStream << ", adjusting amount to " << availableAmount;
					updatedOperation.m_amount = availableAmount;
				}
			}

			process(updatedOperation, nbRetries - 1, retryMessageStream.str());
		}, EventManager::Lifetime::ORDER);
	}
	else
	{
		copyOperation.onOrderError("monitorFailure", [](
				ContextHandle& contextProceed, const TrackOrder& /*track*/) {
			auto operationContext = contextProceed.cast<OperationContext>();
			operationContext->setFailureCause(OperationContext::FailureCause::PLACE_ORDER);
		}, EventManager::Lifetime::ORDER);
	}


	process(copyOperation, message);
}

void Trader::Exchange::process(
		const Operation& operation,
		const std::string message)
{
	IRSTD_ASSERT(TraderExchange, !m_configuration.isReadOnly(),
			"Operations are not permited on " << getId());

	const auto& order = operation.m_order;
	// Floor the amount to process
	const auto amount = IrStd::Type::doubleFormat(operation.m_amount,
			order.getTransaction()->getDecimalPlace(), IrStd::TypeFormat::FLAG_FLOOR);

	// Create a track order for this operation, note this will
	// also fix the order rate within the track order
	TrackOrder trackOrder{order, amount};
	// Associate the context to the track order in order to track it
	trackOrder.setContext(operation.m_context);
	const auto id = trackOrder.getId();
	const auto idForTrace = trackOrder.getIdForTrace();
	const auto originalEvents = operation.m_events;
	const auto type = trackOrder.getType();

	// Make sure the track order is valid
	if (!order.isValid(amount))
	{
		std::stringstream strStream;
		strStream << "Invalid order " << order << " with amount " << amount << ", ignoring";
		// This is done to ensure that the event is logged correctly
		m_orderTrackList.add(std::move(trackOrder), message.c_str());
		m_orderTrackList.activate(id, /*mustExists*/true);
		m_orderTrackList.remove(TrackOrderList::RemoveCause::CANCEL, id, strStream.str().c_str(), /*mustExists*/true);
		IRSTD_LOG_ERROR(TraderExchange, getId() << ": " << strStream.str());
		return;
	}

	// Copy the operation to add extra events
	auto updatedOperation = operation;

	// If this order is a linked order, register an event on this order
	if (order.getNext())
	{
		auto& nextOrder = *order.getNext();

		// Note: do not copy onError events along the way. They should be used only
		// for the first order and be renewed after
		updatedOperation.onOrderComplete("nextOrder", [this, nextOrder, originalEvents](
				ContextHandle& contextProceed,
				const TrackOrder& track,
				const IrStd::Type::Decimal amountProceed) {

			const auto order = track.getOrder();
			const auto finalAmount = order.getFirstOrderFinalAmount(amountProceed);

			IRSTD_LOG_DEBUG(TraderExchange, getId() << ": finished processing " << amountProceed << " "
					<< order.getInitialCurrency() << " of " << track.getIdForTrace()
					<< ", now processing " << finalAmount << " "
					<< order.getFinalCurrency() << " of " << nextOrder);

			if (nextOrder.isValid(finalAmount))
			{
				// Create the operation and copy all events which minimum level is operation level
				Operation nextOperation(nextOrder, finalAmount, contextProceed.cast<OperationContext>());
				nextOperation.m_events.copy(originalEvents, EventManager::Lifetime::OPERATION);
				std::stringstream messageStream;
				messageStream << "Next order from " << track.getIdForTrace();
				process(nextOperation, /*nbRetries*/10, messageStream.str().c_str());
			}
			else
			{
				IRSTD_LOG_WARNING(TraderExchange, getId() << ": do not proceed with next order from "
						<< track.getIdForTrace() << " as it would be invalid, most likely too small ("
						<< finalAmount << " " << nextOrder.getInitialCurrency() << "), ignoring");
			}

		}, EventManager::Lifetime::ORDER);
	}

	// Monitor timeout
	updatedOperation.onOrderTimeout("monitorTimeout", [](
			ContextHandle& contextProceed,
			const TrackOrder& /*track*/) {
		auto operationContext = contextProceed.cast<OperationContext>();
		operationContext->setFailureCause(OperationContext::FailureCause::TIMEOUT);
	}, EventManager::Lifetime::ORDER);

	// These operation must be done atomically in order to ensure that the events are always
	// registered at the same time as the order
	{
		auto scope = m_lockOrders.writeScope();

		// Register the event(s) that are order level or higher
		m_eventManager.copyOrder(id, updatedOperation.m_events, EventManager::Lifetime::ORDER);

		// Add the order to list and update the reserve
		m_orderTrackList.add(std::move(trackOrder), message.c_str());
		m_balance.updateReserve(*this);
	}

	// Spawn a new process to avoid this function to block
	addJob([=]() {
		// Copy the first order, this is to ensure that none of the consecutive order are missused
		// as they should not have any effect
		const auto pFirstOrder = order.copy(/*firstOnly*/true);

		// Make the actual order
		std::vector<Id> createdOrderIdList;
		IRSTD_LOG_INFO(TraderExchange, getId() << ": placing " << idForTrace);
		try
		{
			// Do not allow any retry of the API, this is too dangerous
			switch (type)
			{
			case TrackOrder::Type::MARKET:
			case TrackOrder::Type::LIMIT:
				setOrderImpl(*pFirstOrder, amount, createdOrderIdList);
				break;
			case TrackOrder::Type::WITHDRAW:
				withdrawImpl(pFirstOrder->getInitialCurrency(), amount);
				break;
			default:
				IRSTD_UNREACHABLE(TraderExchange);
			}
			// Note, after here, the order id might have disapeared already,
			// it can happen if it has matched an order
		}
		catch (const IrStd::Exception& e)
		{
			IRSTD_LOG_ERROR(TraderExchange, getId() << ": error while placing " << idForTrace
					<< " (" << *pFirstOrder << "): " << e);
			m_orderTrackList.remove(TrackOrderList::RemoveCause::FAILED, id, e.what(), /*mustExists*/false);
		}

		// If a created order Id has been registered, set it to the current order
		if (createdOrderIdList.size())
		{
			if (m_orderTrackList.match(id, createdOrderIdList, /*mustExists*/false))
			{
				IRSTD_LOG_INFO(TraderExchange, getId() << ": assign (" << IrStd::arrayJoin(createdOrderIdList) 
					<< ") to " << idForTrace);
			}
		}

		// Mark order as active
		m_orderTrackList.activate(id, /*mustExists*/false);

		// Notify the update of the order list
		m_eventUpdateBalanceAndOrders.trigger();
	});
}

// ---- Trader::Exchange (balance & orders) -----------------------------------

void Trader::Exchange::updateBalanceAndOrdersThread()
{
	IRSTD_ASSERT(TraderExchange, !m_configuration.isReadOnly(),
			"Operation not permited on " << getId());

	auto curEventCounter = m_eventUpdateBalanceAndOrders.getCounter();
	bool needUpdate = true;

	// Update de balance if the orders are updated
	while (IrStd::Threads::isActive())
	{
		// If the counter has changed since last check
		if (needUpdate || curEventCounter != m_eventUpdateBalanceAndOrders.getCounter())
		{
			IrStd::Threads::setActive();
			curEventCounter = m_eventUpdateBalanceAndOrders.getCounter();
			try
			{
				needUpdate = updateBalanceAndOrders();
			}
			catch (const IrStd::Exception& e)
			{
				// Any unhandled exception shall break the thread
				IRSTD_LOG_ERROR(TraderExchange, getId() << ": unhandled error: " << e
						<< ", trace=" << e.trace() << ", abort.");
				break;
			}
		}
		else
		{
			IrStd::Threads::setIdle();
			m_eventUpdateBalanceAndOrders.waitForNext(m_configuration.getOrderPollingPeriodMs());
			needUpdate = true;
		}
	}
}

void Trader::Exchange::updateBalanceAndOrdersStart()
{
	IRSTD_ASSERT(TraderExchange, !m_configuration.isReadOnly(),
			"Operation not permited on " << getId());
	createThread("Balance&Orders", &Exchange::updateBalanceAndOrdersThread, this);
}

/**
 * Order and balance must be updated together
 *
 * Balance (B) must be updated after order list (O)
 * Balance Update: B B B B B B B
 * Order update:   O   O   O
 * update:                x1   x2
 *                 1   2
 * If order disappear: 
 * Balance movements for update x1 must be within 1 (before the previous order update) and x1
 * Balance movements for update x2 must be within 2 (before the previous order update) and x2
 */
bool Trader::Exchange::updateBalanceAndOrders()
{
	IRSTD_ASSERT(TraderExchange, !m_configuration.isReadOnly(),
			"Operation not permited on " << getId());

	// Those are the information that we are tryiing to get
	std::vector<TrackOrder> trackOrderList;
	Balance balance;
	const auto curEventCounter = m_eventUpdateBalanceAndOrders.getCounter();
	// By default print the balance the first time
	bool printBalance = m_balance.empty();

	// Previous valid balance update (note: -1 to make sure the timestamp will be taken into account)
	const auto lastValidBalanceUpdate = m_orderTrackList.getBalanceMovements().getLastUpdateTimestamp() - 1;

	// Note the balance must be updated after the order list
	// in order to catch movement balance if an order disappear
	try
	{
		IRSTD_LOG_TRACE(TraderExchange, "Updating order list for " << getId());
		IRSTD_HANDLE_RETRY({
			trackOrderList.clear();
			updateOrdersImpl(trackOrderList);
		}, 3);

		IRSTD_LOG_TRACE(TraderExchange, "Updating balance for " << getId());
		IRSTD_HANDLE_RETRY({
			balance.clear();
			updateBalanceImpl(balance);
			m_orderTrackList.updateBalance(balance);
		}, 3);
	}
	catch (const IrStd::Exception& e)
	{
		IRSTD_LOG_ERROR(TraderExchange, getId() << ": error while fetching balance/orders: " << e
				<< ", trace=" << e.trace() << ", ignore.");
		return false;
	}

	// If a modification occured in the mean time, re-load the balance and order
	if (m_eventUpdateBalanceAndOrders.getCounter() > curEventCounter)
	{
		return true;
	}

	// Update the order list and the events under the order scope
	// Note this must be before setting the balance, as the balance
	// is set with the orders.
	{
		printBalance |= m_orderTrackList.update(trackOrderList, lastValidBalanceUpdate);
		m_eventManager.garbageCollection(*this);
		m_eventOrders.trigger();
	}

	// Update the balance data into the real balance
	{
		auto scope = m_lockBalance.writeScope();
		m_balance.setFundsAndUpdateReserve(balance, *this, m_configuration.isBalanceIncludeReserve());
		m_orderTrackList.reserveBalance(m_balance);
		// Set the initial balance if not set
		if (m_initialBalance.empty())
		{
			m_initialBalance.setFunds(m_balance);
		}
		m_eventBalance.trigger();
	}

	// Print the order list and the balance
	if (printBalance)
	{
		auto scope = m_lockBalance.readScope();
		IRSTD_LOG_INFO(TraderExchange, "Balance for " << getId() << "\n"
				<< m_balance
				<< "==================================================================\n"
				<< m_orderTrackList
				<< m_eventManager << "\n" 
				<< "Timestamp: " << getServerTimestamp() << " (" << std::showpos
				<< (m_timestampDelta / 1000.) << std::noshowpos << "s)");
	}

	// Handles timeout of the order
	bool needUpdate = false;
	try
	{
		needUpdate |= m_orderTrackList.cancelTimeout(getServerTimestamp(), [&](const TrackOrder& track) {
			IRSTD_HANDLE_RETRY(cancelOrderImpl(track), 3);
			IRSTD_LOG_INFO(TraderExchange, getId() << ": Order#" << track.getId() << " canceled");
			return true;
		});
	}
	catch (const IrStd::Exception& e)
	{
		IRSTD_LOG_ERROR(TraderExchange, getId() << ": error while canceling: " << e
				<< ", trace=" << e.trace() << ", ignore.");
	}

	return needUpdate;
}

// ---- Trader::Exchange (watchdog) ---------------------------------------------

void Trader::Exchange::watchdogThread()
{
	constexpr size_t WATCHDOG_TIMEOUT_S = 60;
	constexpr size_t RETRY_CONNECT_S = 60;

	// Check if it needs a restart
	size_t restartCounter = 0;
	while (IrStd::Threads::isActive())
	{
		const size_t initalRestartCounter = restartCounter;

		// If the exchange is not connected, attempt to connect
		if (m_status == Status::DISCONNECTED)
		{
			try
			{
				connect();
			}
			catch (const IrStd::Exception& e)
			{
				IRSTD_LOG_ERROR(TraderExchange, "Error while connecting to "
						<< getId() << ": " << e << ", will retry in "
						<< RETRY_CONNECT_S << "s");
				disconnect();

				// Wait for RETRY_CONNECT_S seconds
				size_t counter = RETRY_CONNECT_S;
				while (counter-- && IrStd::Threads::sleep(1000))
				{
					// Do nothing
				}
			}
		}

		// If not connected, re-iterate
		if (m_status != Status::CONNECTED)
		{
			continue;
		}

		// Monitor the activity once connected
		IrStd::Threads::setIdle();
		const auto pEvent = (!m_configuration.isReadOnly()) ?
				IrStd::Event::waitForNexts(WATCHDOG_TIMEOUT_S * 1000, m_eventBalance, m_eventOrders, m_eventRates)
				: IrStd::Event::waitForNexts(WATCHDOG_TIMEOUT_S * 1000, m_eventRates);
		IrStd::Threads::setActive();
		if (pEvent != nullptr && m_status == Status::CONNECTED)
		{
			IRSTD_LOG_ERROR(TraderExchange, "Watchdog detected inactivity on "
					<< getId() << " for event " << *pEvent << " (timeout="
					<< WATCHDOG_TIMEOUT_S << "s)");
			restartCounter++;
		}

		// Make sure that the threads are active
		{
			std::lock_guard<std::mutex> lock(m_threadIdMapLock);
			for (auto& it : m_threadIdMap)
			{
				if (!IrStd::Threads::isActive(it.second) && m_status == Status::CONNECTED)
				{
					IRSTD_LOG_ERROR(TraderExchange, getId() << ": thread ("
							<< *IrStd::Threads::get(it.second) << ") is inactive");
					restartCounter++;
				}
			}
		}

		// If the counter did not change, reset it
		if (initalRestartCounter == restartCounter)
		{
			restartCounter = 0;
		}
		else if (restartCounter > 6)
		{
			disconnect();
		}
	}
}

void Trader::Exchange::watchdogStart()
{
	createThread("Watchdog", &Exchange::watchdogThread, this);
}

void Trader::Exchange::watchdogStop()
{
	// Reset all events
	m_eventRates.reset();
	m_eventProperties.reset();
	m_eventOrders.reset();
	m_eventBalance.reset();
	m_eventUpdateBalanceAndOrders.reset();
	terminateThread("Watchdog", /*mustExist*/false);
}

// ---- Trader::Exchange (timestamp) ------------------------------------------

IrStd::Type::Timestamp Trader::Exchange::getServerTimestamp() const noexcept
{
	return IrStd::Type::Timestamp::now() + m_timestampDelta;
}

void Trader::Exchange::setServerTimestamp(const IrStd::Type::Timestamp timestamp) noexcept
{
	m_timestampDelta = timestamp - IrStd::Type::Timestamp::now();
}

IrStd::Type::Timestamp Trader::Exchange::getConnectedTimestamp() const noexcept
{
	return m_connectedTimestamp;
}

// ---- Trader::Exchange::toStream* -------------------------------------------

void Trader::Exchange::toStreamRates(std::ostream& out)
{
	auto scope1 = m_lockRates.readScope();

	constexpr size_t CELL_WIDTH = 15;

	// Display the header
	out << std::setw(CELL_WIDTH) << " ";
	getCurrencies([&](const CurrencyPtr currency) {
		out << std::setw(CELL_WIDTH) << currency;
	});
	out << std::endl;

	// Loop through all transaction pairs
	getCurrencies([&](const CurrencyPtr currency1) {
		out << std::setw(CELL_WIDTH) << currency1;
		getCurrencies([&](const CurrencyPtr currency2) {
			if (currency1 != currency2) {
				const auto pTransaction = getTransactionMap().getTransaction(currency1, currency2);
				if (pTransaction)
				{
					out << std::setw(CELL_WIDTH) << pTransaction->getRate();
				}
				else
				{
					out << std::setw(CELL_WIDTH) << "-";
				}
			}
			else
			{
				out << std::setw(CELL_WIDTH) << "-";
			}
		});
		out << std::endl;
	});
}
