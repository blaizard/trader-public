#pragma once

#include "Trader/Exchange/Currency/CurrencyImpl.hpp"

#define TRADER_CURRENCY_LIST NONE, BCC, BCH, BCU, BTC, CAD, CNH, DASH, DAO, DOGE, DSH, EOS, ETH, ETC, EUR, \
				FTC, GBP, GNO, ICN, IOT, LTC, MLN, NMC, NVC, OMG, PPC, RRT, RUR, SAN, TRC, USD, USDT, \
				XLM, XMR, XOT, XPM, XRP, ZEC

namespace Trader
{
	typedef const CurrencyImpl* CurrencyPtr;
	namespace Currency
	{
		extern const CurrencyPtr TRADER_CURRENCY_LIST;

		/**
		 * Identify a currency from a string
		 */
		CurrencyPtr discover(const char* const pStr);

		/**
		 * Return the currency pair from the ticker
		 */
		std::pair<Trader::CurrencyPtr, Trader::CurrencyPtr> tickerToCurrency(
				const char* const pTicker, const size_t symbolLength);
		std::pair<Trader::CurrencyPtr, Trader::CurrencyPtr> tickerToCurrency(
				const char* const pSymbol1, const char* const pSymbol2);
		std::pair<Trader::CurrencyPtr, Trader::CurrencyPtr> tickerToCurrency(
				const char* const pTicker, const char delimiter);
	}
}

std::ostream& operator<<(std::ostream& os, Trader::CurrencyPtr currency);
