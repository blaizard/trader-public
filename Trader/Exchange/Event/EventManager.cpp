#include "Trader/Exchange/Event/EventManager.hpp"
#include "Trader/Exchange/Exchange.hpp"

IRSTD_TOPIC_REGISTER(Trader, Event);
IRSTD_TOPIC_USE_ALIAS(TraderEvent, Trader, Event);

// ---- Trader::EventManager::OrderEvents -------------------------------------

void Trader::EventManager::OrderEvents::copy(const OrderEvents& events, const Lifetime lifetime)
{
	m_onComplete.copy(events.m_onComplete, IrStd::Type::toIntegral(lifetime));
	m_onError.copy(events.m_onError, IrStd::Type::toIntegral(lifetime));
	m_onTimeout.copy(events.m_onTimeout, IrStd::Type::toIntegral(lifetime));
}

// ---- Trader::EventManager (helper) -----------------------------------------

Trader::EventManager::OrderEvents& Trader::EventManager::getOrderEventContainerNoLock(
		const Id& orderId,
		const bool createIfNotExists)
{
	IRSTD_ASSERT(TraderEvent, m_orderEventLock.isScope(), "This operation must executed be under a scope");

	// If the order does not exists yet, create it
	auto it = m_orderEventList.find(orderId);
	if (it == m_orderEventList.end())
	{
		if (!createIfNotExists)
		{
			static OrderEvents orderEvents;
			orderEvents = OrderEvents();
			return orderEvents;
		}
		m_orderEventList.insert({orderId, OrderEvents()});
		it = m_orderEventList.find(orderId);
	}

	IRSTD_ASSERT(TraderEvent, it != m_orderEventList.end(),
			"The order id#" << orderId << " is not created correctly");

	return it->second;
}

// ---- Trader::EventManager (onOrder<event>) ---------------------------------

void Trader::EventManager::onOrderComplete(
		const char* const pName,
		ContextHandle context,
		const Id& orderId,
		const OrderCompleteCallback& callback,
		const Lifetime lifetime)
{
	onOrder("onOrderComplete", &OrderEvents::m_onComplete, pName,
			context, orderId, callback, IrStd::Type::toIntegral(lifetime));
}

void Trader::EventManager::onOrderError(
		const char* const pName,
		ContextHandle context,
		const Id& orderId,
		const OrderErrorCallback& callback,
		const Lifetime lifetime)
{
	onOrder("onOrderError", &OrderEvents::m_onError, pName,
			context, orderId, callback, IrStd::Type::toIntegral(lifetime));
}

void Trader::EventManager::onOrderTimeout(
		const char* const pName,
		ContextHandle context,
		const Id& orderId,
		const OrderTimeoutCallback& callback,
		const Lifetime lifetime)
{
	onOrder("onOrderTimeout", &OrderEvents::m_onTimeout, pName,
			context, orderId, callback, IrStd::Type::toIntegral(lifetime));
}

// ---- Trader::EventManager (triggerOrder<event>) ----------------------------

void Trader::EventManager::triggerOnOrderComplete(
		const TrackOrder& track,
		const IrStd::Type::Decimal amount)
{
	triggerOrder("onOrderComplete", &OrderEvents::m_onComplete, track, amount);
}

void Trader::EventManager::triggerOnOrderError(
		const TrackOrder& track)
{
	triggerOrder("onOrderError", &OrderEvents::m_onError, track);
}

void Trader::EventManager::triggerOnOrderTimeout(
		const TrackOrder& track)
{
	triggerOrder("onOrderTimeout", &OrderEvents::m_onTimeout, track);
}

// ---- Trader::EventManager --------------------------------------------------

void Trader::EventManager::copyOrder(
		const Id orderIdOriginal,
		const Id orderIdNew,
		const Lifetime minLifetimeToKeep)
{
	IRSTD_LOG_TRACE(TraderEvent, "Copying events from order id#" << orderIdOriginal
			<< " to order id#" << orderIdNew);
	{
		auto scope = m_orderEventLock.writeScope();

		auto& containerNew = getOrderEventContainerNoLock(orderIdNew, /*createIfNotExists*/true);
		auto& containerOriginal = getOrderEventContainerNoLock(orderIdOriginal, /*createIfNotExists*/false);
		containerNew.copy(containerOriginal, minLifetimeToKeep);
	}
}

void Trader::EventManager::copyOrder(
		const Id orderId,
		const OrderEvents& events,
		const Lifetime minLifetimeToKeep)
{
	IRSTD_LOG_TRACE(TraderEvent, "Copying events to id#" << orderId);
	{
		auto scope = m_orderEventLock.writeScope();

		auto& container = getOrderEventContainerNoLock(orderId, /*createIfNotExists*/true);
		container.copy(events, minLifetimeToKeep);
	}
}

void Trader::EventManager::garbageCollection(const Exchange& exchange)
{
	// Cleanup events based on orders
	{
		auto scope = m_orderEventLock.writeScope();

		for (auto it = m_orderEventList.begin(); it != m_orderEventList.end();)
		{
			// Check if the order still exists
			bool orderExists = false;
			exchange.getTrackOrderList().each([&](const TrackOrder& track) {
				if (track.getId() == it->first)
				{
					orderExists = true;
				}
			});

			// If the order does not exists, delete the event
			if (!orderExists)
			{
				IRSTD_LOG_DEBUG(TraderEvent, "Deleting event onOrderComplete(id#" << it->first
						<< "), as the referring order does not exists anymore");
				it = m_orderEventList.erase(it);
			}
			else
			{
				it++;
			}
		}
	}
}

void Trader::EventManager::toStream(std::ostream& os) const
{
	os << "Active event(s): " << m_orderEventList.size() << std::endl;
	{
		auto scope = m_orderEventLock.readScope();

		for (const auto& orderEvent : m_orderEventList)
		{
			os << "  Id#" << orderEvent.first << std::endl;
			os << "    - onOrderComplete(";
			orderEvent.second.m_onComplete.toStream(os);
			os << ")" << std::endl;
			os << "    - onOrderError(";
			orderEvent.second.m_onError.toStream(os);
			os << ")" << std::endl;
			os << "    - onOrderTimeout(";
			orderEvent.second.m_onTimeout.toStream(os);
			os << ")" << std::endl;
		}
	}
}

std::ostream& operator<<(std::ostream& os, const Trader::EventManager& eventManager)
{
	eventManager.toStream(os);
	return os;
}
