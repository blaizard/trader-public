#include "Trader/tests/TestBase.hpp"

class TransactionTest : public Trader::TestBase
{
};

// ---- testSimple ------------------------------------------------------------

TEST_F(TransactionTest, testSimple)
{
	auto pTransaction = createPairTransaction(Trader::Currency::EUR, Trader::Currency::USD);

	// No fee
	{
		pTransaction->setRate(1.);
		ASSERT_TRUE(pTransaction->getFinalAmount(15., /*includeFee*/false) == 15.);
		ASSERT_TRUE(pTransaction->getFinalAmount(15., /*includeFee*/true) == 15.);
		ASSERT_TRUE(pTransaction->getInitialAmount(15., /*includeFee*/false) == 15.);
		ASSERT_TRUE(pTransaction->getInitialAmount(15., /*includeFee*/true) == 15.);

		pTransaction->setRate(2.);
		ASSERT_TRUE(pTransaction->getFinalAmount(15., /*includeFee*/false) == 30.);
		ASSERT_TRUE(pTransaction->getFinalAmount(15., /*includeFee*/true) == 30.);
		ASSERT_TRUE(pTransaction->getInitialAmount(30., /*includeFee*/false) == 15.);
		ASSERT_TRUE(pTransaction->getInitialAmount(30., /*includeFee*/true) == 15.);
	}

	// fee percent
	{
		pTransaction->cast<Trader::PairTransactionImpl>()->setFeePercent(10.);

		pTransaction->setRate(1.);
		ASSERT_TRUE(pTransaction->getFinalAmount(10.) == 9.);
		ASSERT_TRUE(pTransaction->getInitialAmount(9.) == 10.);

		pTransaction->setRate(2.);
		ASSERT_TRUE(pTransaction->getFinalAmount(10.) == 18.);
		ASSERT_TRUE(pTransaction->getInitialAmount(18.) == 10.);
	}

	// fee fixed + percent
	{
		pTransaction->cast<Trader::PairTransactionImpl>()->setFeeFixed(2.);

		pTransaction->setRate(1.);
		ASSERT_TRUE(pTransaction->getFinalAmount(12.) == 9.);
		ASSERT_TRUE(pTransaction->getInitialAmount(9.) == 12.);

		pTransaction->setRate(2.);
		ASSERT_TRUE(pTransaction->getFinalAmount(12.) == 18.);
		ASSERT_TRUE(pTransaction->getInitialAmount(18.) == 12.);
	}
}

// ---- testComparison --------------------------------------------------------

TEST_F(TransactionTest, testComparison)
{
	auto pTransaction1 = createPairTransaction(Trader::Currency::EUR, Trader::Currency::USD);
	auto pTransaction2 = createPairTransaction(Trader::Currency::EUR, Trader::Currency::USD);

	// Simple equality/ineguality
	{
		auto pTransaction3 = createPairTransaction(Trader::Currency::BTC, Trader::Currency::USD);
		auto pTransaction4 = createPairTransaction(Trader::Currency::USD, Trader::Currency::EUR);

		ASSERT_TRUE(*pTransaction1 == *pTransaction1);
		ASSERT_TRUE(*pTransaction1 == *pTransaction2);
		ASSERT_TRUE(*pTransaction1 != *pTransaction3);
		ASSERT_TRUE(*pTransaction1 != *pTransaction4);
	}

	// Change decimal place
	{
		pTransaction2->setDecimalPlace(1);
		ASSERT_TRUE(*pTransaction1 != *pTransaction2);
		pTransaction1->setDecimalPlace(1);
		ASSERT_TRUE(*pTransaction1 == *pTransaction2);
	}

	// Change order decimal place
	{
		pTransaction2->setOrderDecimalPlace(1);
		ASSERT_TRUE(*pTransaction1 != *pTransaction2);
		pTransaction1->setOrderDecimalPlace(1);
		ASSERT_TRUE(*pTransaction1 == *pTransaction2);
	}

	auto pPairTransaction1 = pTransaction1->cast<Trader::PairTransactionImpl>();
	auto pPairTransaction2 = pTransaction2->cast<Trader::PairTransactionImpl>();

	// Change the fee (fixed)
	{
		pPairTransaction1->setFeeFixed(1);
		ASSERT_TRUE(*pPairTransaction1 != *pPairTransaction2);
		pPairTransaction2->setFeeFixed(1);
		ASSERT_TRUE(*pPairTransaction1 == *pPairTransaction2);
	}

	// Change the fee (percent)
	{
		pPairTransaction1->setFeePercent(1);
		ASSERT_TRUE(*pPairTransaction1 != *pPairTransaction2);
		pPairTransaction2->setFeePercent(1);
		ASSERT_TRUE(*pPairTransaction1 == *pPairTransaction2);
	}
}
