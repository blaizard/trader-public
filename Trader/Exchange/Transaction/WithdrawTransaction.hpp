#pragma once

#include "IrStd/IrStd.hpp"
#include "Trader/Exchange/Transaction/Transaction.hpp"

namespace Trader
{
	class WithdrawTransaction : public Transaction
	{
	public:
		WithdrawTransaction(const CurrencyPtr currency,
				const IrStd::Type::Gson data = IrStd::Type::Gson());

		/**
		 * \brief Set the fixed fee amount to the transaction
		 */
		void setFeeFixed(const IrStd::Type::Decimal fee);

		/**
		 * \brief Set the precentage fee for the transaction
		 */
		void setFeePercent(const IrStd::Type::Decimal fee);

		/**
		 * \brief Optional data that can be associated with this transaction
		 */
		const IrStd::Type::Gson& getData() const noexcept;

		/**
		 * \brief Implementation of the final amount with percentage fee
		 */
		IrStd::Type::Decimal getFinalAmountImpl(const IrStd::Type::Decimal amount,
				const IrStd::Type::Decimal rate) const noexcept override;

		IrStd::Type::Decimal getInitialAmountImpl(const IrStd::Type::Decimal amount,
				const IrStd::Type::Decimal rate) const noexcept override;

	protected:
		const IrStd::Type::Gson m_data;
		IrStd::Type::Decimal m_feePercent;
		IrStd::Type::Decimal m_feeFixed;
	};
}
