#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Order/Order.hpp"
#include "Trader/Exchange/Transaction/PairTransaction.hpp"
#include "Trader/Exchange/Transaction/WithdrawTransaction.hpp"

constexpr double Trader::Order::INVALID_RATE;
constexpr size_t Trader::Order::DEFAULT_TIMEOUT_S;

IRSTD_TOPIC_REGISTER(Trader, Order);
IRSTD_TOPIC_USE_ALIAS(TraderOrder, Trader, Order);

// ---- Trader::Order ---------------------------------------------------------

Trader::Order::Order()
		: m_pTransaction(nullptr)
		, m_next(nullptr)
		, m_specificRate(INVALID_RATE)
		, m_timeoutS(DEFAULT_TIMEOUT_S)
{
}

Trader::Order::Order(
		std::shared_ptr<Transaction> pTransaction,
		const IrStd::Type::Decimal specificRate,
		const size_t timeoutS)
		: m_pTransaction(pTransaction)
		, m_next(nullptr)
		, m_timeoutS(timeoutS)
{
	setRate(specificRate);
}

// ---- Trader::Order (copy) --------------------------------------------------

Trader::Order::Order(const Order& order)
		: Order()
{
	*this = order;
}

Trader::Order& Trader::Order::operator=(const Order& order)
{
	m_pTransaction = order.m_pTransaction;
	m_specificRate = order.m_specificRate;
	m_timeoutS = order.m_timeoutS;
	m_next = nullptr;

	if (order.getNext())
	{
		addNext(*order.getNext());
	}

	return *this;
}

std::unique_ptr<Trader::Order> Trader::Order::copy(
		const bool firstOnly,
		const bool withFixedRate) const noexcept
{
	std::unique_ptr<Order> pOrderCopy(new Order(m_pTransaction,
			(withFixedRate) ? getRate() : m_specificRate, m_timeoutS));

	// Copy chained orders if any
	if (!firstOnly && getNext())
	{
		pOrderCopy->m_next = std::move(getNext()->copy());
	}

	return std::move(pOrderCopy);
}

// ---- Trader::Order (move) --------------------------------------------------

Trader::Order::Order(Order&& order)
		: Order()
{
	*this = std::move(order);
}

Trader::Order& Trader::Order::operator=(Order&& order)
{
	m_pTransaction = order.m_pTransaction;
	m_next = std::move(order.m_next);
	m_specificRate = order.m_specificRate;
	m_timeoutS = order.m_timeoutS;

	return *this;
}

// ---- Trader::Order ---------------------------------------------------------

bool Trader::Order::isFixedRate() const noexcept
{
	return (IrStd::almostEqual(m_specificRate, INVALID_RATE)) ? false : true;
}

IrStd::Type::Decimal Trader::Order::getRate() const noexcept
{
	if (isFixedRate())
	{
		return m_specificRate;
	}
	return m_pTransaction->getRate();
}

IrStd::Type::Decimal Trader::Order::getRate(const int position) const
{
	if (position == 0)
	{
		return getRate();
	}
	return m_pTransaction->getRate(position);
}

void Trader::Order::setRate(const IrStd::Type::Decimal rate) noexcept
{
	// Always round up the order to make sure we do not loose money
	m_specificRate = IrStd::Type::doubleFormat(rate,
			m_pTransaction->getDecimalPlace(), IrStd::TypeFormat::FLAG_CEIL);
}

const Trader::Order* Trader::Order::get(const size_t position) const noexcept
{
	const Order* pOrder = this;
	size_t counter = position;
	while (counter)
	{
		pOrder = pOrder->getNext();
		IRSTD_THROW_ASSERT(pOrder, "No order::get(" << counter << ") in " << *this);
		counter--;
	}

	return pOrder;
}

const Trader::Order* Trader::Order::getNext() const noexcept
{
	return m_next.get();
}

void Trader::Order::addNext(const Order& order)
{
	// Look for the last order
	std::unique_ptr<Order>* pNext = &m_next;
	while (pNext->get())
	{
		pNext = &((*pNext)->m_next);
	}

	// Create the new order and attach it
	*pNext = std::move(order.copy());
}

Trader::CurrencyPtr Trader::Order::getInitialCurrency() const noexcept
{
	return m_pTransaction->getInitialCurrency();
}

Trader::CurrencyPtr Trader::Order::getFinalCurrency() const noexcept
{
	// Look for the last order
	const Order* pOrder = this;
	while (pOrder->getNext())
	{
		pOrder = pOrder->getNext();
	}
	return pOrder->m_pTransaction->getFinalCurrency();
}

Trader::CurrencyPtr Trader::Order::getFirstOrderFinalCurrency() const noexcept
{
	return m_pTransaction->getFinalCurrency();
}

const Trader::Transaction* Trader::Order::getTransaction() const noexcept
{
	return m_pTransaction.get();
}

IrStd::Type::Decimal Trader::Order::getFirstOrderFinalAmount(const IrStd::Type::Decimal amount, const bool includeFee) const noexcept
{
	return m_pTransaction->getFinalAmount(amount, getRate(), includeFee);
}

