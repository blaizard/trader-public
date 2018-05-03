#include "Operation.hpp"
#include "Trader/Manager/Manager.hpp"

IRSTD_TOPIC_REGISTER(Trader, Operation);

Trader::Operation::Operation(
	const Order& order,
	const IrStd::Type::Decimal amount,
	const OperationContextHandle& context)
		: m_order(order)
		, m_amount(amount)
		, m_context(context)
{
	IRSTD_ASSERT(IRSTD_TOPIC(Trader, Operation), m_context, "Context must be valid");
	IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, Operation), m_order.isValid(m_amount),
			"Order is not valid, amount=" << m_amount << ", order=" << m_order);

	// Record all transactions from an operation
	onOrderComplete("recordTransaction", Trader::Operation::recordTransaction, EventManager::Lifetime::OPERATION);
	// Record all transactions from an operation
	onOrderComplete("applyProfit", Trader::OperationContext::applyProfit, EventManager::Lifetime::OPERATION);
}

void Trader::Operation::recordTransaction(
		ContextHandle& /*contextProceed*/,
		const TrackOrder& trackProceed,
		const IrStd::Type::Decimal amountProceed)
{
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	// Open the file and populate it with the data
	// Format:
	// - Timestamp now
	// - Creation timestamp
	// - Order Id
	// - Initial Currency
	// - Final Currency
	// - Amount
	// - Rate
	// - Final Amount
	// - Fee
	{
		const auto finalAmount = trackProceed.getOrder().getFirstOrderFinalAmount(amountProceed);
		std::string path = Trader::Manager::getGlobalOutputDirectory();
		IrStd::FileSystem::append(path, "transactions.csv");
		IrStd::FileSystem::FileCsv file(path);
		file.write(static_cast<uint64_t>(IrStd::Type::Timestamp::now()),
				static_cast<uint64_t>(trackProceed.getCreationTime()),
				trackProceed.getId(),
				trackProceed.getTypeToString(),
				trackProceed.getOrder().getInitialCurrency(),
				trackProceed.getOrder().getFirstOrderFinalCurrency(),
				amountProceed,
				trackProceed.getRate(),
				finalAmount,
				IrStd::Type::Decimal(trackProceed.getOrder().getFirstOrderFinalAmount(amountProceed, /*includeFee*/false) - finalAmount));
	}
}

void Trader::Operation::onOrderComplete(
		const char* const pDescription,
		const EventManager::OrderCompleteCallback& callback,
		const EventManager::Lifetime lifetime)
{
	m_events.m_onComplete.add(pDescription, m_context, callback, IrStd::Type::toIntegral(lifetime));
}

void Trader::Operation::onOrderError(
		const char* const pDescription,
		const EventManager::OrderErrorCallback& callback,
		const EventManager::Lifetime lifetime)
{
	m_events.m_onError.add(pDescription, m_context, callback, IrStd::Type::toIntegral(lifetime));
}

void Trader::Operation::onOrderTimeout(
		const char* const pDescription,
		const EventManager::OrderTimeoutCallback& callback,
		const EventManager::Lifetime lifetime)
{
	m_events.m_onTimeout.add(pDescription, m_context, callback, IrStd::Type::toIntegral(lifetime));
}

void Trader::Operation::onComplete(
		const char* const pDescription,
		const OperationContext::CompleteCallback& callback)
{
	m_context->onComplete(pDescription, callback);
}
