#pragma once

#include "Trader/Exchange/Exchange.hpp"

#include <string>
#include <memory>

namespace Trader
{
	class ExchangeBitstamp : public Trader::Exchange
	{
	public:
		ExchangeBitstamp();

		void updatePropertiesImpl(PairTransactionMap& transactionMap);
		//void updateRatesSpecificPairImpl(const CurrencyPtr currency1, const CurrencyPtr currency2) override;
		void updateRatesStartImpl() override;
		void updateRatesStopImpl() override;

	private:
		void websocketReceive(const CurrencyPtr initialCurrency, const CurrencyPtr finalCurrency,
				const std::string& event, const std::string& data) noexcept;

		IrStd::Websocket::Pusher m_pusher;
		struct CurrencyPair
		{
			CurrencyPtr m_currency1;
			CurrencyPtr m_currency2;
			const char* m_ticker;
			const char* m_webscoketChannel;
		};
		const std::array<CurrencyPair, 15> m_availablePairs;
	};
}