IrStd::Type::Decimal Trader::Order::getFinalAmount(const IrStd::Type::Decimal amount, const bool includeFee) const noexcept
{
	const Trader::Order* pOrder = this;
	IrStd::Type::Decimal processedAmount = amount;

	do
	{
		processedAmount = pOrder->getFirstOrderFinalAmount(processedAmount, includeFee);
		pOrder = pOrder->getNext();
	} while (pOrder);

	return processedAmount;
}

IrStd::Type::Decimal Trader::Order::getFirstOrderInitialAmount(const IrStd::Type::Decimal amount, const bool includeFee) const noexcept
{
	return m_pTransaction->getInitialAmount(amount, getRate(), includeFee);
}

IrStd::Type::Decimal Trader::Order::getInitialAmount(const IrStd::Type::Decimal amount, const bool includeFee) const noexcept
{
	const std::function<IrStd::Type::Decimal(const Trader::Order*)> recGetInitialAmount = [&](const Trader::Order* pOrder) -> IrStd::Type::Decimal {
		auto curAmount = (pOrder->getNext()) ? recGetInitialAmount(pOrder->getNext()) : amount;
		return pOrder->getFirstOrderInitialAmount(curAmount, includeFee);
	};

	return recGetInitialAmount(this);
}

IrStd::Type::Decimal Trader::Order::getFeeInitialCurrency(const IrStd::Type::Decimal amount) const noexcept
{
	const Trader::Order* pOrder = this;
	IrStd::Type::Decimal processedAmount = amount;
	IrStd::Type::Decimal processedAmountWithoutFee = amount;
	IrStd::Type::Decimal conversionRate = 1.;

	do
	{
		const auto rate = pOrder->m_pTransaction->getRate();
		conversionRate *= rate;
		processedAmountWithoutFee *= rate;
		processedAmount = pOrder->m_pTransaction->getFinalAmount(processedAmount, rate);
		pOrder = pOrder->getNext();
	} while (pOrder);

	return (processedAmountWithoutFee - processedAmount) / conversionRate;
}

IrStd::Type::Decimal Trader::Order::getFeeFinalCurrency(const IrStd::Type::Decimal amount) const noexcept
{
	const Trader::Order* pOrder = this;
	IrStd::Type::Decimal processedAmount = amount;
	IrStd::Type::Decimal processedAmountWithoutFee = amount;

	do
	{
		const auto rate = pOrder->m_pTransaction->getRate();
		processedAmountWithoutFee *= rate;
		processedAmount = pOrder->m_pTransaction->getFinalAmount(processedAmount, rate);
		pOrder = pOrder->getNext();
	} while (pOrder);

	return processedAmountWithoutFee - processedAmount;
}

void Trader::Order::setTimeout(const size_t timeoutS) noexcept
{
	m_timeoutS = timeoutS;
}

size_t Trader::Order::getTimeout() const noexcept
{
	return m_timeoutS;
}

bool Trader::Order::isValid() const noexcept
{
	return static_cast<bool>(m_pTransaction);
}

bool Trader::Order::isValid(const IrStd::Type::Decimal amount) const noexcept
{
	if (!isValid())
	{
		return false;
	}

	const Trader::Order* pOrder = this;
	IrStd::Type::Decimal processedAmount = amount;

	do
	{
		if (!pOrder->m_pTransaction->isValid(processedAmount, pOrder->getRate()))
		{
			return false;
		}

		processedAmount = pOrder->getFirstOrderFinalAmount(processedAmount, /*includeFee*/true);
		const auto currencyToMatch = pOrder->getFirstOrderFinalCurrency();
		pOrder = pOrder->getNext();

		// Make sure that the currencies match
		if (pOrder && pOrder->getInitialCurrency() != currencyToMatch)
		{
			return false;
		}
	} while (pOrder);

	return true;
}

bool Trader::Order::isFirstValid(const IrStd::Type::Decimal amount) const noexcept
{
	if (isValid() && m_pTransaction->isValid(amount, getRate()))
	{
		return true;
	}

	return false;
}

void Trader::Order::toStream(std::ostream& out) const
{
	out << m_pTransaction->getInitialCurrency() << "/" << m_pTransaction->getFinalCurrency();
	if (isFixedRate())
	{
		out << "(rate=" << m_specificRate << ", timeout=" << m_timeoutS << "s)";
	}
	if (getNext())
	{
		out << " -> ";
		getNext()->toStream(out);
	}
}

bool Trader::Order::operator==(const Order& order) const noexcept
{
	// Compare the rates, the cast to the IrStd::Type::Decimal place with the constructor,
	// should ensure that this check is valid
	return (*m_pTransaction == *order.m_pTransaction
			&& IrStd::almostEqual(m_specificRate, order.m_specificRate));
}

std::ostream& operator<<(std::ostream& os, const Trader::Order& order)
{
	order.toStream(os);
	return os;
}
