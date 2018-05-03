#pragma once

#include "Trader/Exchange/Balance/Balance.hpp"

namespace Trader
{
	class BalanceMovements
	{
	public:
		/**
		 * \brief Defines the number of movements to be recorded
		 */
		static constexpr size_t NB_RECORDS = 256;

		BalanceMovements();

		/**
		 * \brief Update the balance the detect movements
		 */
		void update(const Balance& balance);

		/**
		 * \brief Completly clear the movements
		 */
		void clear() noexcept;

		/**
		 * Get the tiemstamp of the last change
		 */
		IrStd::Type::Timestamp getLastUpdateTimestamp() const noexcept;

		/**
		 * \brief Consume changes of a specific currency and amount.
		 *
		 * This will look for the oldest change (starting from oldTimestamp)
		 * and cosume the change amount of the currency passed into argument
		 *
		 * \return The amount left that has not been consumed. 0 if everythign has been consumed.
		 */
		IrStd::Type::Decimal consume(const IrStd::Type::Timestamp oldTimestamp, const IrStd::Type::Decimal,
				const CurrencyPtr) noexcept;

		/**
		 * \brief Retrieves the last balance movements
		 *
		 * \param newTimestamp The newset timestamp from when we need the timestamps
		 * \param oldTimestamp The oldest timestamp until when it should stop
		 *
		 * \return true if all the samples asked have been delivered, false otherwise.
		 *         It can be false, when not enough samples are present for example.
		 */
		bool get(const IrStd::Type::Timestamp newTimestamp,
				const IrStd::Type::Timestamp oldTimestamp,
				std::function<void(const IrStd::Type::Timestamp, const IrStd::Type::Decimal, const CurrencyPtr)> callback) const noexcept;

	private:
		mutable IrStd::RWLock m_lock;

		std::map<CurrencyPtr, IrStd::Type::Decimal> m_fundList;
		IrStd::Type::RingBufferSorted<IrStd::Type::Timestamp,
				std::pair<IrStd::Type::Decimal, Trader::CurrencyPtr>, NB_RECORDS> m_movements;
		IrStd::Type::Timestamp m_timestamp;
	};
}
