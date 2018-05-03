#pragma once

#include <memory>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Transaction/Transaction.hpp"

namespace Trader
{
	class Order
	{
	private:
		static constexpr double INVALID_RATE = 1.;
		static constexpr size_t DEFAULT_TIMEOUT_S = 24 * 3600; // 1 day

	public:
		Order();
		Order(std::shared_ptr<Transaction> transaction, const IrStd::Type::Decimal specificRate = INVALID_RATE,
				const size_t timeoutS = DEFAULT_TIMEOUT_S);
		// Copy
		Order(const Order& order);
		Order& operator=(const Order& order);

		// Move
		Order(Order&& order);
		Order& operator=(Order&& order);

		/**
		 * \brief Copy the current order.
		 *
		 * \note It will not make a copy of the chained orders,
		 *       but only of the current order.
		 */
		std::unique_ptr<Order> copy(const bool firstOnly = false,
				const bool withFixedRate = false) const noexcept;

		/**
		 * Tell if the rate is fixed (specified) or not.
		 */
		bool isFixedRate() const noexcept;

		const Order* get(const size_t position) const noexcept;
		const Order* getNext() const noexcept;
		void addNext(const Order& order);

		CurrencyPtr getInitialCurrency() const noexcept;
		CurrencyPtr getFinalCurrency() const noexcept;
		CurrencyPtr getFirstOrderFinalCurrency() const noexcept;

		/**
		 * \brief Return the current rate of the first order
		 */
		IrStd::Type::Decimal getRate() const noexcept;
		IrStd::Type::Decimal getRate(const int position) const;

		const Transaction* getTransaction() const noexcept;

		IrStd::Type::Decimal getFirstOrderFinalAmount(const IrStd::Type::Decimal amount, const bool includeFee = true) const noexcept;
		IrStd::Type::Decimal getFinalAmount(const IrStd::Type::Decimal amount, const bool includeFee = true) const noexcept;

		/**
		 * By passing the expected final amount, calculate the inital amount for this order chain
		 */
		IrStd::Type::Decimal getInitialAmount(const IrStd::Type::Decimal amount, const bool includeFee = true) const noexcept;
		IrStd::Type::Decimal getFirstOrderInitialAmount(const IrStd::Type::Decimal amount, const bool includeFee = true) const noexcept;

		IrStd::Type::Decimal getFeeInitialCurrency(const IrStd::Type::Decimal amount) const noexcept;
		IrStd::Type::Decimal getFeeFinalCurrency(const IrStd::Type::Decimal amount) const noexcept;

		/**
		 * Set/Get the order timeout in second
		 */
		void setTimeout(const size_t timeoutS) noexcept;
		size_t getTimeout() const noexcept;

		/**
		 * \brief Ensure that the order itself is valid
		 */
		bool isValid() const noexcept;

		/**
		 * \brief Assess if the order is valid
		 */
		bool isValid(const IrStd::Type::Decimal amount) const noexcept;

		/**
		 * \brief Assess if the first order of the chain is valid 
		 */
		bool isFirstValid(const IrStd::Type::Decimal amount) const noexcept;

		/**
		 * Set the rate of the order
		 */
		void setRate(const IrStd::Type::Decimal rate) noexcept;

		void toStream(std::ostream& out) const;

		/**
		 * \brief Compare 2 orders (only compare the first)
		 */
		bool operator==(const Order& order) const noexcept;

	private:
		std::shared_ptr<Transaction> m_pTransaction;
		std::unique_ptr<Order> m_next;
		IrStd::Type::Decimal m_specificRate;
		/// Timeout in seconds before canceling the order
		size_t m_timeoutS;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::Order& order);
