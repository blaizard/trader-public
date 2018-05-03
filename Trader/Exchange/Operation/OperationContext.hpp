#pragma once

#include <atomic>

#include "Trader/Generic/Id/Id.hpp"
#include "Trader/Generic/Event/Context.hpp"
#include "Trader/Generic/Event/EventContainer.hpp"
#include "Trader/Exchange/Currency/Currency.hpp"

namespace Trader
{
	class TrackOrder;
	class Operation;
	class Strategy;
	class OperationContext : public Context
	{
	public:
		typedef std::function<void(const OperationContext&)> CompleteCallback;

		enum class FailureCause
		{
			NONE = 0,
			TIMEOUT,
			PLACE_ORDER
		};

		OperationContext(const Strategy& strategy);
		virtual ~OperationContext();

		void toStream(std::ostream& os) const override;

		/**
		 * Get the corresponding strategy Id
		 */
		Id getStrategyId() const noexcept;

		/**
		 * Return the current profit of the context
		 */
		void getProfit(const std::function<void(const CurrencyPtr, const IrStd::Type::Decimal)>& callback) const noexcept;

		/**
		 * This sets the expected profit of the operation on completion
		 */
		void setProfitOnComplete(const IrStd::Type::Decimal profitPercent) noexcept;

		/**
		 * Attach an event to the completion handler.
		 * Thi function is called regardless if the operation succeed or not.
		 */
		void onComplete(const char* const pName, CompleteCallback callback);

		/**
		 * Get the failure cause of the operation
		 */
		FailureCause getFailureCause() const noexcept;

		/**
		 * Tells if an operation has processed anything
		 */
		bool isEffective() const noexcept;

		const std::string getDescription() const noexcept;

	protected:
		/**
		 * Triggers the event onComplete
		 */
		void triggerOnComplete();

		/**
		 * Set profit to the context
		 */
		void setProfit(const CurrencyPtr currency, const IrStd::Type::Decimal profit) noexcept;
		void addProfit(const CurrencyPtr currency, const IrStd::Type::Decimal profit) noexcept;

		/**
		 * Need to be updated to reflect the current status of the order
		 */
		std::string m_description;

	private:
		friend class Exchange;
		friend class Operation;

		typedef std::function<void(ContextHandle&, const OperationContext&)> InternalCompleteCallback;

		static void applyProfit(ContextHandle& context, const TrackOrder& trackOrder,
				const IrStd::Type::Decimal amountProceed);

		/**
		 * Set a failure cuase to the operation
		 */
		void setFailureCause(const FailureCause cause) noexcept;

		const Id m_strategyId;

		// Monitor the failure
		FailureCause m_failureCause;

		EventContainer<InternalCompleteCallback> m_onComplete;
		struct Profit
		{
			IrStd::Type::Decimal m_fixed;
			IrStd::Type::Decimal m_accumulated;
		};
		std::map<const CurrencyPtr, Profit> m_profitMap;
		mutable IrStd::RWLock m_lock;
		IrStd::Type::Decimal m_profitRatio;
	};

	typedef ContextHandleImpl<OperationContext> OperationContextHandle;
}
