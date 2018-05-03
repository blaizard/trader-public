#pragma once

#include <iomanip>

#include "Trader/Exchange/Order/Order.hpp"
#include "Trader/Exchange/Balance/Balance.hpp"
#include "Trader/Generic/Event/Context.hpp"
#include "Trader/Generic/Id/Id.hpp"

namespace Trader
{
	class TrackOrder
	{
	public:
		TrackOrder(const Id id, const Order& order, const IrStd::Type::Decimal amount,
				const IrStd::Type::Timestamp creationTime = IrStd::Type::Timestamp::now())
				: m_id(id)
				, m_order(order)
				, m_type(identifyOrderType(order))
				, m_amount(amount)
				, m_creationTime(creationTime)
		{
			// The order is copied and hance we need to set a fixed rate when tracking this order
			// to make sure it does not vary accross the operation.
			// Also make sure we use the max rate to maximize the profitability.
			m_order.setRate(std::max(m_order.getRate(), m_order.getTransaction()->getRate()));
		}

		TrackOrder(const Id id, std::shared_ptr<Transaction> transaction, const IrStd::Type::Decimal rate,
				const IrStd::Type::Decimal amount, const IrStd::Type::Timestamp timestamp = IrStd::Type::Timestamp::now())
				: TrackOrder(id, Order{transaction, rate}, amount, timestamp)
		{
		}

		TrackOrder(const Order& order, const IrStd::Type::Decimal amount, const IrStd::Type::Timestamp timestamp = IrStd::Type::Timestamp::now())
				: TrackOrder(Trader::Id::unique(), order, amount, timestamp)
		{
		}

		// Return the type of order
		enum class Type
		{
			LIMIT,
			MARKET,
			WITHDRAW
		};
		Type getType() const noexcept;
		const char* getTypeToString() const noexcept;
		static const char* getTypeToString(const Type type) noexcept;

		/**
		 * Identify the order type
		 */
		static Type identifyOrderType(const Order& order) noexcept;

		/**
		 * Make sure a track order is valid
		 */
		bool isValid() const noexcept;

		/**
		 * Attach a context to the order. A context can be attached only if none is present.
		 * Thsi to ensure that only one context is used per order.
		 */
		void setContext(ContextHandle context) noexcept;
		ContextHandle getContext() const noexcept;

		template<class T>
		ContextHandleImpl<T> getContext() const noexcept
		{
			IRSTD_ASSERT(m_context);
			return m_context.cast<T>();
		}

		bool operator==(const TrackOrder& track) const noexcept
		{
			return m_id == track.m_id
					&& m_amount == track.m_amount
					&& m_creationTime == track.m_creationTime
					&& m_order == track.m_order
					&& m_type == track.m_type;
		}

		bool operator!=(const TrackOrder& track) const noexcept
		{
			return !(*this == track);
		}

		void toStream(std::ostream& os) const;

		/**
		 * Return true if a timeout needs to occur
		 */
		bool isTimeout(const IrStd::Type::Timestamp current) const noexcept
		{
			return (current > (m_creationTime + IrStd::Type::Timestamp::s(m_order.getTimeout())));
		}

		void setCreationTime(const IrStd::Type::Timestamp timestamp) noexcept
		{
			m_creationTime = timestamp;
		}

		/**
		 * \brief Return the timestamp when the order has been created
		 */
		IrStd::Type::Timestamp getCreationTime() const noexcept;

		const Order& getOrder() const noexcept;
		Order& getOrderForWrite() noexcept;
		IrStd::Type::Decimal getAmount() const noexcept;
		IrStd::Type::Decimal getRate() const noexcept;

		Id getId() const noexcept;
		std::string getIdForTrace() const noexcept;

		void setAmount(const IrStd::Type::Decimal amount) noexcept;

	private:
		friend class TrackOrderList;

		void setId(const Id id) noexcept;

		/// Order identifier
		Id m_id;
		/// The order itself
		Order m_order;
		/// Order type
		Type m_type;
		/// Amount of currency reserved for the order
		IrStd::Type::Decimal m_amount;
		/// When the order has been created
		IrStd::Type::Timestamp m_creationTime;
		/// The context associated with this order. this is mainly used to link multiple
		/// orders togethers with a unique context.
		ContextHandle m_context;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::TrackOrder& track);
