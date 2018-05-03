#include "Trader/Exchange/Transaction/WithdrawTransaction.hpp"

// ---- Trader::WithdrawTransaction -------------------------------------------

Trader::WithdrawTransaction::WithdrawTransaction(
		const CurrencyPtr currency,
		const IrStd::Type::Gson data)
	: Transaction(currency, Currency::NONE)
	, m_data(data)
	, m_feePercent(100)
	, m_feeFixed(0)
{
	setRate(1., IrStd::Type::Timestamp::now());
}

const IrStd::Type::Gson& Trader::WithdrawTransaction::getData() const noexcept
{
	return m_data;
}

IrStd::Type::Decimal Trader::WithdrawTransaction::getFinalAmountImpl(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	IRSTD_ASSERT(IrStd::almostEqual(rate, 1.), "rate=" << rate);
	return (amount - m_feeFixed) * (1. - m_feePercent / 100.);
}

IrStd::Type::Decimal Trader::WithdrawTransaction::getInitialAmountImpl(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	IRSTD_UNREACHABLE();
	IRSTD_ASSERT(IrStd::almostEqual(rate, 1.), "rate=" << rate);
	return 0;
	return amount / (1. - m_feePercent / 100.) + m_feeFixed;
}
