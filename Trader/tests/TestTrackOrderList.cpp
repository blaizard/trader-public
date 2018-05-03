#include "Trader/tests/TestBase.hpp"

//#define DEBUG 1

IRSTD_TOPIC_USE(Trader);

#define REGISTER_TRANSACTION(fctName, currencyFrom, currencyTo) \
	std::shared_ptr<Trader::Transaction> fctName() \
	{ \
		static auto pTransaction = createPairTransaction(Trader::Currency::currencyFrom, Trader::Currency::currencyTo); \
		return pTransaction; \
	}

class TrackOrderListTest : public Trader::TestBase
{
public:
	static constexpr size_t IS_ANY = IrStd::Type::toIntegral(Trader::TrackOrderList::EachFilter::ALL);
	static constexpr size_t IS_PLACEHOLDER = IrStd::Type::toIntegral(Trader::TrackOrderList::EachFilter::PLACEHOLDER);
	static constexpr size_t IS_ACTIVE_PLACEHOLDER = IrStd::Type::toIntegral(Trader::TrackOrderList::EachFilter::ACTIVE_PLACEHOLDER);
	static constexpr size_t IS_MATCHED = IrStd::Type::toIntegral(Trader::TrackOrderList::EachFilter::MATCHED);
	static constexpr size_t IS_CANCEL = IrStd::Type::toIntegral(Trader::TrackOrderList::EachFilter::CANCEL);
	static constexpr size_t IS_FAILED = IrStd::Type::toIntegral(Trader::TrackOrderList::EachFilter::FAILED);

	static constexpr size_t IGNORE_ID = 0x010000;
	static constexpr size_t IGNORE_RATE = 0x020000;
	static constexpr size_t IGNORE_AMOUNT = 0x040000;
	static constexpr size_t IGNORE_CONTEXT = 0x080000;
	static constexpr size_t IGNORE_TRANSACTION = 0x100000;
	static constexpr size_t IGNORE_CHAIN = 0x200000;

	static constexpr size_t TIMEOUT_ORDER_REGISTERED_MS = 30 * 1000;

	TrackOrderListTest()
			: m_trackOrderList(m_eventManager, TIMEOUT_ORDER_REGISTERED_MS)
	{
#if defined(DEBUG)
		IrStd::Logger::getDefault().addTopic(IRSTD_TOPIC(Trader));
#endif
	}

	void SetUp()
	{
		Trader::TestBase::SetUp();
		m_trackOrderList.initialize(/*keepOrders*/false);
	}

	size_t getNumberOrders(const size_t check = IS_ANY) const noexcept
	{
		const auto filter = (((check & IS_ANY) == 0) ? IS_ANY : (check & IS_ANY));
		size_t nbOrders = 0;
		m_trackOrderList.each([&](const Trader::TrackOrder& trackOrder) {
			nbOrders++;
		}, static_cast<const Trader::TrackOrderList::EachFilter>(filter));
		return nbOrders;
	}

	struct ExpectedOrder
	{
		Trader::TrackOrderList::EachFilter m_type;
		Trader::TrackOrder m_trackOrder;
	};

	size_t checkOrder(const Trader::TrackOrder& trackOrder, const size_t check = IS_ANY)
	{
		size_t nbMatch = 0;
		const auto filter = (((check & IS_ANY) == 0) ? IS_ANY : (check & IS_ANY));
		m_trackOrderList.each([&](const Trader::TrackOrder& trackOrderFromList) {
			bool isMatch = (check & IGNORE_ID || trackOrderFromList.getId() == trackOrder.getId());
			isMatch &= (check & IGNORE_TRANSACTION || *trackOrderFromList.getOrder().getTransaction() == *trackOrder.getOrder().getTransaction());
			isMatch &= (check & IGNORE_RATE || IrStd::almostEqual(trackOrderFromList.getOrder().getRate(), trackOrder.getOrder().getRate()));
			isMatch &= (check & IGNORE_AMOUNT || IrStd::almostEqual(trackOrderFromList.getAmount(), trackOrder.getAmount()));
			isMatch &= (check & IGNORE_CONTEXT || trackOrderFromList.getContext() == trackOrder.getContext());
			isMatch &= (check & IGNORE_CHAIN || (!trackOrderFromList.getOrder().getNext() && !trackOrder.getOrder().getNext())
					|| (trackOrderFromList.getOrder().getNext() && trackOrder.getOrder().getNext() && *trackOrderFromList.getOrder().getNext() == *trackOrder.getOrder().getNext()));
			nbMatch += (isMatch) ? 1 : 0;
		}, static_cast<const Trader::TrackOrderList::EachFilter>(filter));
		return nbMatch;
	}

