#pragma once

#include <list>
#include "IrStd/IrStd.hpp"

#include "Trader/Generic/Event/Context.hpp"

IRSTD_TOPIC_USE(Trader, Event);

namespace Trader
{
	template<class T>
	class EventContainer
	{
	public:
		EventContainer& operator=(const EventContainer& eventContainer)
		{
			copy(eventContainer);
			return *this;
		}

		/**
		 * Merge \ref eventContainer into this
		 */
		void copy(const EventContainer& eventContainer, const size_t minLevel = 0)
		{
			m_container.clear();
			eventContainer.each([&](const std::string& name, ContextHandle context, const T& callback, const size_t level) {
				add(name, context, callback, level);
			}, minLevel);
		}

		/**
		 * Add a new event to the list
		 */
		void add(const std::string& name, const T callback, const size_t level)
		{
			m_container.push_back({name, ContextHandle(), callback, level});
		}
		void add(const std::string& name, ContextHandle context, const T callback, const size_t level)
		{
			m_container.push_back({name, context, callback, level});
		}

		/**
		 * This function triggers all the events associated with the entry
		 *
		 * \return true if all the events are consumed. This should trigger
		 * the deletion of the event list.
		 */
		template<class ...  Args>
		void execute(Args&& ... args)
		{
			each([&](const std::string& /*name*/, ContextHandle context, const T& callback, const size_t /*level*/) {
				callback(context, std::forward<Args>(args)...);
			});
		}

		/**
		 * Print the event list
		 */
		void toStream(std::ostream& os) const
		{
			bool isFirst = true;
			os << "list=[";
			each([&](const std::string& name, ContextHandle context, const T&, const size_t level) {
				if (!isFirst)
				{
					os << ", ";
				}
				os << name << "(context=";
				if (context)
				{
					os << context->getId();
				}
				else
				{
					os << "<invalid>";
				}
				os << ", level=" << level << ")";
				isFirst = false;
			});
			os << "]";
		}

		/**
		 * Iterate through the events
		 */
		void each(std::function<void(const std::string&, ContextHandle, const T&, const size_t)> callback,
				const size_t minLevel = 0) const
		{
			for (const auto& event : m_container)
			{
				if (event.m_level >= minLevel)
				{
					callback(event.m_name, event.m_context, event.m_callback, event.m_level);
				}
			}
		}

	private:
		friend class EventManager;
		struct Item
		{
			std::string m_name;
			ContextHandle m_context;
			T m_callback;
			/// Level of this entry, this is used to filter the entries
			size_t m_level;
		};
		std::list<Item> m_container;
	};
}
