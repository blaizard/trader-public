#pragma once

#include "Trader/Exchange/Exchange.hpp"

#include <string>
#include <memory>

namespace Trader
{
	class ExchangeKraken : public Trader::Exchange
	{
	public:
		ExchangeKraken(const char* const key = nullptr, const char* const secret = nullptr,
				const std::initializer_list<std::pair<CurrencyPtr, const std::string>> withdraw = {});

		void updatePropertiesImpl(PairTransactionMap& transactionMap) override;
		void updateRatesImpl() override;
		void updateBalanceImpl(Balance& balance) override;
		void updateOrdersImpl(std::vector<TrackOrder>& trackOrders) override;

		void setOrderImpl(const Order& order, const IrStd::Type::Decimal amount, std::vector<Id>& idList) override;
		void cancelOrderImpl(const TrackOrder& /*order*/) override;
		void withdrawImpl(const CurrencyPtr currency, const IrStd::Type::Decimal amount) override;

	private:
		void makeFetchForPrivateAPI(IrStd::FetchUrl& fetch, const char* const pUri) const;
		void sanityCheckResponse(const IrStd::Json& json, const std::function<void(std::stringstream&)>& onError = {}) const;

		CurrencyPtr getCurrencyFromId(const char* const currencyId) const;
		std::string getIdFromCurrency(Trader::CurrencyPtr currency) const;
		std::shared_ptr<PairTransaction> getTransactionFromId(const char* const pairId) const;

		const char* const m_key;
		const std::string m_decodedSecret;
		const std::initializer_list<std::pair<CurrencyPtr, const std::string>> m_withdrawList;

		// Store pair & currency information
		struct CurrencyInfo
		{
			CurrencyPtr m_currency;
			std::string m_name;
			size_t m_decimal;
		};
		std::map<std::string, CurrencyInfo> m_currencyMap;
		std::map<std::string, std::shared_ptr<PairTransaction>> m_pairMap;
		mutable IrStd::RWLock m_lockProperties;
		std::string m_tickerUrl;
	};
}