	void monitorOnOrderComplete(const Trader::Id orderId, bool& isOnCompleteTriggered, IrStd::Type::Decimal& amount)
	{
		isOnCompleteTriggered = false;
		m_eventManager.onOrderComplete("monitorOnOrderComplete", Trader::ContextHandle(), orderId,
			[&](Trader::ContextHandle& contextProceed, const Trader::TrackOrder& track, const IrStd::Type::Decimal completeAmount) {
				isOnCompleteTriggered = true;
				amount = completeAmount;
		}, Trader::EventManager::Lifetime::ORDER);
	}

	void monitorOnOrderError(const Trader::Id orderId, bool& isOnErrorTriggered)
	{
		isOnErrorTriggered = false;
		m_eventManager.onOrderError("monitorOnOrderError", Trader::ContextHandle(), orderId,
			[&](Trader::ContextHandle& contextProceed, const Trader::TrackOrder& track) {
				isOnErrorTriggered = true;
		}, Trader::EventManager::Lifetime::ORDER);
	}

	REGISTER_TRANSACTION(getTransactionUSDEUR, USD, EUR);
	REGISTER_TRANSACTION(getTransactionUSDBTC, USD, BTC);
	REGISTER_TRANSACTION(getTransactionEURUSD, EUR, USD);
	REGISTER_TRANSACTION(getTransactionEURBTC, EUR, BTC);
	REGISTER_TRANSACTION(getTransactionBTCUSD, BTC, USD);
	REGISTER_TRANSACTION(getTransactionBTCEUR, BTC, EUR);

protected:
	class TrackOrderListForTest : public Trader::TrackOrderList
	{
	public:
		template<class ... Args>
		TrackOrderListForTest(Args&& ... args)
				: Trader::TrackOrderList(std::forward<Args>(args)...)
				, m_timestamp(1.)
		{
		}

		void increaseTimestampMs(const size_t deltaMs)
		{
			m_timestamp += deltaMs;
		}

	protected:
		IrStd::Type::Timestamp m_timestamp;
		IrStd::Type::Timestamp getCurrentTimestamp() const noexcept override
		{
			return m_timestamp;
		}
	};

	Trader::EventManager m_eventManager;
	TrackOrderListForTest m_trackOrderList;
};

constexpr size_t TrackOrderListTest::TIMEOUT_ORDER_REGISTERED_MS;

// ---- testAddOrder ----------------------------------------------------------

TEST_F(TrackOrderListTest, testAddOrder)
{
	// Add a single order
	{
		Trader::TrackOrder track(Trader::Id("test-0"), getTransactionUSDEUR(), 0.5, 10);
		track.setContext(createOperationContext());
		m_trackOrderList.add(track);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(track, IS_PLACEHOLDER) == 1) << m_trackOrderList;
	}

	// Add many orders
	for (size_t i=2; i<99; i++)
	{
		Trader::TrackOrder track(Trader::Id(), getTransactionUSDEUR(), 0.5, 10);
		track.setContext(createOperationContext());
		m_trackOrderList.add(track);
		ASSERT_TRUE(getNumberOrders() == i);
		ASSERT_TRUE(checkOrder(track, IS_PLACEHOLDER) == 1) << m_trackOrderList;
	}
}

// ---- testRemoveOrder -------------------------------------------------------

TEST_F(TrackOrderListTest, testRemoveOrder)
{
	// Add a single order and remove it
	{
		const Trader::Id id("test-0");
		Trader::TrackOrder track(id, getTransactionUSDEUR(), 0.5, 10);
		m_trackOrderList.add(track);
		m_trackOrderList.remove(Trader::TrackOrderList::RemoveCause::FAILED, id, "Test failed");
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(track, IS_FAILED) == 1) << m_trackOrderList;
	}
}

// ---- testMatchSingleOrder --------------------------------------------------

TEST_F(TrackOrderListTest, testMatchSingleOrder)
{
	Trader::TrackOrder track{{getTransactionUSDEUR(), 0.5}, 10};
	track.setContext(createOperationContext());

	// Add a placeholder order
	{
		m_trackOrderList.add(track);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(track, IS_PLACEHOLDER | IGNORE_ID) == 1);
	}

	// Match it with a non-placeholder order
	{
		const Trader::TrackOrder order1{Trader::Id{"test-0"}, getTransactionUSDEUR(), 0.5, 10};
		m_trackOrderList.update({order1});

		ASSERT_TRUE(getNumberOrders() == 1) << "getNumberOrders=" << getNumberOrders()
				<< "\n" << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order1, IGNORE_CONTEXT) == 1) << m_trackOrderList << order1;
		ASSERT_TRUE(checkOrder(track, IGNORE_ID) == 1) << m_trackOrderList << track;
	}
}

