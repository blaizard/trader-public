#pragma once

#include <vector>

#include "Trader/Exchange/Balance/BalanceMovements.hpp"
#include "Trader/Exchange/Order/TrackOrder.hpp"
#include "Trader/Exchange/Event/EventManager.hpp"
#include "Trader/Generic/Id/Id.hpp"

namespace Trader
{
	class TrackOrderList
	{
	private:
		/**
		 * \brief Defines the number of previous order updates to be recorded
		 */
		static constexpr size_t NB_RECORDS = 256;

	public:
		TrackOrderList(EventManager& eventManager, const size_t timeoutOrderRegisteredMs);

		/**
		 * \brief Resets the content of the track order list,
		 * must be called to start from fresh.
		 *
		 * \param keepOrders Will keep the current orders backlog and will
		 * try to match them with the next update. This is usefull not to
		 * loose any data after a restart.
		 */
		void initialize(const bool keepOrders);

		/**
		 * Move an order into the track list
		 */
		void add(const TrackOrder& track, const char* const pMessage = "")
		{
			add(std::move(TrackOrder(track)), pMessage);
		}
		void add(TrackOrder&& track, const char* const pMessage = "");

		/**
		 * Remove cause
		 */
		enum class RemoveCause : size_t
		{
			NONE = 0,
			FAILED,
			CANCEL,
			TIMEOUT
		};

		/**
		 * Mark the order as failed
		 */
		bool remove(const RemoveCause cause, const Id id, const char* const pMessage, const bool mustExists = true);

		/**
		 * Activate an order previously registered
		 */
		bool activate(const Id id, const bool mustExists = true);

		/**
		 * Match an Id with a set of ids
		 */
		bool match(const Id id, const std::vector<Id>& newIdList, const bool mustExists = true);

		/**
		 * Filter 
		 */
		enum class EachFilter
		{
			ALL = 0xff,
			ACTIVE_PLACEHOLDER = 0x1,
			MATCHED_PLACEHOLDER = 0x2,
			PLACEHOLDER = 0x4,
			MATCHED = 0x8,
			CANCEL = 0x10,
			FAILED = 0x20,
			TIMEOUT = 0x40
		};

		/**
		 * Iterate through all active orders
		 */
		void each(const std::function<void(const TrackOrder&)>& callback,
				const EachFilter filter = EachFilter::ALL) const noexcept;

		/**
		 * \brief Update the current track order list
		 *
		 * Note 1 existing entry can match multiple new entries (not supported yet)
		 * but the way around is not possible.
		 *
		 * \param list The new order list
		 * \param timestampBalanceBeforeOrder Timestamp of the previosu balance update before
		 *        the current order update.
		 *
		 * \return true if the order list is different and hence updated,
		 *         false otherwise.
		 */
		bool update(std::vector<TrackOrder> list,
				const IrStd::Type::Timestamp timestampBalanceBeforeOrder = 0);

		/**
		 * \brief Track movements of the balance
		 */
		void updateBalance(const Balance& balance) noexcept;

		/**
		 * \brief Update the reserve of the balance to ensure some funds are kept for next orders
		 */
		void reserveBalance(Balance& balance) const;

		/**
		 * This function will cancel all out of dates order (timeout)
		 */
		bool cancelTimeout(const IrStd::Type::Timestamp timestamp,
				const std::function<bool(const TrackOrder&)>& cancelOrder);

		/**
		 * Return the instance of the balance movement object in read only.
		 */
		const BalanceMovements& getBalanceMovements() const noexcept;

		void toStream(std::ostream& out) const;

		void getRecords(const std::function<void(const IrStd::Type::Timestamp, const char* const, const Id,
				const char* const, const CurrencyPtr, const CurrencyPtr, const IrStd::Type::Decimal,
				const IrStd::Type::Decimal, const size_t, const char* const, const Id)>& callback) const noexcept;

	private:
		class TrackOrderEntry
		{
		public:
			TrackOrderEntry(const TrackOrder& trackOrder, const bool isPlaceHolder);

			/**
			 * Match the entry with a tracking order.
			 * The entry will then be considered as matched.
			 */
			void matchTrackOrder(const TrackOrder& trackOrder) noexcept;

