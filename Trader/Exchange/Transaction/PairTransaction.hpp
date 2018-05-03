#pragma once

#include <atomic>
#include <ostream>
#include <memory>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Transaction/Transaction.hpp"
#include "Trader/Exchange/Transaction/Boundaries.hpp"

namespace Trader
{
	// ---- Interface ---------------------------------------------------------

	class PairTransactionImpl;
	class InvertPairTransactionImpl;
	class PairTransaction : public Transaction
	{
	public:
		PairTransaction(const CurrencyPtr intialCurrency, const CurrencyPtr finalCurrency);

		/**
		 * \brief Get the fee in currency2 of the transaction
		 */
		IrStd::Type::Decimal getFeeFinalCurrency(const IrStd::Type::Decimal amount) const
		{
			return getFeeFinalCurrency(amount, getRate());
		}
		IrStd::Type::Decimal getFeeFinalCurrency(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const;

		/**
		 * \brief Get the fee in the initial currency of the transaction
		 */
		IrStd::Type::Decimal getFeeInitialCurrency(const IrStd::Type::Decimal amount) const
		{
			return getFeeInitialCurrency(amount, getRate());
		}
		IrStd::Type::Decimal getFeeInitialCurrency(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const;

		/**
		 * Get the boundaries for this transaction
		 */
		virtual const Boundaries getBoundaries() const noexcept = 0;

		/**
		 * \brief Assess if the transaction is valid
		 */
		bool isValid(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const noexcept;
		void toStream(std::ostream& out) const;

		/**
		 * \brief Helper functions to set the rate
		 */
		void setBidPrice(const IrStd::Type::Decimal rate, const IrStd::Type::Timestamp timestamp);
		void setAskPrice(const IrStd::Type::Decimal rate, const IrStd::Type::Timestamp timestamp);

		/**
		 * \brief Helper function to get the right transaction from the pair
		 */
		std::shared_ptr<PairTransaction> getSellTransaction() const;
		std::shared_ptr<PairTransaction> getBuyTransaction() const;

		/**
		 * \brief Tells if this is an inverted transaction of not
		 */
		virtual bool isInvertedTransaction() const noexcept = 0;

		/**
		 * \brief Optional data that can be associated with this transaction
		 * in the form of a string
		 */
		virtual const IrStd::Type::Gson& getData() const noexcept = 0;

		/**
		 * \brief Return the formated amount and rate (as a string with
		 *        the appropriate decimal) for placeing an order.
		 * \note If the pair is inverted, the order is considered as a buy, hence
		 *       The rate is the inverse and the amount is processed accordingly.
		 */
		virtual std::pair<IrStd::Type::ShortString, IrStd::Type::ShortString> getAmountAndRateForOrder(
				IrStd::Type::Decimal amount, IrStd::Type::Decimal rate) const = 0;

		/**
		 * \brief Set the fixed fee amount to the transaction
		 */
		void setFeeFixed(const IrStd::Type::Decimal fee);
		/**
		 * \brief Set the precentage fee for the transaction
		 */
		void setFeePercent(const IrStd::Type::Decimal fee);

		/**
		 * \brief Get the fixed fee for the transaction
		 */
		IrStd::Type::Decimal getFeeFixed() const;
		/**
		 * \brief Get the precentage fee for the transaction
		 */
		IrStd::Type::Decimal getFeePercent() const;

		/**
		 * \brief Compare 2 transactions
		 * They are considered equal only if the initial and final currencies are the same
		 */
		bool operator==(const PairTransaction& transaction) const noexcept;
		bool operator!=(const PairTransaction& transaction) const noexcept;

	protected:
		friend class Exchange;
		/**
		 * Returns a pointer on the boundaries of the transaction for write,
		 * or nullptr if the transaction is inverted.
		 */
		virtual Boundaries* getBoundariesForWrite() noexcept = 0;

	private:
		const PairTransactionImpl* getPairTransaction() const;
		const InvertPairTransactionImpl* getInvertPairTransaction() const;
		PairTransactionImpl* getPairTransactionForWrite();
	};

	// ---- PairTransactionImpl -----------------------------------------------

	class PairTransactionImpl : public PairTransaction
	{
	public:
		PairTransactionImpl(const CurrencyPtr intialCurrency, const CurrencyPtr finalCurrency,
				const IrStd::Type::Gson data = IrStd::Type::Gson());

		/**
		 * \brief Set the precentage fee for the transaction
		 */
		void setBoundaries(const Boundaries& boundaries) noexcept;

		/**
		 * \brief Always returns false
		 */
		bool isInvertedTransaction() const noexcept final;

		/**
		 * \brief Get the boundary object of the transaction
		 */
		const Boundaries getBoundaries() const noexcept;

		/**
		 * \brief Optional data that can be associated with this transaction
		 * in the form of a string
		 */
		const IrStd::Type::Gson& getData() const noexcept;

		std::pair<IrStd::Type::ShortString, IrStd::Type::ShortString> getAmountAndRateForOrder(
				IrStd::Type::Decimal amount, IrStd::Type::Decimal rate) const override;

		/**
		 * \brief Implementation of the final amount with percentage fee
		 */
		IrStd::Type::Decimal getFinalAmountImpl(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const noexcept override;
		IrStd::Type::Decimal getInitialAmountImpl(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const noexcept override;

		void toStream(std::ostream& os) const override;

	protected:
		friend class Exchange;
		friend class PairTransaction;
		friend class PairTransactionMap;
		friend class InvertPairTransactionImpl;

		Boundaries* getBoundariesForWrite() noexcept override;

		Boundaries m_boundaries;
		IrStd::Type::Decimal m_feePercent;
		IrStd::Type::Decimal m_feeFixed;
		const IrStd::Type::Gson m_data;

		// Link to its inverted transaction
		std::shared_ptr<PairTransaction> m_pInvertedTransaction;
	};

	// ---- InvertPairTransactionImpl -----------------------------------------

	/**
	 * Inverse of an existing transaction (to go from finalCurrency to intialCurrency)
	 */
	class InvertPairTransactionImpl : public PairTransaction
	{
	public:
		InvertPairTransactionImpl(std::shared_ptr<PairTransaction>& pTransaction);

		/**
		 * \brief Always returns true
		 */
		bool isInvertedTransaction() const noexcept final;

		template<class T>
		const T* getTransaction() const noexcept
		{
			return static_cast<const T*>(m_pTransaction.get());
		}

		/**
		 * \brief Get the boundary object of the transaction
		 */
		const Boundaries getBoundaries() const noexcept;

		/*
		 * \brief Optional data that can be associated with this transaction
		 * in the form of a string
		 */
		const IrStd::Type::Gson& getData() const noexcept;

		std::pair<IrStd::Type::ShortString, IrStd::Type::ShortString> getAmountAndRateForOrder(
				IrStd::Type::Decimal amount, IrStd::Type::Decimal rate) const override;

		/**
		 * \brief Implementation of the final amount with percentage fee
		 */
		IrStd::Type::Decimal getFinalAmountImpl(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const noexcept override;
		IrStd::Type::Decimal getInitialAmountImpl(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const noexcept override;

		void toStream(std::ostream& os) const override;

	protected:
		friend class PairTransaction;

		Boundaries* getBoundariesForWrite() noexcept override;

		std::shared_ptr<PairTransaction> m_pTransaction;
	};

	// ---- PairTransactionNop ------------------------------------------------

	/**
	 * No-operation transaction are pair transaction on a single currency.
	 * Hence the rate is always 1. It is mainly used for code simplification.
	 */
	class PairTransactionNop : public IrStd::SingletonImpl<PairTransactionNop>
	{
	public:
		/**
		 * Returns the no-operation trasnaction
		 */
		const std::shared_ptr<PairTransaction> get(CurrencyPtr currency) const noexcept;

		PairTransactionNop();
	protected:
		std::map<CurrencyPtr, std::shared_ptr<PairTransaction>> m_nopPairTransactionMap;
	};
}
