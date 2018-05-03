#pragma once

#include <memory>
#include <map>
#include <mutex>
#include <set>
#include <list>

#include "IrStd/IrStd.hpp"
#include "Trader/Generic/Event/Context.hpp"
#include "Trader/Generic/Event/EventContainer.hpp"
#include "Trader/Exchange/Currency/Currency.hpp"
#include "Trader/Exchange/Order/TrackOrder.hpp"

IRSTD_TOPIC_USE(Trader, Event);

#define IRSTD_API

namespace Trader
{
	class Exchange;
	class Operation;
	class EventManager
	{
	public:
		typedef std::function<void(ContextHandle& context, const TrackOrder&, const IrStd::Type::Decimal)> OrderCompleteCallback;
		typedef std::function<void(ContextHandle& context, const TrackOrder&)> OrderErrorCallback;
		typedef std::function<void(ContextHandle& context, const TrackOrder&)> OrderTimeoutCallback;

		enum class Lifetime : size_t
		{
			ORDER = 0,
			OPERATION = 1,
			CONTEXT = 2
		};

		// Events related to order id
		struct OrderEvents
		{
			EventContainer<const OrderCompleteCallback> m_onComplete;
			EventContainer<const OrderErrorCallback> m_onError;
			EventContainer<const OrderTimeoutCallback> m_onTimeout;

			void copy(const OrderEvents& events, const Lifetime minLifetimeToKeep);
		};

		EventManager() = default;

		/**
		 * Attach an event to the order completion handler.
		 * Note this handler might be called mulitple times for the same order,
		 * if the order is partially executed.
		 */
		void onOrderComplete(const char* const pName, ContextHandle context,
				const Id& orderId, const OrderCompleteCallback& callback,
				const Lifetime lifetime);
		void onOrderError(const char* const pName, ContextHandle context,
				const Id& orderId, const OrderErrorCallback& callback,
				const Lifetime lifetime);
		void onOrderTimeout(const char* const pName, ContextHandle context,
				const Id& orderId, const OrderTimeoutCallback& callback,
				const Lifetime lifetime);

		/**
		 * Triggers the event onOrderComplete
		 */
		void triggerOnOrderComplete(const TrackOrder& order, const IrStd::Type::Decimal amount);
		void triggerOnOrderError(const TrackOrder& order);
		void triggerOnOrderTimeout(const TrackOrder& order);

		/**
		 * Delete pending events that lost their reference
		 */
		void garbageCollection(const Exchange& exchange);

		/**
		 * Copy the events of one order to another
		 */
		void copyOrder(const Id orderIdOriginal, const Id orderIdNew,
				const Lifetime minLifetimeToKeep);
		void copyOrder(const Id orderId, const OrderEvents& events,
				const Lifetime minLifetimeToKeep);

		void toStream(std::ostream& os) const;

	private:
		/**
		 * Register an order related event
		 */
		template<class Member, class Callback>
		void onOrder(const char* const pEventName, Member member, const char* const pName,
				ContextHandle context, const Id& orderId, const Callback& callback,
				const size_t level)
		{
			{
				auto scope = m_orderEventLock.writeScope();

				auto& container = getOrderEventContainerNoLock(orderId, /*createIfNotExists*/true);
				(container.*member).add(pName, context, callback, level);
				IRSTD_LOG_DEBUG(IRSTD_TOPIC(Trader, Event), "Registering " << pName
						<< "(type=" << pEventName << ", context=" << context
						<< ", level=" << level << ") with order id#" << orderId);
			}
		}

		/**
		 * Trigger an order related event
		 */
		template<class Member, class ... Args>
		void triggerOrder(const char* const pEventName, Member member, const TrackOrder& track, Args&& ... args)
		{
			auto scope = m_orderEventLock.readScope();

			// Check if there is an event related to this order
			const auto it = m_orderEventList.find(track.getId());
			if (it != m_orderEventList.end())
			{
				// Make a copy to ensure that any modification of the event manager in between
				// will not affect the execution of the callback(s)
				auto copyEventContainer = (it->second.*member);
				scope.release();

				IRSTD_LOG_DEBUG(IRSTD_TOPIC(Trader, Event), "Triggering event "
						<< pEventName << "(id#" << track.getId() << ")");
				copyEventContainer.execute(track, std::forward<Args>(args)...);
			}
		}

		OrderEvents& getOrderEventContainerNoLock(const Id& orderId, const bool createIfNotExists);

		std::map<Id, OrderEvents> m_orderEventList;
		mutable IrStd::RWLock m_orderEventLock;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::EventManager& eventManager);
