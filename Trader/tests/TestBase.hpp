#pragma once

#include "IrStd/Test.hpp"
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Exchange.hpp"
#include "Trader/Exchange/ExchangeMock.hpp"
#include "Trader/ExchangeImpl/Test/ExchangeTest.hpp"
#include "Trader/Strategy/Dummy/Dummy.hpp"

namespace Trader
{
	class TestBase : public IrStd::Test
	{
	public:
		TestBase();

		/**
		 * Creates a dummy transaction for test purpose
		 */
		std::shared_ptr<Transaction> createPairTransaction(CurrencyPtr currencyFrom, CurrencyPtr currencyTo) const;

		/**
		 * Create a dummy balance
		 */
		Balance createBalance(const std::initializer_list<std::pair<CurrencyPtr, IrStd::Type::Decimal>>& initialBalance) const;

		/**
		 * Create a dummy operation context
		 */
		ContextHandle createOperationContext() const;

	private:
		ExchangeMock<ExchangeTest> m_exchange;
		Dummy m_strategy;
	};
}
