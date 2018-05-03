#include "Trader/Strategy/Strategy.hpp"
#include "Trader/Manager/Manager.hpp"
#include "Trader/Exchange/Operation/OperationOrder.hpp"
#include "Trader/Exchange/Transaction/WithdrawTransaction.hpp"

IRSTD_TOPIC_REGISTER(Trader, Strategy);
IRSTD_TOPIC_USE_ALIAS(TraderStrategy, Trader, Strategy);

Trader::Strategy::Strategy(const Id type, ConfigurationStrategy&& config)
		: m_type(type)
		, m_id(generateUniqueId(type))
		, m_configuration(std::move(config))
		, m_status(Status::UNINITIALIZED)
{
}

void Trader::Strategy::setup(std::vector<std::shared_ptr<Exchange>>& exchangeList)
{
	IRSTD_ASSERT(TraderStrategy, m_status == Status::UNINITIALIZED,
			"Can only setup the strategy when uninitialized");
	IRSTD_ASSERT(TraderStrategy, m_exchangeList.empty(),
			"The exchange list has laready been initialized");

	// Assign the echanges according to the configuration
	m_configuration.eachExchangeId([&](const Id& id) {
		bool exchangeAssigned = false;
		for (auto pExchange : exchangeList)
		{
			if (pExchange->getId() == id)
			{
				IRSTD_ASSERT(TraderStrategy, m_exchangeList.find(id) == m_exchangeList.end(),
						"An exchange with the similar ID (" << id << ") is already assigned.");
				m_exchangeList.insert(std::make_pair(id, ExchangeInfo(pExchange)));
				exchangeAssigned = true;
			}
		}
		IRSTD_ASSERT(TraderStrategy, exchangeAssigned, "There is no match for exchange ID ("
				<< id << ")");
	});
}

Trader::Id Trader::Strategy::generateUniqueId(const Id type)
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

const Trader::ConfigurationStrategy& Trader::Strategy::getConfiguration() const noexcept
{
	return m_configuration;
}

Trader::Id Trader::Strategy::getType() const noexcept
{
	return m_type;
}

Trader::Id Trader::Strategy::getId() const noexcept
{
	return m_id;
}

void Trader::Strategy::initialize()
{
	initializeImpl();
	IRSTD_LOG_INFO(TraderStrategy, "Initialized strategy " << getId()
			<< " with configuration " << m_configuration);
	m_status = Status::INITIALIZED;
	m_totalTime.start();
}

void Trader::Strategy::process(const size_t counter)
{
	IRSTD_ASSERT(TraderStrategy, m_status == Status::INITIALIZED,
			"Strategy must be initilaized first");
	try
	{
		IrStd::Type::Stopwatch scope(m_processTime, /*autoStart*/true);
		processImpl(counter);
	}
	catch (const IrStd::Exception& e)
	{
		IRSTD_LOG_FATAL(TraderStrategy, "Exception thrown by " << getId()
				<< ": " << e.what());
	}
}

double Trader::Strategy::getProcessTimePercent() const noexcept
{
	if (!m_totalTime.get().getMs())
	{
		return 0;
	}
	return m_processTime.getMs() / (1. * m_totalTime.get().getMs()) * 100.;
}

const IrStd::Json& Trader::Strategy::getJson() const
{
	const static IrStd::Json json;
	return json;
}

void Trader::Strategy::getProfitInJson(IrStd::Json& json) const
{
	const IrStd::Type::Gson emptyArray;
	json.add("list", emptyArray);

	for (const auto& it : m_exchangeList)
	{
		const auto& entry = it.second;
		IrStd::Json statistics;
		// Add the profit
		{
			IrStd::Json jsonExchangeProfit;
			entry.m_profit.toJson(jsonExchangeProfit, entry.m_pExchange.get());
			statistics.add("profit", jsonExchangeProfit);
		}
		// Add statistics
		statistics.add("nbSuccess", entry.m_nbSuccess);
		statistics.add("nbFailedTimeout", entry.m_nbFailedTimeout);
		statistics.add("nbFailedPlaceOrder", entry.m_nbFailedPlaceOrder);

		json.getArray("list").add(json, statistics);
	}
}