// ---- testMatchSingleOrderChained -------------------------------------------

TEST_F(TrackOrderListTest, testMatchSingleOrderChained)
{
	Trader::TrackOrder track{{getTransactionUSDEUR(), 0.5}, 10};
	track.getOrderForWrite().addNext({getTransactionEURUSD(), 2.});
	track.setContext(createOperationContext());

	// Add a placeholder order
	{
		m_trackOrderList.add(track);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(track, IS_PLACEHOLDER | IGNORE_ID) == 1) << m_trackOrderList;
	}

	// Match it with a non-placeholder order
	{
		Trader::TrackOrder order1{Trader::Id{"test-0"}, getTransactionUSDEUR(), 0.5, 10};
		m_trackOrderList.update({order1});

		ASSERT_TRUE(getNumberOrders() == 1) << "getNumberOrders=" << getNumberOrders()
				<< "\n" << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order1, IS_MATCHED | IGNORE_CONTEXT | IGNORE_CHAIN) == 1) << m_trackOrderList << order1;
		ASSERT_TRUE(checkOrder(track, IGNORE_ID) == 1) << m_trackOrderList << track;
	}
}

// ---- testMatchMultipleOrders -----------------------------------------------

TEST_F(TrackOrderListTest, testMatchMultipleOrders)
{
	// Add a placeholder order
	{
		m_trackOrderList.add(std::move(Trader::TrackOrder{{getTransactionUSDEUR(), 0.5}, 10}));
		m_trackOrderList.add(std::move(Trader::TrackOrder{{getTransactionBTCUSD(), 0.5}, 10}));

		ASSERT_TRUE(getNumberOrders() == 2);
		ASSERT_TRUE(checkOrder(Trader::TrackOrder{{getTransactionUSDEUR(), 0.5}, 10}, IS_PLACEHOLDER | IGNORE_ID) == 1);
		ASSERT_TRUE(checkOrder(Trader::TrackOrder{{getTransactionBTCUSD(), 0.5}, 10}, IS_PLACEHOLDER | IGNORE_ID) == 1);
	}

	const Trader::TrackOrder order1{Trader::Id{"test-0"}, getTransactionUSDEUR(), 0.5, 10};
	const Trader::TrackOrder order2{Trader::Id{"test-1"}, getTransactionBTCUSD(), 0.5, 10};

	// Match with 2 non-placeholder orders
	{
		m_trackOrderList.update({order1, order2});
		ASSERT_TRUE(getNumberOrders() == 2) << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order1) == 1) << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order2) == 1) << m_trackOrderList;
	}

	// Non match with place holder
	{
		m_trackOrderList.add(std::move(Trader::TrackOrder{{getTransactionEURUSD(), 0.5}, 10}));
		m_trackOrderList.update({order1, order2});

		ASSERT_TRUE(getNumberOrders() == 3) << "getNumberOrders=" << getNumberOrders();
		ASSERT_TRUE(checkOrder(Trader::TrackOrder{{getTransactionEURUSD(), 0.5}, 10}, IS_PLACEHOLDER | IGNORE_ID) == 1);
		ASSERT_TRUE(checkOrder(order1) == 1) << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order2) == 1) << m_trackOrderList;
	}
}

// ---- testFailedOrder -------------------------------------------------------

