#pragma once

#include "Trader/Exchange/Exchange.hpp"

#include <string>
#include <memory>

namespace Trader
{
	class ExchangeTest : public Trader::Exchange
	{
	private:
		// Configuration
		static constexpr double FEE_PERCENT = 0.2;

	public:
		ExchangeTest();
		void updateRatesImpl() override;
		void updatePropertiesImpl(PairTransactionMap& transactionMap) override;
	};
}
