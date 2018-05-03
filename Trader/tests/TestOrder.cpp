#include "Trader/tests/TestBase.hpp"

class OrderTest : public Trader::TestBase
{
};

// ---- testAmountCalculation -------------------------------------------------

TEST_F(OrderTest, testAmountCalculation)
{
	auto pTransaction1 = createPairTransaction(Trader::Currency::EUR, Trader::Currency::USD);
	pTransaction1->cast<Trader::PairTransactionImpl>()->setFeePercent(10.);

	// Single order
	{
		const Trader::Order order(pTransaction1, /*rate*/2.);
		ASSERT_TRUE(order.getRate() == 2.);
		ASSERT_TRUE(order.getFirstOrderFinalAmount(10.) == 18.);
		ASSERT_TRUE(order.getFinalAmount(10.) == 18.);
		ASSERT_TRUE(order.getFirstOrderInitialAmount(18.) == 10.);
		ASSERT_TRUE(order.getInitialAmount(18.) == 10.);
	}

	auto pTransaction2 = createPairTransaction(Trader::Currency::USD, Trader::Currency::BTC);
	pTransaction2->cast<Trader::PairTransactionImpl>()->setFeePercent(50.);

	// Chain order
	{
		Trader::Order order(pTransaction1, /*rate*/2.);
		order.addNext({pTransaction2, /*rate*/3.});

		ASSERT_TRUE(order.getRate() == 2.);

		ASSERT_TRUE(order.getFirstOrderFinalAmount(10.) == 18.);
		ASSERT_TRUE(order.getNext()->getFirstOrderFinalAmount(18.) == 27.);
		ASSERT_TRUE(order.getFinalAmount(10.) == 27.) << "finalAmount=" << order.getFinalAmount(10.);

		ASSERT_TRUE(order.getFirstOrderInitialAmount(18.) == 10.);
		ASSERT_TRUE(order.getNext()->getFirstOrderInitialAmount(27.) == 18.);
		ASSERT_TRUE(order.getInitialAmount(27.) == 10.) << "initialAmount=" << order.getInitialAmount(27.);
	}
}
