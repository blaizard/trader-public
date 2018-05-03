#include "IrStd/IrStd.hpp"
#include "Trader/Exchange/Order/TrackOrder.hpp"
#include "Trader/Exchange/Transaction/PairTransaction.hpp"
#include "Trader/Exchange/Transaction/WithdrawTransaction.hpp"

IRSTD_TOPIC_REGISTER(Trader, TrackOrder);
IRSTD_TOPIC_USE_ALIAS(TraderTrackOrder, Trader, TrackOrder);

// ----  Trader::TrackOrder ---------------------------------------------------

bool Trader::TrackOrder::isValid() const noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, m_order.isFixedRate(), "The rate of the transaction must be fixed, order="
			<< m_order);
	if (!m_order.isFirstValid(m_amount))
	{
		return false;
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, const Trader::TrackOrder& track)
{
	track.toStream(os);
	return os;
}

Trader::TrackOrder::Type Trader::TrackOrder::identifyOrderType(const Order& order) noexcept
{
	if (dynamic_cast<const PairTransaction*>(order.getTransaction()))
	{
		return (order.isFixedRate()) ? Type::LIMIT : Type::MARKET;
	}
	if (dynamic_cast<const WithdrawTransaction*>(order.getTransaction()))
	{
		return Type::WITHDRAW;
	}
	IRSTD_UNREACHABLE(TraderTrackOrder, "order=" << order);
}

Trader::TrackOrder::Type Trader::TrackOrder::getType() const noexcept
{
	return m_type;
}

const char* Trader::TrackOrder::getTypeToString() const noexcept
{
	return getTypeToString(m_type);
}

const char* Trader::TrackOrder::getTypeToString(const Type type) noexcept
{
	switch (type)
	{
	case Type::LIMIT:
		return "Limit";
	case Type::MARKET:
		return "Market";
	case Type::WITHDRAW:
		return "Withdraw";
	default:
		IRSTD_UNREACHABLE(TraderTrackOrder, "type=" << IrStd::Type::toIntegral(type));
	}
}

void Trader::TrackOrder::setId(const Id id) noexcept
{
	m_id = id;
}


Trader::Id Trader::TrackOrder::getId() const noexcept
{
	return m_id;
}

std::string Trader::TrackOrder::getIdForTrace() const noexcept
{
	std::string str(getTypeToString());
	str += '#';
	str += m_id;
	return str;
}

const Trader::Order& Trader::TrackOrder::getOrder() const noexcept
{
	return m_order;
}

Trader::Order& Trader::TrackOrder::getOrderForWrite() noexcept
{
	return m_order;
}

IrStd::Type::Decimal Trader::TrackOrder::getAmount() const noexcept
{
	return m_amount;
}

IrStd::Type::Decimal Trader::TrackOrder::getRate() const noexcept
{
	return m_order.getRate();
}

void Trader::TrackOrder::setAmount(const IrStd::Type::Decimal amount) noexcept
{
	m_amount = amount;
}

IrStd::Type::Timestamp Trader::TrackOrder::getCreationTime() const noexcept
{
	return m_creationTime;
}

Trader::ContextHandle Trader::TrackOrder::getContext() const noexcept
{
	return m_context;
}

void Trader::TrackOrder::setContext(ContextHandle context) noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, !m_context,
			"A context is already attached to this order: " << *this);
	IRSTD_ASSERT(TraderTrackOrder, context,
			"Only a valid context can be set to an order: " << *this);
	m_context = context;
}

void Trader::TrackOrder::toStream(std::ostream& os) const
{
	os << "[" << std::setw(10) << m_id << "] ";
	m_order.getTransaction()->toStreamPair(os);
	os << " (type=" << getTypeToString() << ", amount=" << m_amount << ", rate=" << m_order.getRate()
			<< ", timeout=" << m_order.getTimeout() << "s";

	// Print the context
	if (m_context)
	{
		os << ", context=" << m_context->getId();
	}
	else
	{
		os << ", context=<none>";
	}

	os << ", creationTime=" << m_creationTime << ")";

	// Print link orders if any
	{
		const Order* pOrder = &m_order;
		while ((pOrder = pOrder->getNext()))
		{
			os << " -> ";
			os << *pOrder;
		}
	}
}
