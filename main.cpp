#include <iostream>
#include <thread>
#include <unistd.h>

#include "IrStd/IrStd.hpp"
#include "Trader/Trader.hpp"

#include "config.hpp"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

IRSTD_TOPIC_USE(IrStd);
IRSTD_TOPIC_USE(Trader);

int mainIrStd()
{
#if defined(TRADER_HTTP_PORT) && defined(TRADER_REGISTER_TASKS)
	IrStd::Logger::getDefault().addTopic(IRSTD_TOPIC(Trader), IrStd::Logger::Level::Info);
	{
		Trader::Manager trader(TRADER_HTTP_PORT);
		TRADER_REGISTER_TASKS(trader);
		trader.start();
	}
#endif

	return 0;
}

int main()
{
	auto& main = IrStd::Main::getInstance();
	main.setVersion(VERSION_MAJOR, VERSION_MINOR);
	return main.call(mainIrStd);
}
