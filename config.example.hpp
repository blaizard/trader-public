#pragma once

#define TRADER_HTTP_PORT 8080

#define TRADER_REGISTER_TASKS(trader) \
	{ \
		const auto exchangeId = trader.registerExchangeMock<Trader::ExchangeTest>(); \
		trader.registerStrategy<Trader::Dummy>({ \
			{"exchangeList", {exchangeId.c_str()}} \
		}); \
	}
