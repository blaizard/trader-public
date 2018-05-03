#pragma once

// Strategies Implemented
#include "Trader/Strategy/SwingTrading/SwingTrading.hpp"
#include "Trader/Strategy/Dummy/Dummy.hpp"

#include "Trader/Exchange/Currency/Currency.hpp"
#include "Trader/Exchange/Exchange.hpp"
#include "Trader/Exchange/ExchangeMock.hpp"
#include "Trader/Exchange/ExchangeReadOnly.hpp"
#include "Trader/Exchange/Transaction/Transaction.hpp"
#include "Trader/Exchange/Transaction/PairTransaction.hpp"
#include "Trader/Exchange/Transaction/WithdrawTransaction.hpp"

// Exchanges Implemented
#include "Trader/ExchangeImpl/Test/ExchangeTest.hpp"
#include "Trader/ExchangeImpl/Wex/ExchangeWex.hpp"
#include "Trader/ExchangeImpl/Coinbase/ExchangeCoinbase.hpp"
#include "Trader/ExchangeImpl/Bitfinex/ExchangeBitfinex.hpp"
#include "Trader/ExchangeImpl/Bitstamp/ExchangeBitstamp.hpp"
#include "Trader/ExchangeImpl/Kraken/ExchangeKraken.hpp"

#include "Trader/Server/Server.hpp"
#include "Trader/Manager/Manager.hpp"
