#pragma once

#include <memory>
#include <ostream>
#include <atomic>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Currency/Currency.hpp"

IRSTD_TOPIC_USE(Trader, Transaction);

namespace Trader
{
	/**
	 * A transaction is an instance of buying or selling something.
	 * Every trasnactions consists of buying/selling an amount of good
	 * to a specific rate.
	 */
	class Transaction
	{
	public:
		/**
		 * \brief Defines the number of previous data points to be recorded per transaction
		 */
		static constexpr size_t NB_RECORDS = 1024;

		Transaction(const CurrencyPtr initialCurrency, const CurrencyPtr finalCurrency);
		Transaction(const Transaction& transaction);

		template<class T>
		T* cast() noexcept
		{
			auto pCasted = dynamic_cast<T*>(this);
			IRSTD_ASSERT(IRSTD_TOPIC(Trader, Transaction), pCasted, "The dynamic cast did not work.");
			return pCasted;
		}

		/**
		 * \brief Compare 2 transactions
		 * They are considered equal only if the initial and final currencies are the same
		 */
		bool operator==(const Transaction& transaction) const noexcept;
		bool operator!=(const Transaction& transaction) const noexcept;

		/**
		 * \brief Set the rate of the transaction
		 *
		 * The rate of the transaction i supdated only if the timestamp are different
		 */
		void setRate(const IrStd::Type::Decimal rate, const IrStd::Type::Timestamp timestamp = IrStd::Type::Timestamp::now());

		/**
		 * \brief Return the rate of the transaction
		 * 
		 * The rate is the equivalence of currency2 for 1 unit of currency1:
		 * 1 currency1 = rate currency2
		 * For example, for transaction BTC/USD if the rate is 100,
		 * 1 BTC = 100 USD
		 *
		 * \note The rate does not include the transaction fee
		 */
		IrStd::Type::Decimal getRate() const noexcept;

		/**
		 * Get the previous rate for this transaction
		 */
		IrStd::Type::Decimal getRate(const int position) const;

		/**
		 * Return the number of rates recorded (available with getRate)
		 */
		size_t getNbRates() const noexcept;

		/**
		 * \brief Retrieves the rates between 2 timestamps
		 *
		 * \param fromTimestamp The newset timestamp from when we need the timestamps
		 * \param toTimestamp The oldest timestamp until when it should stop
		 *
		 * \return true if all the samples asked have been delivered, false otherwise.
		 *         It can be false, when not enough samples are present for example.
		 */
		bool getRates(const IrStd::Type::Timestamp fromTimestamp,
				const IrStd::Type::Timestamp toTimestamp,
				std::function<void(const IrStd::Type::Timestamp, const IrStd::Type::Decimal)> callback) const noexcept;

		/**
		 * Set the maximal precision of this transaction
		 */
		void setDecimalPlace(const size_t decimalPlace) noexcept;

		/**
		 * Get the maximal precision of this transaction
		 */
		size_t getDecimalPlace() const noexcept;

		/**
		 * Set the maximal precision of an order based on this transaction
		 */
		void setOrderDecimalPlace(const size_t decimalPlace) noexcept;

		/**
		 * Get the maximal precision of an order based on this transaction
		 */
		size_t getOrderDecimalPlace() const noexcept;

		/**
		 * \brief This is the timestamp when the rate of the transaction has been updated
		 */
		IrStd::Type::Timestamp getTimestamp() const noexcept;

		/**
		 * Get the final amount after processing the transaction
		 */
		IrStd::Type::Decimal getFinalAmount(const IrStd::Type::Decimal amount, const bool includeFee = true) const noexcept;
		IrStd::Type::Decimal getFinalAmount(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate, const bool includeFee = true) const noexcept;

		/**
		 * Get the intial amount expected given the final amount
		 */
		IrStd::Type::Decimal getInitialAmount(const IrStd::Type::Decimal amount, const bool includeFee = true) const noexcept;
		IrStd::Type::Decimal getInitialAmount(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate, const bool includeFee = true) const noexcept;

		CurrencyPtr getInitialCurrency() const noexcept;
		CurrencyPtr getFinalCurrency() const noexcept;

		/**
		 * \brief Check if the transaction is valid.
		 */
		virtual bool isValid(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const
		{
			return (amount > 0 && rate > 0);
		}

		virtual IrStd::Type::Decimal getFinalAmountImpl(const IrStd::Type::Decimal amount,
				const IrStd::Type::Decimal rate) const noexcept = 0;
		virtual IrStd::Type::Decimal getInitialAmountImpl(const IrStd::Type::Decimal amount,
				const IrStd::Type::Decimal rate) const noexcept = 0;

		virtual void toStream(std::ostream& out) const;
		void toStreamPair(std::ostream& out) const;

	private:
		struct Data
		{
			IrStd::Type::Decimal m_rate;
			IrStd::Type::Timestamp m_timestamp;
		};
		std::atomic<Data> m_data;
		IrStd::Type::RingBufferSorted<IrStd::Type::Timestamp, IrStd::Type::Decimal, NB_RECORDS> m_previousRates;
		CurrencyPtr m_initalCurrency;
		CurrencyPtr m_finalCurrency;
		IrStd::Type::Decimal m_decimalPlace;
		IrStd::Type::Decimal m_decimalPlaceOrder;
		bool m_isFirst;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::Transaction& transaction);
