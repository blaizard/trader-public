#include "Trader/Exchange/Operation/OperationContext.hpp"
#include "Trader/Strategy/Strategy.hpp"

// ---- Trader::OperationContext ----------------------------------------------

Trader::OperationContext::OperationContext(const Strategy& strategy)
		: m_strategyId(strategy.getId())
		, m_failureCause(FailureCause::NONE)
		, m_profitRatio(0)
{
}

Trader::OperationContext::~OperationContext()
{
	IRSTD_LOG_INFO(m_description);
	triggerOnComplete();
}

void Trader::OperationContext::toStream(std::ostream& os) const
{
	Context::toStream(os);
	os << ", " << m_description;
}

Trader::Id Trader::OperationContext::getStrategyId() const noexcept
{
	return m_strategyId;
}

void Trader::OperationContext::setProfit(const CurrencyPtr currency, const IrStd::Type::Decimal profit) noexcept
{
	auto scope = m_lock.writeScope();
	if (m_profitMap.find(currency) == m_profitMap.end())
	{
		m_profitMap[currency] = {0, 0};
	}
	m_profitMap[currency].m_fixed = profit;
}

void Trader::OperationContext::addProfit(const CurrencyPtr currency, const IrStd::Type::Decimal profit) noexcept
{
	auto scope = m_lock.writeScope();
	if (m_profitMap.find(currency) == m_profitMap.end())
	{
		m_profitMap[currency] = {0, 0};
	}
	m_profitMap[currency].m_accumulated += profit;
}

void Trader::OperationContext::getProfit(const std::function<void(const CurrencyPtr, const IrStd::Type::Decimal)>& callback) const noexcept
{
	auto scope = m_lock.readScope();
	for (const auto& it : m_profitMap)
	{
		callback(it.first, it.second.m_fixed + it.second.m_accumulated);
	}
}

void Trader::OperationContext::setProfitOnComplete(const IrStd::Type::Decimal profitPercent) noexcept
{
	m_profitRatio = profitPercent / 100.;
}

bool Trader::OperationContext::isEffective() const noexcept
{
	auto scope = m_lock.readScope();
	return !m_profitMap.empty();
}

void Trader::OperationContext::setFailureCause(const FailureCause cause) noexcept
{
	m_failureCause = cause;
}

Trader::OperationContext::FailureCause Trader::OperationContext::getFailureCause() const noexcept
{
	return m_failureCause;
}

const std::string Trader::OperationContext::getDescription() const noexcept
{
	return m_description;
}

void Trader::OperationContext::onComplete(
		const char* const pName,
		CompleteCallback callback)
{
	m_onComplete.add(pName, [callback](ContextHandle& /*context*/, const OperationContext& curContext) {
		callback(curContext);
	}, 0);
}

void Trader::OperationContext::triggerOnComplete()
{
	m_onComplete.execute(*this);
}

void Trader::OperationContext::applyProfit(
		ContextHandle& context,
		const TrackOrder& trackOrder,
		const IrStd::Type::Decimal amountProceed)
{
	auto operationContext = context.cast<Trader::OperationContext>();

	// If this is the last order and the profit ratio is set
	if (operationContext->m_profitRatio != 0 && !trackOrder.getOrder().getNext())
	{
		const auto finalCurrency = trackOrder.getOrder().getFinalCurrency();
		const auto finalAmountNoFee = trackOrder.getOrder().getFinalAmount(amountProceed, /*includeFee*/false);
		const auto profit = finalAmountNoFee * operationContext->m_profitRatio;

		operationContext->addProfit(finalCurrency, profit);
	}
}
