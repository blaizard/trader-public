#include "Trader/tests/TestBase.hpp"

class PairTransactionMapTest : public Trader::TestBase
{
};

// ---- testComparison --------------------------------------------------------

TEST_F(PairTransactionMapTest, testComparison)
{
	Trader::PairTransactionMap map1;
	Trader::PairTransactionMap map2;

	// No data in both
	ASSERT_TRUE(map1 == map2);

	// A pair in one, and no data in the other
	map1.registerPair<Trader::PairTransactionImpl>(Trader::PairTransactionImpl(Trader::Currency::EUR, Trader::Currency::USD));
	ASSERT_TRUE(map1 != map2);

	// 1 same pair in both
	map2.registerPair<Trader::PairTransactionImpl>(Trader::PairTransactionImpl(Trader::Currency::EUR, Trader::Currency::USD));
	ASSERT_TRUE(map1 == map2);

	// 2 pair in one (same initial currency), 1 in the other
	map2.registerPair<Trader::PairTransactionImpl>(Trader::PairTransactionImpl(Trader::Currency::EUR, Trader::Currency::BTC));
	ASSERT_TRUE(map1 != map2);

	// 2 pair in both (same initial currency)
	map1.registerPair<Trader::PairTransactionImpl>(Trader::PairTransactionImpl(Trader::Currency::EUR, Trader::Currency::BTC));
	ASSERT_TRUE(map1 == map2);

	// 2 pairs in both, different fee
	map1.getTransactionForWrite(Trader::Currency::EUR, Trader::Currency::BTC)->setFeePercent(1);
	ASSERT_TRUE(map1 != map2);

	// 2 pairs in both, same fee again
	map2.getTransactionForWrite(Trader::Currency::EUR, Trader::Currency::BTC)->setFeePercent(1);
	ASSERT_TRUE(map1 == map2);
}
