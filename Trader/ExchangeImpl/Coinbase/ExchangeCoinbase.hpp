#pragma once

#include "Trader/Exchange/Exchange.hpp"

#include <string>
#include <memory>

namespace Trader
{
	class ExchangeCoinbase : public Trader::Exchange
	{
	public:
		ExchangeCoinbase();

		void updatePropertiesImpl(PairTransactionMap& transactionMap) override;
		void updateRatesSpecificCurrencyImpl(const CurrencyPtr currency) override;

	private:
		IrStd::RWLock m_lockProperties;
		std::map<std::string, CurrencyPtr> m_stringToCurrency;
		std::map<CurrencyPtr, std::string> m_currencyToString;
	};
}
