#include "Trader/Exchange/Transaction/Boundaries.hpp"

Trader::Boundaries::Boundaries()
		: m_initialAmount(0.0000001)
		, m_finalAmount(0.0000001)
		, m_rate(0.0000001)
{
}

void Trader::Boundaries::setInitialAmount(
		const IrStd::Type::Decimal minInitialAmount,
		const IrStd::Type::Decimal maxInitialAmount) noexcept
{
	m_initialAmount.merge(Interval(minInitialAmount, maxInitialAmount));
}

void Trader::Boundaries::setFinalAmount(
		const IrStd::Type::Decimal minFinalAmount,
		const IrStd::Type::Decimal maxFinalAmount) noexcept
{
	m_finalAmount.merge(Interval(minFinalAmount, maxFinalAmount));
}

void Trader::Boundaries::setRate(
		const IrStd::Type::Decimal minRate,
		const IrStd::Type::Decimal maxRate) noexcept
{
	m_rate.merge(Interval(minRate, maxRate));
}

Trader::Boundaries Trader::Boundaries::getInvert() const noexcept
{
	Boundaries invert;

	invert.setInitialAmount(m_finalAmount.getMin(), m_finalAmount.getMax());
	invert.setFinalAmount(m_initialAmount.getMin(), m_initialAmount.getMax());
	invert.setRate(m_rate.getInvertMax(), m_rate.getInvertMin());

	return invert;
}

bool Trader::Boundaries::checkInitialAmount(const IrStd::Type::Decimal amount) const noexcept
{
	return amount >= 0 && m_initialAmount.check(amount);
}

bool Trader::Boundaries::checkFinalAmount(const IrStd::Type::Decimal amount) const noexcept
{
	return amount >= 0 && m_finalAmount.check(amount);
}

bool Trader::Boundaries::checkRate(const IrStd::Type::Decimal rate) const noexcept
{
	return rate >= 0 && m_rate.check(rate);
}

void Trader::Boundaries::merge(const Boundaries& boundaries) noexcept
{
	m_initialAmount.merge(boundaries.m_initialAmount);
	m_finalAmount.merge(boundaries.m_finalAmount);
	m_rate.merge(boundaries.m_rate);
}

void Trader::Boundaries::toStream(std::ostream& out) const
{
	bool separator = false;

	m_initialAmount.toStream("intial.amount", out, separator);
	m_finalAmount.toStream("final.amount", out, separator);
	m_rate.toStream("rate", out, separator);
}
