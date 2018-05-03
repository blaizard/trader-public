#include <chrono>

#include "Trader/Manager/Manager.hpp"

IRSTD_TOPIC_REGISTER(Trader);
IRSTD_TOPIC_REGISTER(Trader, Manager);
IRSTD_TOPIC_USE_ALIAS(TraderManager, Trader, Manager);

// ---- Trader::ManagerEnvironment --------------------------------------------

Trader::ManagerEnvironment::ManagerEnvironment()
{
	IrStd::FileSystem::pwd(m_outputDirectory);
	IrStd::FileSystem::append(m_outputDirectory, "output");
	// Create the directory
	IrStd::FileSystem::mkdir(m_outputDirectory);
}

// ---- Trader::ManagerTrace --------------------------------------------------

void Trader::ManagerTrace::format(
		std::ostream& /*out*/,
		const IrStd::Logger::Info& info,
		const std::string& str)
{
	const TraceInfo traceInfo{info.m_level, &info.m_topic, str};

	// To make sure all traces a written in the right order,
	// we cannot use info.m_time as a newer trace might be written
	// before the current one.
	{
		static std::mutex mutex;
		std::lock_guard<std::mutex> lock(mutex);
		getInstance().m_traceInfoList.push(IrStd::Type::Timestamp::now(), traceInfo);
	}
}

// ---- Trader::Manager -------------------------------------------------------

Trader::Manager::Manager(
		const int port,
		const std::string& outputDirectory)
		: m_server(*this, port)
		, m_startedSince(0)
{
	// Add a stream to log warning and errors
	IrStd::Logger::Filter filter;
	filter.setLevel(IrStd::Logger::Level::Warning);
	IrStd::Logger::Stream stream(std::cerr, Trader::ManagerTrace::format, filter);
	IrStd::Logger::getDefault().addStream(stream);
	// Construct the absolute path of the output directory
	{
		m_outputDirectory = getGlobalOutputDirectory();
		IrStd::FileSystem::append(m_outputDirectory, outputDirectory);
		IRSTD_LOG_INFO(TraderManager, "Creating output directory: " << m_outputDirectory);
		IrStd::FileSystem::mkdir(m_outputDirectory);
	}
}

Trader::Exchange& Trader::Manager::getLastExchange()
{
	return *m_exchangeList.back();
}

const std::string& Trader::Manager::getOutputDirectory() const noexcept
{
	return m_outputDirectory;
}

const std::string& Trader::Manager::getGlobalOutputDirectory()
{
	return Trader::ManagerEnvironment::getInstance().m_outputDirectory;
}

