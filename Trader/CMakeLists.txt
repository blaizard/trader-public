set(trader_sources
	ExchangeImpl/Wex/ExchangeWex.cpp
	ExchangeImpl/Test/ExchangeTest.cpp
	ExchangeImpl/Coinbase/ExchangeCoinbase.cpp
	ExchangeImpl/Bitfinex/ExchangeBitfinex.cpp
	ExchangeImpl/Bitstamp/ExchangeBitstamp.cpp
	ExchangeImpl/Kraken/ExchangeKraken.cpp
	Exchange/Balance/Balance.cpp
	Exchange/Balance/BalanceMovements.cpp
	Exchange/Currency/Currency.cpp
	Exchange/Exchange.cpp
	Exchange/Order/Order.cpp
	Exchange/Order/TrackOrder.cpp
	Exchange/Order/TrackOrderList.cpp
	Exchange/Transaction/Transaction.cpp
	Exchange/Transaction/Boundaries.cpp
	Exchange/Transaction/PairTransaction.cpp
	Exchange/Transaction/PairTransactionMap.cpp
	Exchange/Transaction/WithdrawTransaction.cpp
	Exchange/Event/EventManager.cpp
	Exchange/Operation/Operation.cpp
	Exchange/Operation/OperationOrder.cpp
	Exchange/Operation/OperationContext.cpp
	Strategy/SwingTrading/SwingTrading.cpp
	Strategy/Dummy/Dummy.cpp
	Strategy/Strategy.cpp
	Server/Server.cpp
	Server/EndPoint/Exchange.cpp
	Server/EndPoint/Strategy.cpp
	Server/EndPoint/Manager.cpp
	Manager/Manager.cpp
	Generic/Id/Id.cpp
	Generic/Event/Context.cpp
)

add_subdirectory(tests)

add_compile_options(
		-Wall
		-Wextra
#		-Wfloat-equal
#		-Wshadow
		-Wpointer-arith
		-Wcast-align
		-Wstrict-overflow=2
		-Wwrite-strings
		-Wcast-qual
		-Wswitch-default
		-Wswitch-enum
#		-Wconversion
		-Wunreachable-code
		-Werror)

add_library(trader ${trader_sources})
target_link_libraries(trader irstd atomic)
