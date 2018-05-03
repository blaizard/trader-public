#pragma once

#include "Trader/Exchange/Exchange.hpp"

#include <string>
#include <memory>

namespace Trader
{
	class ExchangeWex : public Trader::Exchange
	{
	public:
		ExchangeWex(const char* const key = nullptr, const char* const secret = nullptr);

		void updateBalanceImpl(Balance& balance) override;
		void updatePropertiesImpl(PairTransactionMap& /*transactionMap*/) override;
		void updateRatesImpl() override;
		void updateOrdersImpl(std::vector<TrackOrder>& trackOrders) override;
		void setOrderImpl(const Order& order, const IrStd::Type::Decimal amount, std::vector<Id>& idList) override;
		void cancelOrderImpl(const TrackOrder& order) override;

	private:
		void makeFetchForPrivateAPI(IrStd::FetchUrl& fetch, const char* const command);
		IrStd::Json sanityCheckPrivateAPIResponse(const std::string& response);

		std::string m_tickerUrl;

		bool m_terminate;
		const char* const m_key;
		const char* const m_secret;
		size_t m_nonce;
	};
}
