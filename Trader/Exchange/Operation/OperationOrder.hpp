#pragma once

#include "Trader/Exchange/Operation/Operation.hpp"

namespace Trader
{
	class Strategy;
	class OperationOrder : public Operation
	{
	public:
		OperationOrder(const Order& order, const IrStd::Type::Decimal amount, const Strategy& strategy);

	protected:
		static void monitorProfit(ContextHandle& contextProceed, const TrackOrder& trackProceed,
				const IrStd::Type::Decimal amountProceed);
	};
}