void Trader::Manager::start()
{
	m_startedSince = IrStd::Type::Timestamp::now();
	m_server.start();

	// Start all exchanges
	for (auto& pExchange : m_exchangeList)
	{
		// Update the output directory
		{
			std::string exchangeDirectory(getOutputDirectory());
			IrStd::FileSystem::append(exchangeDirectory, pExchange->getId().c_str());
			pExchange->setOutputDirectory(exchangeDirectory);
		}
		pExchange->start();
	}

	// All important threads for this 
	std::vector<std::pair<std::string, std::thread::id>> threadList;

	// Run each strategy inside a separate thread
	for (auto& pStrategy : m_strategyList)
	{
		// Setup the strategy
		pStrategy->setup(m_exchangeList);

		// Generate the name of the thread and create the entry
		{
			std::string name(pStrategy->getId());
			name.append("(");
			pStrategy->getExchangeList([&](const Exchange& exchange) {
				if (name.back() != '(')
				{
					name.append(", ");
				}
				name.append(exchange.getId());
			});
			name.append(")");
			threadList.push_back({name, std::thread::id()});
		}

		// This is the thread processing the strategy
		threadList.back().second = IrStd::Threads::create(threadList.back().first.c_str(), [&pStrategy]() {
			std::mutex mutex;
			std::unique_lock<std::mutex> lock(mutex);
			std::condition_variable ratesUpdated;
			const std::thread::id threadId = std::this_thread::get_id();
			std::vector<std::future<void>> waitForNewRatesList;

			// Create threads to listen to each echanges
			pStrategy->getExchangeList([&](const Exchange& exchange) {
				const Exchange* const pExchange = &exchange;
				waitForNewRatesList.push_back(std::async(std::launch::async, [threadId, pExchange, &pStrategy, &ratesUpdated] {
					while (IrStd::Threads::isActive(threadId))
					{
						if (pExchange->waitForNewRates(4000/*4 seconds*/))
						{
							ratesUpdated.notify_one();
						}
					}
					IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Manager), "* New rates listener for " << pStrategy->getId()
							<< "(" << pExchange->getId() << ") is stopped");
				}));
			});

			bool needInitialization = true;
			IrStd::Type::Timestamp lastProcessedTimestamp = 0;
			size_t counterProcess = 0;
			while (IrStd::Threads::isActive())
			{
				IrStd::Threads::setIdle();
				switch (pStrategy->getConfiguration().getTrigger())
				{
				case Trader::ConfigurationStrategy::Trigger::ON_RATE_CHANGE:
					{
						if (ratesUpdated.wait_for(lock, std::chrono::seconds(4)) == std::cv_status::timeout)
						{
							continue;
						}
					}
					break;

				case Trader::ConfigurationStrategy::Trigger::EVERY_SECOND:
				case Trader::ConfigurationStrategy::Trigger::EVERY_MINUTE:
				case Trader::ConfigurationStrategy::Trigger::EVERY_HOUR:
				case Trader::ConfigurationStrategy::Trigger::EVERY_DAY:
					{
						IrStd::Threads::sleep(1000);
						const auto timeDiffS = (IrStd::Type::Timestamp::now() - lastProcessedTimestamp) / 1000;
						if (((pStrategy->getConfiguration().getTrigger() == Trader::ConfigurationStrategy::Trigger::EVERY_SECOND) && timeDiffS < 1)
								|| ((pStrategy->getConfiguration().getTrigger() == Trader::ConfigurationStrategy::Trigger::EVERY_MINUTE) && timeDiffS < 60)
								|| ((pStrategy->getConfiguration().getTrigger() == Trader::ConfigurationStrategy::Trigger::EVERY_HOUR) && timeDiffS < 60 * 60)
								|| ((pStrategy->getConfiguration().getTrigger() == Trader::ConfigurationStrategy::Trigger::EVERY_DAY) && timeDiffS < 24 * 60 * 60))
						{
							continue;
						}
					}
					break;

				default:
					IRSTD_UNREACHABLE(TraderManager);
				}
				IrStd::Threads::setActive();

				// Wait unitl the exchanges associated with this strategy are ready
				bool isReady = true;
				pStrategy->getExchangeList([&](const Exchange& exchange) {
					isReady &= (exchange.getStatus() == Exchange::Status::CONNECTED);
				});

				// If the exchanges are not ready, trigger a reinitilization for the strategy
				if (!isReady)
				{
					needInitialization = true;
					continue;
				}

				// Reinitilaize if needed
				if (needInitialization)
				{
					counterProcess = 0;
					pStrategy->initialize();
					needInitialization = false;
				}

				// Process the exchange, at this point the strategy must be initilized and
				// all related exchanges ready
				pStrategy->process(++counterProcess);
				lastProcessedTimestamp = IrStd::Type::Timestamp::now();
			}

			for (auto& waitForNewRates : waitForNewRatesList)
			{
				waitForNewRates.wait();
			}

			IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Manager), "* Strategy thread (" << threadId << ") is stopped");
		});
	}

	// Loop forever
	{
		std::condition_variable cv;
		std::mutex m;
		std::unique_lock<std::mutex> lock(m);
		cv.wait(lock, []{ return false; });
		//cv.wait_for(lock, std::chrono::seconds(20));
	}

	IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Manager), "* Stopping trader manager");

	// Kill all strategy threads
	for (const auto& thread : threadList)
	{
		IrStd::Threads::terminate(thread.second);
	}

	IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Manager), "* All strategies are stopped");

	// Disconnect all exchanges
	for (auto& pExchange : m_exchangeList)
	{
		pExchange->stop();
	}

	IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Manager), "* All exchanges are stopped");

	m_server.stop();

	IRSTD_LOG_INFO(IRSTD_TOPIC(Trader, Manager), "* Server stopped");
}

void Trader::Manager::eachExchanges(std::function<void(Exchange&)> callback)
{
	for (auto& pExchange : m_exchangeList)
	{
		callback(*pExchange);
	}
}

void Trader::Manager::eachStrategies(std::function<void(Strategy&)> callback)
{
	for (auto& pStrategy : m_strategyList)
	{
		callback(*pStrategy);
	}
}

Trader::Exchange& Trader::Manager::getExchange(const size_t index)
{
	IRSTD_THROW_ASSERT(index < m_exchangeList.size());
	return *m_exchangeList[index];
}

const Trader::Exchange& Trader::Manager::getExchange(const size_t index) const
{
	IRSTD_THROW_ASSERT(index < m_exchangeList.size());
	return *m_exchangeList[index];
}

Trader::Strategy& Trader::Manager::getStrategy(const size_t index)
{
	IRSTD_THROW_ASSERT(index < m_strategyList.size());
	return *m_strategyList[index];
}

const Trader::Strategy& Trader::Manager::getStrategy(const size_t index) const
{
	IRSTD_THROW_ASSERT(index < m_strategyList.size());
	return *m_strategyList[index];
}

const Trader::ManagerTrace::RingBufferSorted& Trader::Manager::getTraces() const noexcept
{
	return Trader::ManagerTrace::getInstance().m_traceInfoList;
}
