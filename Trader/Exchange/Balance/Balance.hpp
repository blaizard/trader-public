#pragma once

#include <map>
#include <atomic>
#include <ostream>
#include <mutex>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Currency/Currency.hpp"

namespace Trader
{
	class Exchange;
	class Balance
	{
	public:
		static constexpr double INVALID_AMOUNT = -123456789;

		Balance();
		Balance(Exchange& exchange);

		/// Move constructor
		Balance(Balance&& rhs);
		Balance& operator=(Balance&& rhs);

		/**
		 * Set the funds of the balance without touching the reserve
		 */
		void setFunds(const Balance& balance);

		/**
		 * Set the balance value and update the reserve
		 */
		void setFundsAndUpdateReserve(const Balance& balance,
				const Exchange& exchange, const bool isBalanceincludeReserve);

		/**
		 * \brief Compare the current balance with another and return the differences
		 */
		void compareFunds(const Balance& balance, const std::function<void(const CurrencyPtr, const IrStd::Type::Decimal)>& callback) const noexcept;

		/**
		 * \brief Completly clear the balance
		 */
		void clear() noexcept;

		/**
		 * \brief Set funds
		 */
		void set(const CurrencyPtr currency, const IrStd::Type::Decimal amount);

		/**
		 * \brief Add funds to the existing balance
		 */
		void add(const CurrencyPtr currency, const IrStd::Type::Decimal amount);

		/**
		 * \brief Reserve a specific amount of th eexisting balance
		 */
		void reserve(const CurrencyPtr currency, const IrStd::Type::Decimal amount);

		/**
		 * \brief update the reserve from the exchange
		 */
		void updateReserve(const Exchange& exchange);

		/**
		 * Tell whether or not the balance is empty
		 */
		bool empty() const noexcept;

		/**
		 * \brief Retrieve an amount of currency.
		 * This is the default function to be used, as it
		 * does not include the reserve amount.
		 */
		IrStd::Type::Decimal get(const CurrencyPtr currency) const noexcept;

		/**
		 * \brief Retrieve an amount of currency including the reserve.
		 */
		IrStd::Type::Decimal getWithReserve(const CurrencyPtr currency) const noexcept;

		/**
		 * \brief Estimate the value of the balance
		 */
		IrStd::Type::Decimal estimate(const Exchange* const pExchange) const noexcept;
		IrStd::Type::Decimal estimate(const CurrencyPtr currency, const IrStd::Type::Decimal amount,
				const Exchange* const pExchange) const noexcept;

		/**
		 * List all availabel currencies
		 */
		void getCurrencies(const std::function<void(const CurrencyPtr)>& callback) const noexcept;

		void toStream(std::ostream& out) const;

		void toJson(IrStd::Json& json, const Exchange* const pExchange = nullptr) const;

	private:
		void reserveNoLock(const CurrencyPtr currency, const IrStd::Type::Decimal amount);
		IrStd::Type::Decimal getWithReserveNoLock(const CurrencyPtr currency) const noexcept;

		// Main lock to allow multithread access
		mutable IrStd::RWLock m_lock;

		std::map<CurrencyPtr, IrStd::Type::Decimal> m_fundList;
		std::map<CurrencyPtr, IrStd::Type::Decimal> m_reservedFundList;
		Exchange* m_pExchange;

		// Related to currency estimates
		mutable IrStd::Type::Decimal m_initialEstimate;
	};
}

std::ostream& operator<<(std::ostream& os, Trader::Balance& balance);