			// Get the track order related to the entry
			const TrackOrder& getTrackOrder() const noexcept;
			TrackOrder& getTrackOrder() noexcept;

			/**
			 * Order type
			 */
			bool isPlaceHolder() const noexcept;
			bool isActivatedPlaceHolder() const noexcept;
			bool isMatchedPlaceHolder() const noexcept;
			bool isMatched() const noexcept;
			void setActivatedPlaceHolder() noexcept;
			IrStd::Type::Timestamp getActivatedPlaceHolderTimestamp() const noexcept;
			void activatePlaceHolder(const IrStd::Type::Timestamp timestamp) noexcept;
			void matchPlaceHolder(const IrStd::Type::Timestamp timestamp) noexcept;

			/**
			 * Cancel an order
			 */
			void setCancel(const IrStd::Type::Timestamp timestamp, const RemoveCause cause) noexcept;
			void unsetCancel() noexcept;
			RemoveCause getCancelCause() const noexcept;
			bool isCancel() const noexcept;
			bool isCancelTimeout(const IrStd::Type::Timestamp timestamp, const size_t timeoutMs) const noexcept;

			/**
			 * \brief Get the minima,l rate distance between the recorded timestamps within the timeframe.
			 *
			 * A positive distance means that the rate of the entry is higher than the ones regstered,
			 * in other word, that there is no match with the order.
			 * If equal to zero or negative, it means that the rate of the order has been lower and a
			 * match could have happened.
			 */
			IrStd::Type::Decimal getMinDistance(const IrStd::Type::Timestamp newTimestamp,
					const IrStd::Type::Timestamp oldTimestamp) const noexcept;

			/**
			 * \brief Returns the movements in the balance since the time period passed into argument
			 *
			 * It returns the accumulation of negative movement for the initial currency and the
			 * accumulation of positive movement for the final currency.
			 */
			std::pair<IrStd::Type::Decimal, IrStd::Type::Decimal> getBalanceMovements(
					const IrStd::Type::Timestamp newTimestamp, const IrStd::Type::Timestamp oldTimestamp,
					const BalanceMovements& balanceMovements) const noexcept;

			// Print
			void toStream(std::ostream& out) const;

			// Comparators
			bool operator==(const TrackOrderEntry& entry) const noexcept;
			bool operator!=(const TrackOrderEntry& entry) const noexcept;

			/**
			 * Return true if the amount of the order is neglectable
			 */
			bool isAmountNeglectable() const noexcept;
		private:
			TrackOrder m_trackOrder;

			/**
			 * An order can be marked as:
			 * - placedFailed <- expected to disapear in the next X seconds
			 * - cancelled <- expected to disapear in the next X seconds
			 * - placeholder <- it did not matched any order from the update, keep alive for Y seconds
			 *   (after it has been activated). Activating the order will also update its creation time.
			 */
			RemoveCause m_cancelCause;
			/**
			 * A canceled order will be removed only after a certain time, starting
			 * from this timestamp.
			 */
			IrStd::Type::Timestamp m_cancelTimestamp;

			/**
			 * The timestamp when the placeholder got activated
			 */
			IrStd::Type::Timestamp m_activatedTimestamp;

			/**
			 * A placeholder order is a virtual order that has been created by the Trader
			 * but not matched with any order from the server.
			 */
			enum class Type
			{
				/**
				 * The order has matched another one with from the server
				 */
				MATCHED,
				/**
				 * The is a virtual order, that might have failed, or not exists
				 * in the server. At this point the order is not considered as
				 * activated, hence any timeout does not work
				 */
				PLACEHOLDER,
				/**
				 * Same as PLACEHOLDER but the timeout starts to take effect
				 */
				ACTIVATED_PLACEHOLDER,
				/**
				 * This is an activated placeholder, with an ID from teh server
				 */
				MATCHED_PLACEHOLDER
			};
			Type m_type;
		};

		class TrackOrderAction
		{
		public:
			/**
			 * Type of action to be processed
			 */
			enum class Type
			{
				PROCESS,
				FAILED,
				TIMEOUT
			};

