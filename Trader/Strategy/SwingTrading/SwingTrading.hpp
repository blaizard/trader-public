#pragma once

#include "Trader/Strategy/Strategy.hpp"

namespace Trader
{
	class SwingTrading : public Strategy
	{
	public:
		SwingTrading(const IrStd::Type::Gson::Map& config);

		void initializeImpl() override;
		void processImpl(const size_t counter) override;
	};
}
