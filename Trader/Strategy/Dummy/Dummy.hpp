#pragma once

#include "Trader/Strategy/Strategy.hpp"

namespace Trader
{
	class Dummy : public Strategy
	{
	public:
		Dummy(const IrStd::Type::Gson::Map& config);

		void initializeImpl() override;
		void processImpl(const size_t counter) override;
	};
}
