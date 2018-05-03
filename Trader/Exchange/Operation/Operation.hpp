#pragma once

#include "Trader/Exchange/Event/EventManager.hpp"
#include "Trader/Exchange/Order/TrackOrder.hpp"
#include "Trader/Exchange/Operation/OperationContext.hpp"

namespace Trader
{
	/**
	 * Operation is a container holding all important information
	 * for an order or any other exchange related operation.
	 * It combines together, the order, the context and the events.
	 */
	class Operation
	{
	public:
		Operation(const Order& order, const IrStd::Type::Decimal amount, const OperationContextHandle& context);

		/**
		 * Set a function that will be called once the order associated with the operation is partially or fully completed
		 */
		void onOrderComplete(const char* const pDescription, const EventManager::OrderCompleteCallback& callback,
				const EventManager::Lifetime lifetime);

		/**
		 * Set a function that will be called once the order associated with the operation faces an error
		 */
		void onOrderError(const char* const pDescription, const EventManager::OrderErrorCallback& callback,
				const EventManager::Lifetime lifetime);

		/**
		 * Set a function that will be called once the order associated with the operation faces an timeout
		 */
		void onOrderTimeout(const char* const pDescription, const EventManager::OrderTimeoutCallback& callback,
				const EventManager::Lifetime lifetime);

		/**
		 * Set a function that will be called once the operation completes
		 */
		void onComplete(const char* const pDescription, const OperationContext::CompleteCallback& callback);

	private:
		friend class Exchange;
		friend class Strategy;

		static void recordTransaction(ContextHandle& contextProceed, const TrackOrder& trackProceed,
				const IrStd::Type::Decimal amountProceed);

		const Order m_order;
		IrStd::Type::Decimal m_amount;
		OperationContextHandle m_context;

		EventManager::OrderEvents m_events;
	};
}

// Implementations
#include "OperationOrder.hpp"