Trader::OperationContextHandle Trader::Strategy::sell(
		Exchange& exchange,
		const Order& order,
		const IrStd::Type::Decimal amount,
		const size_t nbRetries)
{
	// Look for the exchange in the list (make sure it is registered)
	const auto id = exchange.getId();

	// Create the operation
	OperationOrder operation(order, amount, *this);
	operation.onComplete("strategyProfit", [this, id](const OperationContext& curContext) {

		// Ignore operation that has not done anything
		if (!curContext.isEffective())
		{
			if (curContext.getFailureCause() == OperationContext::FailureCause::NONE)
			{
				IRSTD_LOG_ERROR(TraderStrategy, getId() << ": The operation with context " << curContext.getId()
						<< " is marked as ineffective but did not face any failure");
			}
			return;
		}

		auto it = m_exchangeList.find(id);
		IRSTD_ASSERT(TraderStrategy, it != m_exchangeList.end(), "The exchange "
				<< id << " is not registered with this strategy anymore " << getId());
		auto& entry = it->second;

		// Used to mke an estimate of the profit
		Balance balanceProfit;

		// Add the profit to the balance
		curContext.getProfit([&](const Trader::CurrencyPtr currency, const IrStd::Type::Decimal profit) {
			balanceProfit.add(currency, profit);
			entry.m_profit.add(currency, profit);
			const auto currencyEstimate = entry.m_pExchange->getEstimateCurrency();
			const auto pOrderChain = entry.m_pExchange->identifyOrderChain(currency, currencyEstimate);
			const auto profitEstimate = (pOrderChain) ?
					pOrderChain->getFinalAmount(profit, /*includeFee*/false) : IrStd::Type::Decimal(0);
			monitorProfit(*this, curContext.getId(), currency, profit, currencyEstimate, profitEstimate);
		});

		// Write a message
		const auto profitEstimate = balanceProfit.estimate(entry.m_pExchange.get());
		const auto totalProfitEstimate = entry.m_profit.estimate(entry.m_pExchange.get());

		OperationStatus operationStatus;

		// Set the operation as successfull, only if it had some recorder profit
		if (curContext.getFailureCause() == OperationContext::FailureCause::NONE)
		{
			entry.m_nbSuccess++;
			operationStatus = OperationStatus::SUCCESS;
		}
		else
		{
			// Set the status of the operation
			switch (curContext.getFailureCause())
			{
			case OperationContext::FailureCause::TIMEOUT:
				entry.m_nbFailedTimeout++;
				operationStatus = OperationStatus::TIMEOUT;
				break;
			case OperationContext::FailureCause::PLACE_ORDER:
				entry.m_nbFailedPlaceOrder++;
				operationStatus = OperationStatus::FAILED;
				break;
			case OperationContext::FailureCause::NONE:
			default:
				IRSTD_UNREACHABLE(TraderStrategy);
			}
		}

		// Record the operation
		recordOperation(operationStatus, curContext, profitEstimate, entry.m_pExchange->getEstimateCurrency());
		IRSTD_LOG_INFO(TraderStrategy, "Total profit estimation for " << getId() << ": "
				<< totalProfitEstimate << " " << entry.m_pExchange->getEstimateCurrency());
	});

	std::stringstream messageStream;
	messageStream << "Placed by strategy " << getId();
	exchange.process(operation, nbRetries, messageStream.str().c_str());

	return operation.m_context;
}

Trader::OperationContextHandle Trader::Strategy::withdraw(
		Exchange& exchange,
		const CurrencyPtr currency,
		const IrStd::Type::Decimal amount,
		const size_t nbRetries)
{
	// Create a no-operation order to the same currency
	Order order(std::make_shared<WithdrawTransaction>(currency));
	return sell(exchange, order, amount, nbRetries);
}

void Trader::Strategy::recordOperation(
		const OperationStatus status,
		const OperationContext& context,
		const IrStd::Type::Decimal profit,
		const CurrencyPtr currency) noexcept
{
	const OperationState state{
		status,
		context.getId(),
		profit,
		currency,
		std::string(context.getDescription())
	};
	m_operationRecordList.push(IrStd::Type::Timestamp::now(), state);
}

void Trader::Strategy::getRecordedOperations(const std::function<void(
		const IrStd::Type::Timestamp,
		const char* const,
		const size_t,
		const IrStd::Type::Decimal,
		const char* const,
		const char* const)>& callback) const noexcept
{
	m_operationRecordList.read([&](const IrStd::Type::Timestamp& timestamp, const OperationState& state) {
		// Identify the type of record
		const char* pStatus;
		switch (state.m_status)
		{
		case OperationStatus::SUCCESS:
			pStatus = "success";
			break;
		case OperationStatus::TIMEOUT:
			pStatus = "timeout";
			break;
		case OperationStatus::FAILED:
			pStatus = "failed";
			break;
		default:
			IRSTD_UNREACHABLE(TraderStrategy);
		}
		callback(timestamp, pStatus, state.m_id, state.m_profit, state.m_currency->getId(), state.m_description.c_str());
	}, 20);
}

void Trader::Strategy::getExchangeList(const std::function<void(const Exchange&)>& callback) const
{
	for (const auto& it : m_exchangeList)
	{
		callback(*it.second.m_pExchange);
	}
}

void Trader::Strategy::eachExchanges(const std::function<void(const Id)>& callback) const
{
	for (const auto& it : m_exchangeList)
	{
		callback(it.first);
	}
}

Trader::Exchange& Trader::Strategy::getExchange() noexcept
{
	IRSTD_ASSERT(TraderStrategy, m_exchangeList.size() == 1, "The strategy "
			<< getId() << " must have 1 exchange registered");
	return *(m_exchangeList.begin()->second.m_pExchange);
}

Trader::Exchange& Trader::Strategy::getExchange(const Id id) noexcept
{
	auto it = m_exchangeList.find(id);
	IRSTD_ASSERT(TraderStrategy, it != m_exchangeList.end(), "The exchange "
			<< id << " is not registered with this strategy " << getId());
	return *(it->second.m_pExchange);
}

// ---- Trader::Strategy (static) -----------------------------------------------

void Trader::Strategy::monitorProfit(
		const Strategy& strategy,
		const size_t contextId,
		const CurrencyPtr currency,
		const IrStd::Type::Decimal profit,
		const CurrencyPtr currencyEstimate,
		const IrStd::Type::Decimal profitEstimate)
{
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	// Open the file and populate it with the data
	{
		std::string path = Trader::Manager::getGlobalOutputDirectory();
		IrStd::FileSystem::append(path, "profit.csv");
		IrStd::FileSystem::FileCsv file(path);
		file.write(static_cast<uint64_t>(IrStd::Type::Timestamp::now()),
				strategy.getId(), contextId, currency, profit,
				currencyEstimate, profitEstimate);
	}
}