			static TrackOrderAction process(const TrackOrder& trackOrder, const IrStd::Type::Decimal amount)
			{
				TrackOrderAction action(Type::PROCESS, trackOrder);
				action.m_amount = amount;
				return action;
			}

			static TrackOrderAction failed(const TrackOrder& trackOrder)
			{
				return TrackOrderAction(Type::FAILED, trackOrder);
			}

			static TrackOrderAction timeout(const TrackOrder& trackOrder)
			{
				return TrackOrderAction(Type::TIMEOUT, trackOrder);
			}

			const Type m_type;
			const TrackOrder m_trackOrder;
			IrStd::Type::Decimal m_amount;

		private:
			TrackOrderAction(const Type type, const TrackOrder& trackOrder)
					: m_type(type)
					, m_trackOrder(trackOrder)
			{
			}
		};

		bool isIdentical(const std::vector<TrackOrder>& list) const noexcept;
		void setUpdatedFlag() noexcept;

		/**
		 * \brief Look for a direct match between two order lists.
		 * Orders that matches will be written directly to m_list and removed from both lists.
		 * Note only a single pair with the same Id can match.
		 */
		void matchWithSameId(std::vector<TrackOrderEntry>& originalList,
				std::vector<TrackOrder>& updatedList, std::vector<TrackOrderAction>& actionList,
				const IrStd::Type::Timestamp lastTimestampWhenPresent);

		/**
		 * \brief Look for a between the 2 order lists
		 */
		void matchPlaceHolders(std::vector<TrackOrderEntry>& originalList,
				std::vector<TrackOrder>& updatedList, std::vector<TrackOrderAction>& actionList);

		/**
		 * \brief Look for a between the 2 order lists
		 */
		void handleVanishedOrder(TrackOrderEntry& entry, std::vector<TrackOrderAction>& actionList,
				const IrStd::Type::Timestamp lastTimestampWhenPresent);
		void handleCompletedOrder(const TrackOrderEntry& entry, const IrStd::Type::Decimal initialAmount,
				std::vector<TrackOrderAction>& actionList, const IrStd::Type::Timestamp lastTimestampWhenPresent);

		/**
		 * Match an entry with a track order and returns the resulting entry.
		 * Updated accordingly the amount of the original entry.
		 */
		TrackOrderEntry matchEntry(TrackOrderEntry& trackOriginalEntry, const TrackOrder& trackNew);

		/**
		 * Return the current timestamp
		 */
		virtual IrStd::Type::Timestamp getCurrentTimestamp() const noexcept;
		/**
		 * Get the timestamp of the previous update
		 */
		IrStd::Type::Timestamp getLasUpdateTimestamp() const noexcept;

		enum class RecordType
		{
			PLACE,
			PARTIAL,
			PROCEED,
			CANCEL,
			FAILED,
			TIMEOUT
		};
		void addRecord(const RecordType type, const TrackOrder& track, const char* const pMessage = "") noexcept;

		mutable IrStd::RWLock m_lockOrders;
		EventManager& m_eventManager;

		std::vector<TrackOrderEntry> m_list;

		std::vector<TrackOrderEntry>::iterator getById(const Id id) noexcept;

		// Maximal time before an order created by Trader gets registered on the server side
		const size_t m_timeoutOrderRegisteredMs;

		bool m_isUpdated;

		// The timestamp when the sync between the balance and the order is uncertain
		IrStd::Type::Timestamp m_timestampUnsync[2];

		// To record latests orders
		struct OrderState
		{
			RecordType m_type;
			Id m_id;
			TrackOrder::Type m_orderType;
			CurrencyPtr m_initialCurrency;
			CurrencyPtr m_finalCurrency;
			IrStd::Type::Decimal m_amount;
			IrStd::Type::Decimal m_rate;
			size_t m_contextId;
			std::string m_message;
			Id m_strategyId;
		};
		IrStd::Type::RingBufferSorted<IrStd::Type::Timestamp, OrderState, NB_RECORDS> m_orderRecordList;

		/**
		 * Track movements of the balance
		 */
		BalanceMovements m_balanceMovements;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::TrackOrderList& list);