TEST_F(TrackOrderListTest, testFailedOrder)
{
	// Add a placeholder order
	{
		const Trader::TrackOrder order{{getTransactionUSDEUR(), 0.5}, 10};

		bool isOnErrorTriggered = false;
		monitorOnOrderError(order.getId(), isOnErrorTriggered);

		m_trackOrderList.add(order);
		m_trackOrderList.remove(Trader::TrackOrderList::RemoveCause::FAILED, order.getId(), "Failed test", /*mustExists*/true);
		m_trackOrderList.update({});

		ASSERT_TRUE(isOnErrorTriggered == false);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(order, IS_FAILED) == 1) << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order, IS_CANCEL) == 0) << m_trackOrderList;

		// If not activate it should stay active all the time
		m_trackOrderList.increaseTimestampMs(TIMEOUT_ORDER_REGISTERED_MS * 2);
		m_trackOrderList.update({});
		ASSERT_TRUE(isOnErrorTriggered == false);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(order, IS_FAILED) == 1) << m_trackOrderList;

		// Activate the order
		m_trackOrderList.activate(order.getId(), /*mustExists*/true);
		m_trackOrderList.update({});
		ASSERT_TRUE(isOnErrorTriggered == false);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(order, IS_FAILED) == 1) << m_trackOrderList;
		ASSERT_TRUE(checkOrder(order, IS_ACTIVE_PLACEHOLDER) == 1) << m_trackOrderList;

		// After 10s, it should still be there
		m_trackOrderList.increaseTimestampMs(TIMEOUT_ORDER_REGISTERED_MS / 2);
		m_trackOrderList.update({});
		ASSERT_TRUE(isOnErrorTriggered == false);
		ASSERT_TRUE(getNumberOrders() == 1);
		ASSERT_TRUE(checkOrder(order, IS_FAILED) == 1) << m_trackOrderList;

		// If not activate it should stay active all the time
		m_trackOrderList.increaseTimestampMs(TIMEOUT_ORDER_REGISTERED_MS * 2);
		m_trackOrderList.update({});
		ASSERT_TRUE(isOnErrorTriggered == true);
		ASSERT_TRUE(getNumberOrders() == 0);
	}
}

// ---- testFailedOrderMatch --------------------------------------------------

TEST_F(TrackOrderListTest, testFailedOrderMatch)
{
	const Trader::TrackOrder order{{getTransactionUSDEUR(), 0.5}, 10};

	bool isOnErrorTriggered = false;
	monitorOnOrderError(order.getId(), isOnErrorTriggered);

	m_trackOrderList.add(order);
	m_trackOrderList.remove(Trader::TrackOrderList::RemoveCause::FAILED, order.getId(), "Failed test", /*mustExists*/true);
	m_trackOrderList.update({});
	ASSERT_TRUE(checkOrder(order, IS_FAILED) == 1) << m_trackOrderList;

	// Match the order
	{
		Trader::TrackOrder order{Trader::Id{"test-0"}, getTransactionUSDEUR(), 0.5, 10};
		m_trackOrderList.update({order});

		ASSERT_TRUE(getNumberOrders() == 1) << m_trackOrderList;
		ASSERT_TRUE(getNumberOrders(IS_MATCHED) == 1) << m_trackOrderList;
		ASSERT_TRUE(getNumberOrders(IS_FAILED) == 0) << m_trackOrderList;

		// Wait for some time and make sure the onError event is not triggered
		m_trackOrderList.increaseTimestampMs(TIMEOUT_ORDER_REGISTERED_MS * 2);
		m_trackOrderList.update({});
		ASSERT_TRUE(isOnErrorTriggered == false);
	}
}

// ---- testCompleteOrder -----------------------------------------------------

TEST_F(TrackOrderListTest, testCompleteOrder)
{
	Trader::TrackOrder order{Trader::Id{"test-0"}, getTransactionUSDEUR(), 0.5, 100};

	m_trackOrderList.updateBalance(createBalance({
			{Trader::Currency::USD, 100},
			{Trader::Currency::EUR, 0}}));

	bool isOnCompleteTriggered = false;
	IrStd::Type::Decimal completeAmount;
	monitorOnOrderComplete(order.getId(), isOnCompleteTriggered, completeAmount);

	// Add the order
	{
		m_trackOrderList.update({order});
		ASSERT_TRUE(checkOrder(order, IS_MATCHED) == 1) << m_trackOrderList;
		ASSERT_TRUE(isOnCompleteTriggered == false);
	}

	// Partial complete
	{
		order.setAmount(30);
		m_trackOrderList.update({order});
		ASSERT_TRUE(checkOrder(order, IS_MATCHED) == 1) << m_trackOrderList;
		ASSERT_TRUE(isOnCompleteTriggered == true);
		ASSERT_TRUE(completeAmount == 70);
	}

	// Full complete
	{
		isOnCompleteTriggered = false;

		// The balance must be updated. else the order will be detected as canceled
		m_trackOrderList.updateBalance(createBalance({
				{Trader::Currency::USD, 0},
				{Trader::Currency::EUR, 50}}));
		m_trackOrderList.update({});

		ASSERT_TRUE(checkOrder(order, IS_MATCHED) == 0) << m_trackOrderList;
		ASSERT_TRUE(isOnCompleteTriggered == true);
		ASSERT_TRUE(completeAmount == 30);
	}
}
