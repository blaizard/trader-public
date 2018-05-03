#include "Trader/tests/TestBase.hpp"

// ---- Trader::TestBase ------------------------------------------------------

Trader::TestBase::TestBase()
		: m_exchange()
		, m_strategy({
			{"exchangeList", {m_exchange.getId().c_str()}}
		})
{
}

std::shared_ptr<Trader::Transaction> Trader::TestBase::createPairTransaction(
			CurrencyPtr currencyFrom,
			CurrencyPtr currencyTo) const
{
	return std::make_shared<PairTransactionImpl>(currencyFrom, currencyTo);
}

Trader::Balance Trader::TestBase::createBalance(const std::initializer_list<std::pair<CurrencyPtr, IrStd::Type::Decimal>>& initialBalance) const
{
	Balance balance;
	for (const auto fund : initialBalance)
	{
		balance.set(fund.first, fund.second);
	}
	return balance;
}

Trader::ContextHandle Trader::TestBase::createOperationContext() const
{
	return Context::create<OperationContext>(m_strategy);
}
