#pragma once

#include "Trader/Exchange/Exchange.hpp"

IRSTD_TOPIC_USE(Trader, Exchange);

namespace Trader
{
	template<class T>
	class ExchangeReadOnly : public T
	{
	public:
		template<class ... Args>
		ExchangeReadOnly(Args&& ... args)
				: T(std::forward<Args>(args)...)
		{
			this->m_configuration.setReadOnly(true);
		}

		void setOrderImpl(const Order& /*order*/, const IrStd::Type::Decimal /*amount*/, std::vector<Id>& /*idList*/) override final
		{
			IRSTD_CRASH(IRSTD_TOPIC(Trader, Exchange), this->getId()
					<< " is configured as READONLY, calling Exchange::setOrderImpl is forbidden");
		}

		void withdrawImpl(const CurrencyPtr /*currency*/, const IrStd::Type::Decimal /*amount*/) override final
		{
			IRSTD_CRASH(IRSTD_TOPIC(Trader, Exchange), this->getId()
					<< " is configured as READONLY, calling Exchange::withdrawImpl is forbidden");
		}

		void updateOrdersImpl(std::vector<TrackOrder>& /*trackOrders*/) override final
		{
			IRSTD_CRASH(IRSTD_TOPIC(Trader, Exchange), this->getId()
					<< " is configured as READONLY, calling Exchange::updateOrdersImpl is forbidden");
		}

		void cancelOrderImpl(const TrackOrder& /*order*/) override final
		{
			IRSTD_CRASH(IRSTD_TOPIC(Trader, Exchange), this->getId()
					<< " is configured as READONLY, calling Exchange::cancelOrderImpl is forbidden");
		}

		void updateBalanceImpl(Balance& /*balance*/) override final
		{
			IRSTD_CRASH(IRSTD_TOPIC(Trader, Exchange), this->getId()
					<< " is configured as READONLY, calling Exchange::updateBalanceImpl is forbidden");
		}
	};
}
