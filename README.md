## Trader

Trader is a flexible pair trading platform. It is developped to run custom strategies on crypto currencies exchanges. The backend is written in C++11 while the frontend is in Javascript and based on Vue.js. 

### Screenshot

<div style="text-align:center"><img src="https://github.com/blaizard/trader/raw/master/docs/frontend.png" width="50%" alt="Front-end screenshot"/></div>

## Features

* Robust order tracking (support exchanges with buggy APIs)
* Chainable orders in all sorts (for example: market -> limit -> withdraw)
* Allow multiple custom strategies to run in parallel
* Support market orders, limit orders and withdrawing
* Support for REST API, Websocket (Pusher) and Json parsing
* Responsive HTML front-end (can optionally be used with a custom frontend through a REST API interface)
* Record rates, transactions and profits

### Implemented Exchanges

| Exchange | Support | Withdraw | Note |
| -------- |:-------:|:--------:| ---- |
| <a href="https://www.bitfinex.com">Bitfinex</a> | Read Only (1) | | |
| <a href="https://www.bitstamp.net">Bitstamp</a> | Read Only (1) | | |
| <a href="https://www.coinbase.com">Coinbase</a> | Read Only (1) | | |
| <a href="https://www.kraken.com">Kraken</a> | Full | ✓ | |
| <a href="https://wex.nz">Wex</a> | Full | | |
| Test | Full | | A dummy exchange for test purpose only |

(1) Only supports read-only activities, any private functionality such as
order placing, balance inquiry, etc are disabled.

## Disclaimer

__USE THE SOFTWARE AT YOUR OWN RISK. YOU ARE RESPONSIBLE FOR YOUR OWN MONEY. PAST PERFORMANCE IS NOT NECESSARILY INDICATIVE OF FUTURE RESULTS.__
__THE AUTHORS AND ALL AFFILIATES ASSUME NO RESPONSIBILITY FOR YOUR TRADING RESULTS.__

## Compile & Run

### Prerequisites

You need the following libraries:
* <a href="https://www.openssl.org/source" target="_blank">OpenSSL</a> (libssl1.0-dev)
* <a href="http://curl.haxx.se" target="_blank">cURL</a> (libcurl4-openssl-dev)

C++ requires g++ version 5.3 or later.

### Get the sources

Download the source from github with:
```
git clone git://github.com/blaizard/trader.git
```

### Build

A script is available to automate the build, it will also fetch dependant git submodules.

First you need to intialize the source directory. To do so, simply run the following command from the root directory:
```
./build.sh -s
```

Then the followign will compile the source code:
```
./build.sh release
```

### Run

All binaries generated are located within the build/bin/ directory.
```
./build/bin/main
```

## Output

Trader logs various data that can be found in the output directory (by default within the build/bin/output sub-directory).
This consists of the following records:
* transactions.csv: Contains all transactions, with their timestamp, type and rates.
* profit.csv: Contains all recorded profits from the strategies in place.
* /[echanges]/: Sub-directories containing all the rate changes in CSV format for all available currency pairs for the various exchanges.

## Todo

* Block balance changes for a certain period of time to ensure fund availability for the next order (implemented/in testing)
* Disable pair if it did not get any update (or if update is invalid, rate equals 0 for example)
* Improve the unit test coverage