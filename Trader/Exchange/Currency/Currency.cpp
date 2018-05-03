#include <cstring>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Currency/Currency.hpp"

IRSTD_TOPIC_REGISTER(Trader, Currency);
IRSTD_TOPIC_USE_ALIAS(TraderCurrency, Trader, Currency);

#define TRADER_CURRENCY_REGISTER(id, ...) \
		static const Trader::CurrencyImpl IRSTD_PASTE(id, __)(#id, __VA_ARGS__); \
		const CurrencyPtr id = &IRSTD_PASTE(id, __);

namespace Trader
{
	namespace Currency
	{
		// This is the none currency (or not set)
		static const Trader::CurrencyImpl NONE__("-", "None", {}, /*isFiat*/false);
		const CurrencyPtr NONE = &NONE__;

		// Register all supported currencies here 
		// Minimal amounts are taken from:
		// - https://support.kraken.com/hc/en-us/articles/205893708-What-is-the-minimum-order-size-
		TRADER_CURRENCY_REGISTER(BCC, "BitConnect", {"bcc"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(BCH, "Bitcoin Cash", {"bch"}, /*isFiat*/false, /*minAmount*/0.002);
		TRADER_CURRENCY_REGISTER(BCU, "Bitcoin Unlimited", {"bcu"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(BTC, "Bitcoin", {"btc", "xbt"}, /*isFiat*/false, /*minAmount*/0.002);
		TRADER_CURRENCY_REGISTER(CAD, "Canadian Dollar", {"cad"}, /*isFiat*/true, /*minAmount*/1);
		TRADER_CURRENCY_REGISTER(CNH, "Chinese Yuan Renminbi", {"cnh"}, /*isFiat*/true, /*minAmount*/1);
		TRADER_CURRENCY_REGISTER(DASH, "Dash", {"dash"}, /*isFiat*/false, /*minAmount*/0.03);
		TRADER_CURRENCY_REGISTER(DAO, "The Dao", {"dao"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(DOGE, "DogeCoin", {"xdg"}, /*isFiat*/false, /*minAmount*/3000);
		TRADER_CURRENCY_REGISTER(DSH, "Dashcoin", {"dsh"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(EOS, "EOS", {"eos"}, /*isFiat*/false, /*minAmount*/3);
		TRADER_CURRENCY_REGISTER(ETH, "Ethereum", {"eth"}, /*isFiat*/false, /*minAmount*/0.02);
		TRADER_CURRENCY_REGISTER(ETC, "Ethereum Classic", {"etc"}, /*isFiat*/false, /*minAmount*/0.3);
		TRADER_CURRENCY_REGISTER(EUR, "Euro", {"eur"}, /*isFiat*/true, /*minAmount*/1);
		TRADER_CURRENCY_REGISTER(FTC, "Feathercoin", {"ftc"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(GBP, "British Pound", {"gbp"}, /*isFiat*/true);
		TRADER_CURRENCY_REGISTER(GNO, "Gnosis", {"gno"}, /*isFiat*/false, /*minAmount*/0.03);
		TRADER_CURRENCY_REGISTER(ICN, "Iconomi", {"icn"}, /*isFiat*/false, /*minAmount*/2);
		TRADER_CURRENCY_REGISTER(IOT, "IOTA", {"iot"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(LTC, "Litecoin", {"ltc"}, /*isFiat*/false, /*minAmount*/0.1);
		TRADER_CURRENCY_REGISTER(MLN, "Melon", {"mln"}, /*isFiat*/false, /*minAmount*/0.1);
		TRADER_CURRENCY_REGISTER(NMC, "Namecoin", {"nmc"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(NVC, "Novacoin", {"nvc"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(OMG, "OmiseGo", {"omg"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(PPC, "Peercoin", {"ppc"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(REP, "Augur", {"rep"}, /*isFiat*/false, /*minAmount*/0.3);
		TRADER_CURRENCY_REGISTER(RRT, "Recovery Right Tokens", {"rrt"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(RUR, "Russian Ruble", {"rur"}, /*isFiat*/true);
		TRADER_CURRENCY_REGISTER(SAN, "Santiment Network Token", {"san"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(TRC, "Terra", {"trc"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(USD, "US Dollar", {"usd"}, /*isFiat*/true, /*minAmount*/1);
		TRADER_CURRENCY_REGISTER(USDT, "Tether", {"usdt"}, /*isFiat*/false, /*minAmount*/5);
		TRADER_CURRENCY_REGISTER(XLM, "Stellar Lumens", {"xlm"}, /*isFiat*/false, /*minAmount*/300);
		TRADER_CURRENCY_REGISTER(XMR, "Monero", {"xmr"}, /*isFiat*/false, /*minAmount*/0.1);
		TRADER_CURRENCY_REGISTER(XOT, "Internet of Things", {"xot"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(XPM, "Primecoin", {"xpm"}, /*isFiat*/false);
		TRADER_CURRENCY_REGISTER(XRP, "Ripple", {"xrp"}, /*isFiat*/false, /*minAmount*/30);
		TRADER_CURRENCY_REGISTER(ZEC, "Zcash", {"zec"}, /*isFiat*/false, /*minAmount*/0.03);
	}
}

// ---- Trader::Currency ------------------------------------------------------

Trader::CurrencyPtr Trader::Currency::discover(const char* const pStr)
{
	CurrencyPtr currencyList[] = {TRADER_CURRENCY_LIST};

	const CurrencyImpl* pCandidate = nullptr;
	for (auto* pCurrency : currencyList)
	{
		for (auto* match : pCurrency->getMatches())
		{
			if (::strcasecmp(pStr, match) == 0)
			{
				IRSTD_THROW_ASSERT(TraderCurrency, !pCandidate, "More than one currency matches '"
						<< pStr << "' (currency1=" << pCandidate << ", currency2="
						<< pCurrency << ")");
				pCandidate = pCurrency;
			}
		}
	}
	return pCandidate;
}

std::pair<Trader::CurrencyPtr, Trader::CurrencyPtr> Trader::Currency::tickerToCurrency(
		const char* const pSymbol1, const char* const pSymbol2)
{
	auto currency1 = Currency::discover(pSymbol1);
	auto currency2 = Currency::discover(pSymbol2);

	if (!currency1 || !currency2)
	{
		IRSTD_LOG_TRACE(TraderCurrency, "Cannot find matches for '"
			<< pSymbol1 << "' and/or '" << pSymbol2 << "'");
		return {nullptr, nullptr};
	}

	return {currency1, currency2};
}

std::pair<Trader::CurrencyPtr, Trader::CurrencyPtr> Trader::Currency::tickerToCurrency(
		const char* const pTicker, const size_t symbolLength)
{
	const std::string pair(pTicker);
	const std::string currency1Str = pair.substr(0, symbolLength);
	const std::string currency2Str = pair.substr(symbolLength);

	return tickerToCurrency(currency1Str.c_str(), currency2Str.c_str());
}

std::pair<Trader::CurrencyPtr, Trader::CurrencyPtr> Trader::Currency::tickerToCurrency(
		const char* const pTicker, const char delimiter)
{
	const std::string pair(pTicker);
	const size_t pos = pair.find(delimiter);
	IRSTD_THROW_ASSERT(TraderCurrency, pos != std::string::npos, "Character '"
			<< delimiter << "' cannot be identified in the currency pair '" << pair << "'");

	const std::string currency1Str = pair.substr(0, pos);
	const std::string currency2Str = pair.substr(pos + 1);

	return tickerToCurrency(currency1Str.c_str(), currency2Str.c_str());
}

// ---- Trader::CurrencyPtr --------------------------------------------------

std::ostream& operator<<(std::ostream& os, Trader::CurrencyPtr currency)
{
	os << currency->getId();
	return os;
}
