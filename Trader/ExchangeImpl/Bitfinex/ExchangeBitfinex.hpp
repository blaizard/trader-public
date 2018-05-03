#pragma once

#include "Trader/Exchange/Exchange.hpp"

#include <string>
#include <memory>

namespace Trader
{
	class ExchangeBitfinex : public Trader::Exchange
	{
	public:
		ExchangeBitfinex();

		void updateRatesImpl() override;
		void updatePropertiesImpl(PairTransactionMap& transactionMap) override;

	private:
		std::string m_tickerUrl;
	};
}
