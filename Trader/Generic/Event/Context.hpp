#pragma once

#include <memory>

#include "IrStd/IrStd.hpp"

IRSTD_TOPIC_USE(Trader, Context);

namespace Trader
{
	class Context;

	template<class T>
	class ContextHandleImpl : public std::shared_ptr<T>
	{
	public:
		// Forward constructor to shared_ptr and make sure the class is derived from Context
		template<class ... Args>
		ContextHandleImpl(Args&& ... args)
				: std::shared_ptr<T>(std::forward<Args>(args)...)
		{
			IRSTD_ASSERT_CHILDOF(T, Context);
		}

		// Implicit conversion to ContextHandle
		ContextHandleImpl(const ContextHandleImpl<Context>& handle)
				: std::shared_ptr<T>(handle.cast<Context>())
		{
		}

		template<class U>
		ContextHandleImpl<U> cast() const noexcept
		{
			auto casted = std::dynamic_pointer_cast<U>(*this);
			IRSTD_ASSERT(IRSTD_TOPIC(Trader, Context), !(*this) || casted, "The dynamic cast to ContextHandle did not work.");
			return casted;
		}
	};

	typedef ContextHandleImpl<Context> ContextHandle;

	class Context
	{
	public:
		/**
		 * Create a context that can be attached to event handlers
		 */
		template<class T, class ... Args>
		static ContextHandle create(Args&& ... args)
		{
			auto pContext = std::make_shared<T>(std::forward<Args>(args)...);
			return ContextHandle(pContext);
		}

		Context();

		size_t getId() const noexcept;

		virtual ~Context() = default;
		virtual void toStream(std::ostream& os) const;

	private:
		// Unique Id for the context
		const size_t m_id;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::ContextHandle& context);