#include "IrStd/IrStd.hpp"

#include "Trader/Generic/Event/Context.hpp"
#include "Trader/Exchange/Order/TrackOrderList.hpp"
#include "Trader/Strategy/Strategy.hpp"

IRSTD_TOPIC_USE_ALIAS(TraderTrackOrder, Trader, TrackOrder);

// ---- Trader::TrackOrderList::TrackOrderEntry -------------------------------

Trader::TrackOrderList::TrackOrderEntry::TrackOrderEntry(
	const TrackOrder& trackOrder,
	const bool isPlaceHolder)
		: m_trackOrder(trackOrder)
		, m_cancelCause(RemoveCause::NONE)
		, m_cancelTimestamp(0)
		, m_activatedTimestamp(0)
		, m_type((isPlaceHolder) ? Type::PLACEHOLDER : Type::MATCHED)
{
}

void Trader::TrackOrderList::TrackOrderEntry::matchTrackOrder(const TrackOrder& trackOrder) noexcept
{
	// If it matches a different order Id, then something changed and has been updated,
	// hence remove any cancelation if any
	if (m_trackOrder.getId() != trackOrder.getId() && isCancel())
	{
		unsetCancel();
	}
	m_type = Type::MATCHED;

	IRSTD_ASSERT(TraderTrackOrder, !trackOrder.getOrder().getNext(),
			"Cannot match a track order with a chained order: " << trackOrder);
	IRSTD_ASSERT(TraderTrackOrder, !trackOrder.getContext(),
			"Cannot match a track order with a valid context: " << trackOrder);
	IRSTD_ASSERT(TraderTrackOrder, *trackOrder.getOrder().getTransaction() == *m_trackOrder.getOrder().getTransaction(),
			"Transactions for " << trackOrder.getIdForTrace() << " and " << m_trackOrder.getIdForTrace() << " are different");

	// Match everything except the context and the order chain
	m_trackOrder.setId(trackOrder.getId());
	m_trackOrder.setAmount(trackOrder.getAmount());
	m_trackOrder.setCreationTime(trackOrder.getCreationTime());
	m_trackOrder.getOrderForWrite().setRate(trackOrder.getOrder().getRate());

	IRSTD_LOG_DEBUG(TraderTrackOrder, "Matching " << trackOrder << " -> " << m_trackOrder);
}

bool Trader::TrackOrderList::TrackOrderEntry::operator==(const TrackOrderEntry& entry) const noexcept
{
	return m_trackOrder == entry.m_trackOrder
			&& (m_type == entry.m_type)
			&& (m_cancelCause == entry.m_cancelCause)
			&& (m_cancelTimestamp == entry.m_cancelTimestamp)
			&& (m_activatedTimestamp == entry.m_activatedTimestamp);
}

bool Trader::TrackOrderList::TrackOrderEntry::operator!=(const TrackOrderEntry& entry) const noexcept
{
	return !(*this == entry);
}

const Trader::TrackOrder& Trader::TrackOrderList::TrackOrderEntry::getTrackOrder() const noexcept
{
	return m_trackOrder;
}

Trader::TrackOrder& Trader::TrackOrderList::TrackOrderEntry::getTrackOrder() noexcept
{
	return m_trackOrder;
}

bool Trader::TrackOrderList::TrackOrderEntry::isAmountNeglectable() const noexcept
{
	return !m_trackOrder.getOrder().isFirstValid(m_trackOrder.getAmount() * 5.);
}

IrStd::Type::Decimal Trader::TrackOrderList::TrackOrderEntry::getMinDistance(
		const IrStd::Type::Timestamp newTimestamp,
		const IrStd::Type::Timestamp oldTimestamp) const noexcept
{
	IrStd::Type::Decimal distance = IrStd::Type::Decimal::max();
	const auto orderRate = m_trackOrder.getRate();

	// If orderRate is higher than recorded, it means there is no match.
	// Hence the distance is the orderRate minus the recorded rate.
	m_trackOrder.getOrder().getTransaction()->getRates(newTimestamp, oldTimestamp,
			[&](const IrStd::Type::Timestamp, const IrStd::Type::Decimal rate) {
		distance = std::min(distance, IrStd::Type::Decimal(orderRate - rate));
	});

	// Floor the distance to the transaction decimal
	distance = IrStd::Type::doubleFormat(distance,
			m_trackOrder.getOrder().getTransaction()->getOrderDecimalPlace(), IrStd::TypeFormat::FLAG_FLOOR);

	return distance;
}

std::pair<IrStd::Type::Decimal, IrStd::Type::Decimal> Trader::TrackOrderList::TrackOrderEntry::getBalanceMovements(
		const IrStd::Type::Timestamp newTimestamp,
		const IrStd::Type::Timestamp oldTimestamp,
		const BalanceMovements& balanceMovements) const noexcept
{
	const auto initialCurrency = m_trackOrder.getOrder().getInitialCurrency();
	const auto finalCurrency = m_trackOrder.getOrder().getFirstOrderFinalCurrency();
	IrStd::Type::Decimal initialAmountDiff = 0;
	IrStd::Type::Decimal finalAmountDiff = 0;

	balanceMovements.get(newTimestamp, oldTimestamp,
		[&](const IrStd::Type::Timestamp /*timestamp*/, const IrStd::Type::Decimal amount, const CurrencyPtr currency) {
			if (currency == initialCurrency && amount < 0)
			{
				initialAmountDiff += amount;
			}
			else if (currency == finalCurrency && amount > 0)
			{
				finalAmountDiff += amount;
			}
	});

	return std::make_pair(initialAmountDiff, finalAmountDiff);
}

// ---- Trader::TrackOrderList::TrackOrderEntry (cancel) ----------------------

void Trader::TrackOrderList::TrackOrderEntry::setCancel(
		const IrStd::Type::Timestamp timestamp,
		const RemoveCause cause) noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, cause != RemoveCause::NONE, "Remove cause must not be NONE");
	m_cancelCause = cause;
	m_cancelTimestamp = timestamp;
}

void Trader::TrackOrderList::TrackOrderEntry::unsetCancel() noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, isCancel(), "Cannot uncancel and non-canceled entry");
	m_cancelCause = RemoveCause::NONE;
}

bool Trader::TrackOrderList::TrackOrderEntry::isCancel() const noexcept
{
	return (m_cancelCause != RemoveCause::NONE);
}

Trader::TrackOrderList::RemoveCause Trader::TrackOrderList::TrackOrderEntry::getCancelCause() const noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, isCancel(), "This entry is not canceled");
	return m_cancelCause;
}

bool Trader::TrackOrderList::TrackOrderEntry::isCancelTimeout(
		const IrStd::Type::Timestamp timestamp,
		const size_t timeoutMs) const noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, isCancel(), "This entry is not canceled");
	return timestamp > (m_cancelTimestamp + timeoutMs);
}

// ---- Trader::TrackOrderList::TrackOrderEntry (type) ------------------------

bool Trader::TrackOrderList::TrackOrderEntry::isPlaceHolder() const noexcept
{
	return (m_type == Type::PLACEHOLDER || isActivatedPlaceHolder() || isMatchedPlaceHolder());
}

bool Trader::TrackOrderList::TrackOrderEntry::isActivatedPlaceHolder() const noexcept
{
	return (m_type == Type::ACTIVATED_PLACEHOLDER || isMatchedPlaceHolder());
}

bool Trader::TrackOrderList::TrackOrderEntry::isMatchedPlaceHolder() const noexcept
{
	return (m_type == Type::MATCHED_PLACEHOLDER);
}

bool Trader::TrackOrderList::TrackOrderEntry::isMatched() const noexcept
{
	return (m_type == Type::MATCHED);
}

void Trader::TrackOrderList::TrackOrderEntry::setActivatedPlaceHolder() noexcept
{
	m_type = Type::ACTIVATED_PLACEHOLDER;
}

IrStd::Type::Timestamp Trader::TrackOrderList::TrackOrderEntry::getActivatedPlaceHolderTimestamp() const noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, isActivatedPlaceHolder(), "Only active placeholder can get activated timestamp");
	return m_activatedTimestamp;
}

void Trader::TrackOrderList::TrackOrderEntry::activatePlaceHolder(const IrStd::Type::Timestamp timestamp) noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, m_type == Type::PLACEHOLDER, "Only non-active placeholder can be activated");
	m_type = Type::ACTIVATED_PLACEHOLDER;
	m_activatedTimestamp = timestamp;
}

void Trader::TrackOrderList::TrackOrderEntry::matchPlaceHolder(const IrStd::Type::Timestamp timestamp) noexcept
{
	activatePlaceHolder(timestamp);
	m_type = Type::MATCHED_PLACEHOLDER;
}

// ---- Trader::TrackOrderList::TrackOrderEntry (toStream) --------------------

void Trader::TrackOrderList::TrackOrderEntry::toStream(std::ostream& out) const
{
	switch (m_type)
	{
	case Type::PLACEHOLDER:
		out << "[PlaceHolder Pending] ";
		break;
	case Type::ACTIVATED_PLACEHOLDER:
		out << "[PlaceHolder] ";
		break;
	case Type::MATCHED_PLACEHOLDER:
		out << "[PlaceHolder Matched] ";
		break;
	case Type::MATCHED:
		out << "[Matched] ";
		break;
	default:
		IRSTD_UNREACHABLE();
	}

	switch (m_cancelCause)
	{
	case RemoveCause::NONE:
		break;
	case RemoveCause::FAILED:
		out << "[Failed] ";
		break;
	case RemoveCause::CANCEL:
		out << "[Canceled] ";
		break;
	case RemoveCause::TIMEOUT:
		out << "[Timeout] ";
		break;
	default:
		IRSTD_UNREACHABLE();
	}

	out << getTrackOrder();
}

// ---- Trader::TrackOrderList ------------------------------------------------

Trader::TrackOrderList::TrackOrderList(
	EventManager& eventManager,
	const size_t timeoutOrderRegisteredMs)
		: m_eventManager(eventManager)
		, m_timeoutOrderRegisteredMs(timeoutOrderRegisteredMs)
		, m_isUpdated(false)
		, m_timestampUnsync{0, 0}
{
	initialize(/*keepOrders*/false);
}

void Trader::TrackOrderList::initialize(const bool keepOrders)
{
	auto scope = m_lockOrders.writeScope();

	if (keepOrders)
	{
		// Set all orders as placeholders
		for (auto& elt : m_list)
		{
			elt.setActivatedPlaceHolder();
		}
	}
	else
	{
		m_list.clear();
	}
}

const Trader::BalanceMovements& Trader::TrackOrderList::getBalanceMovements() const noexcept
{
	return m_balanceMovements;
}

std::vector<Trader::TrackOrderList::TrackOrderEntry>::iterator Trader::TrackOrderList::getById(const Id id) noexcept
{
	IRSTD_ASSERT(TraderTrackOrder, m_lockOrders.isScope(), "This operation is only valid under a scope");
	std::vector<TrackOrderEntry>::iterator it = m_list.begin();
	for (; it != m_list.end(); it++)
	{
		if (it->getTrackOrder().getId() == id)
		{
			return it;
		}
	}
	return m_list.end();
}

void Trader::TrackOrderList::add(
		TrackOrder&& track,
		const char* const pMessage)
{
	addRecord(RecordType::PLACE, track, pMessage);
	{
		auto scope = m_lockOrders.writeScope();

		TrackOrderEntry entry(std::move(track), /*isPlaceHolder*/true);
		m_list.push_back(std::move(entry));

		// Set the updated flag
		setUpdatedFlag();
	}
}

bool Trader::TrackOrderList::remove(
		const RemoveCause cause,
		const Id trackOrderId,
		const char* const pMessage,
		const bool mustExists)
{
	// The order will be removed only after a certain timeout to ensure
	// that it was not a false notification
	{
		auto scope = m_lockOrders.writeScope();
		auto it = getById(trackOrderId);
		// Returns silently if the id does not exists
		if (mustExists == false && it == m_list.end())
		{
			return false;
		}
		IRSTD_THROW_ASSERT(TraderTrackOrder, it != m_list.end(),
				"The order (id=#" << trackOrderId << ") is not registered");

		std::stringstream messageStream;
		switch (cause)
		{
		case RemoveCause::FAILED:
			messageStream << "(Failed) " << pMessage;
			addRecord(RecordType::FAILED, it->getTrackOrder(), messageStream.str().c_str());
			it->setCancel(getCurrentTimestamp(), cause);
			break;

		case RemoveCause::CANCEL:
			messageStream << "(Cancel) " << pMessage;
			addRecord(RecordType::CANCEL, it->getTrackOrder(), messageStream.str().c_str());
			it->setCancel(getCurrentTimestamp(), cause);
			break;

		case RemoveCause::TIMEOUT:
			messageStream << "(Timeout) " << pMessage;
			addRecord(RecordType::TIMEOUT, it->getTrackOrder(), messageStream.str().c_str());
			it->setCancel(getCurrentTimestamp(), cause);
			break;

		case RemoveCause::NONE:
			IRSTD_UNREACHABLE(TraderTrackOrder, "None cannot be a cause for removal");
			break;

		default:
			IRSTD_UNREACHABLE();
		}

		// Set the updated flag
		setUpdatedFlag();
	}

	return true;
}

bool Trader::TrackOrderList::match(
		const Id id,
		const std::vector<Id>& newIdList,
		const bool mustExists)
{
	IRSTD_ASSERT(TraderTrackOrder, id.isValid(),
			"The order id are invalid, id=" << id);
	{
		auto scope = m_lockOrders.writeScope();
		const auto it = getById(id);
		// Returns silently if the id does not exists
		if (mustExists == false && it == m_list.end())
		{
			return false;
		}
		IRSTD_THROW_ASSERT(TraderTrackOrder, it != m_list.end(),
				"The order id#" << id << " is not registered");
		IRSTD_THROW_ASSERT(TraderTrackOrder, it->isPlaceHolder(),
				"The order must be a placeholder");

		// Copy and delete the current entry
		const TrackOrderEntry entry(*it);
		m_list.erase(it);

		// Assign the new id to all orders
		for (const auto& newId : newIdList)
		{
			IRSTD_ASSERT(TraderTrackOrder, id != newId, "The order Ids ("
					<< id << ") are equals");
			IRSTD_ASSERT(TraderTrackOrder, newId.isValid(),
					"The order id are invalid, newId=" << newId);

			TrackOrderEntry newEntry(entry);
			newEntry.getTrackOrder().setId(newId);
			newEntry.matchPlaceHolder(getCurrentTimestamp());
			m_eventManager.copyOrder(id, newId, EventManager::Lifetime::ORDER);

			// Add the entry to the list
			m_list.push_back(std::move(newEntry));
			setUpdatedFlag();

			IRSTD_LOG_TRACE(TraderTrackOrder, "Order id#" << id
					<< " matches with order id#" << newId);
		}
	}

	return true;
}

bool Trader::TrackOrderList::activate(
		const Id trackOrderId,
		const bool mustExists)
{
	// Look for the order
	{
		auto scope = m_lockOrders.writeScope();
		auto it = getById(trackOrderId);
		// Returns silently if the id does not exists
		if (mustExists == false && it == m_list.end())
		{
			return false;
		}
		IRSTD_THROW_ASSERT(TraderTrackOrder, it != m_list.end(),
				"The order (id=#" << trackOrderId << ") is not registered");
		it->activatePlaceHolder(getCurrentTimestamp());

		// Set the updated flag
		setUpdatedFlag();
	}

	return true;
}

void Trader::TrackOrderList::each(
		const std::function<void(const TrackOrder&)>& callback,
		const EachFilter filter) const noexcept
{
	auto scope = m_lockOrders.readScope();
	{
		const bool filterAll = IrStd::Type::toIntegral(filter) == IrStd::Type::toIntegral(EachFilter::ALL);
		const bool filterActive = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::ACTIVE_PLACEHOLDER);
		const bool filterMatchedPlaceHolder = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::MATCHED_PLACEHOLDER);
		const bool filterPlaceHolder = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::PLACEHOLDER);
		const bool filterMatched = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::MATCHED);
		const bool filterCancel = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::CANCEL);
		const bool filterFailed = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::FAILED);
		const bool filterTimeout = IrStd::Type::toIntegral(filter) & IrStd::Type::toIntegral(EachFilter::TIMEOUT);
		
		for (const auto& elt : m_list)
		{
			if (filterAll
					|| (filterMatched && elt.isMatched())
					|| (filterActive && elt.isActivatedPlaceHolder())
					|| (filterMatchedPlaceHolder && elt.isMatchedPlaceHolder())
					|| (filterPlaceHolder && elt.isPlaceHolder())
					|| (filterCancel && elt.isCancel() && elt.getCancelCause() == RemoveCause::CANCEL)
					|| (filterFailed && elt.isCancel() && elt.getCancelCause() == RemoveCause::FAILED)
					|| (filterTimeout && elt.isCancel() && elt.getCancelCause() == RemoveCause::TIMEOUT))
			{
				callback(elt.getTrackOrder());
			}
		}
	}
}

bool Trader::TrackOrderList::isIdentical(const std::vector<TrackOrder>& list) const noexcept
{
	auto scope = m_lockOrders.readScope();

	if (m_list.size() != list.size())
	{
		return false;
	}

	for (size_t i = 0; i<m_list.size(); i++)
	{
		if (m_list[i].getTrackOrder() != list[i])
		{
			return false;
		}
	}

	return true;
}

void Trader::TrackOrderList::setUpdatedFlag() noexcept
{
	m_isUpdated = true;
}

Trader::TrackOrderList::TrackOrderEntry Trader::TrackOrderList::matchEntry(
		TrackOrderEntry& trackOriginalEntry,
		const TrackOrder& trackNew)
{
	// Make a copy of the current entry
	TrackOrderEntry entry{trackOriginalEntry};

	// Notify the Event Manager if their id is different
	if (entry.getTrackOrder().getId() != trackNew.getId())
	{
		IRSTD_LOG_INFO(TraderTrackOrder, entry.getTrackOrder().getIdForTrace()
				<< " and " << trackNew.getIdForTrace() << " are refering to the same order");
		m_eventManager.copyOrder(entry.getTrackOrder().getId(), trackNew.getId(), EventManager::Lifetime::ORDER);
	}

	// Match the track order
	entry.matchTrackOrder(trackNew);

	// Reduce the amount of the current placeholder with the matching entry
	trackOriginalEntry.getTrackOrder().setAmount(trackOriginalEntry.getTrackOrder().getAmount() - trackNew.getAmount());
	if (trackOriginalEntry.getTrackOrder().getAmount() < 0)
	{
		// Ignore this message for amoutns that are too small
		if (trackOriginalEntry.getTrackOrder().getOrder().isValid(-trackOriginalEntry.getTrackOrder().getAmount()))
		{
			IRSTD_LOG_FATAL(TraderTrackOrder, trackOriginalEntry.getTrackOrder().getIdForTrace()
					<< " matched with more orders or higher amount than it initially had, the amount of "
					<< (-trackOriginalEntry.getTrackOrder().getAmount()) << " "
					<< trackOriginalEntry.getTrackOrder().getOrder().getInitialCurrency()
					<< " is overmatched and will be ignored");
		}
		trackOriginalEntry.getTrackOrder().setAmount(0);
	}

	return entry;
}

void Trader::TrackOrderList::matchWithSameId(
		std::vector<TrackOrderEntry>& originalList,
		std::vector<TrackOrder>& updatedList,
		std::vector<TrackOrderAction>& actionList,
		const IrStd::Type::Timestamp lastTimestampWhenPresent)
{
	for (auto itOriginal = originalList.begin(); itOriginal != originalList.end();)
	{
		auto& entry = *itOriginal;
		const auto entryId = entry.getTrackOrder().getId();

		// Look for a match
		bool isMatch = false;
		for (auto itUpdated = updatedList.begin(); itUpdated != updatedList.end();)
		{
			if (entryId == itUpdated->getId())
			{
				IRSTD_ASSERT(TraderTrackOrder, isMatch == false,
						"There is more than one order with the id#" << entryId);

				// Check if the entry is supposed to be canceled
				if (entry.isCancel() && entry.isCancelTimeout(getCurrentTimestamp(), m_timeoutOrderRegisteredMs))
				{
					IRSTD_LOG_ERROR(TraderTrackOrder, "Order#" << entryId
							<< " is marked as cancel but is still present, unset cancel flag");
					entry.unsetCancel();
				}

				const auto initialAmount = entry.getTrackOrder().getAmount();

				// Process the entry
				auto matchingEntry = matchEntry(entry, *itUpdated);
				m_list.push_back(std::move(matchingEntry));

				// Process the left over of the entry
				handleCompletedOrder(entry, initialAmount, actionList, lastTimestampWhenPresent);

				// Delete the entry from the matching order
				itUpdated = updatedList.erase(itUpdated);
				isMatch = true;
			}
			else
			{
				++itUpdated;
			}
		}

		if (isMatch)
		{
			itOriginal = originalList.erase(itOriginal);
		}
		else
		{
			++itOriginal;
		}
	}
}

void Trader::TrackOrderList::matchPlaceHolders(
		std::vector<TrackOrderEntry>& originalList,
		std::vector<TrackOrder>& updatedList,
		std::vector<TrackOrderAction>& /*actionList*/)
{
	// Gives the weight between 0 and 1 of the matching between 2 values.
	// 1 being the exact match and 0 a no match at all
	const auto calculateWeight = [](const IrStd::Type::Decimal original, const IrStd::Type::Decimal updated,
			const IrStd::Type::Decimal distanceMax) -> IrStd::Type::Decimal {
		const IrStd::Type::Decimal distance = std::abs(original - updated);
		if (distance >= distanceMax)
		{
			return 0.;
		}
		return 1. - distance / distanceMax;
	};

	// If there is no entries in one or the other list, return
	if (!updatedList.size() || !originalList.size())
	{
		return;
	}

	// Note 1 existing entry can match multiple new entries
	// but the way around is not possible.
	// Build a matrix that weights matches of originalList entries vs updatedList entries
	// Example: matrixMatch:
	// WEIGHT(originalList[0], updatedList[0]), WEIGHT(originalList[0], updatedList[1]), WEIGHT(originalList[0], updatedList[2]),
	// WEIGHT(originalList[1], updatedList[0]), WEIGHT(originalList[1], updatedList[1]), WEIGHT(originalList[1], updatedList[2])
	std::vector<IrStd::Type::Decimal> matrixMatch;
	for (const auto& entry : originalList)
	{
		const auto& originalTrackOrder = entry.getTrackOrder();
		IRSTD_ASSERT(TraderTrackOrder, entry.isPlaceHolder(),
				"This entry is not a placeholder: " << originalTrackOrder.getIdForTrace());

		// Look for one or multiple matches
		for (const auto& updatedTrackOrder : updatedList)
		{
			IrStd::Type::Decimal weight = 0;
			// These are the must conditions
			if (*originalTrackOrder.getOrder().getTransaction() == *updatedTrackOrder.getOrder().getTransaction())
			{
				weight += calculateWeight(originalTrackOrder.getOrder().getRate(),
						updatedTrackOrder.getOrder().getRate(), originalTrackOrder.getOrder().getRate() * 0.1);
				if (weight > 0)
				{
					weight += calculateWeight(originalTrackOrder.getCreationTime(),
							updatedTrackOrder.getCreationTime(), 5 * 60 * 1000); // 5 minutes
					// Add the amount as well (only if the new amount is lower or +10% of the current one)
					if (updatedTrackOrder.getAmount() <= originalTrackOrder.getAmount() * 1.1)
					{
						weight += calculateWeight(originalTrackOrder.getAmount(),
								updatedTrackOrder.getAmount(), originalTrackOrder.getAmount()); // 5 minutes
					}
				}
			}
			matrixMatch.push_back(weight);
		}
	}

	// Find the highest matches
	{
		while (true)
		{
			const auto it = std::max_element(matrixMatch.begin(), matrixMatch.end());
			const auto weight = *it;
			if (weight < 0.1)
			{
				break;
			}

			const size_t i = std::distance(matrixMatch.begin(), it);
			const size_t originalListIndex = i / updatedList.size();
			const size_t updatedListIndex = i % updatedList.size();
			auto& entry = originalList[originalListIndex];
			const auto& trackOrder = updatedList[updatedListIndex];

			// If the order is marked as canceled, display a warning
			if (entry.isCancel())
			{
				IRSTD_LOG_WARNING(TraderTrackOrder, entry.getTrackOrder().getIdForTrace()
						<< " expected to be canceled but matches with " << trackOrder.getIdForTrace()
						<< " with weight " << weight);
			}
			else
			{
				IRSTD_LOG_INFO(TraderTrackOrder, entry.getTrackOrder().getIdForTrace()
						<< " matches with " << trackOrder.getIdForTrace() << " with weight " << weight);
			}

			// Add the matched order to the list
			{
				auto matchingEntry = matchEntry(entry, trackOrder);
				m_list.push_back(std::move(matchingEntry));
			}

			// Reset all the weight for all original entries in the matrix to ensure it does not match
			// any other entry
			for (size_t originalListCurIndex=0; originalListCurIndex<originalList.size(); originalListCurIndex++)
			{
				const auto matrixMatchStartIndex = originalListCurIndex * updatedList.size();
				const auto curWeight = matrixMatch[matrixMatchStartIndex + updatedListIndex];
				if (originalListCurIndex != originalListIndex && curWeight > (weight * 0.5))
				{
					IRSTD_LOG_WARNING(TraderTrackOrder, originalList[originalListCurIndex].getTrackOrder().getIdForTrace()
							<< " could also have matched " << trackOrder.getIdForTrace()
							<< " but matching weight is slightly lower (" << curWeight
							<< " vs. " << weight << ")");
				}
				IRSTD_ASSERT(matrixMatchStartIndex + updatedListIndex < matrixMatch.size());
				matrixMatch[matrixMatchStartIndex + updatedListIndex] = -1.;
			}
		}
	}

	// Delete orders on the original list that fully matched
	for (auto it = originalList.begin(); it != originalList.end();)
	{
		if (it->isAmountNeglectable())
		{
			it = originalList.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Delete the orders on the updated list that matched
	size_t i = 0;
	for (auto it = updatedList.begin(); it != updatedList.end(); i++)
	{
		if (matrixMatch[i] < 0.)
		{
			it = updatedList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Trader::TrackOrderList::handleVanishedOrder(
		TrackOrderEntry& entry,
		std::vector<TrackOrderAction>& actionList,
		const IrStd::Type::Timestamp lastTimestampWhenPresent)
{
	const auto timestamp = getCurrentTimestamp();
	const auto distance = entry.getMinDistance(timestamp, lastTimestampWhenPresent);
	const auto movements = entry.getBalanceMovements(timestamp, lastTimestampWhenPresent, m_balanceMovements);

	// Calculate the probability that the order is completed (or not)
	IrStd::Type::Decimal probabilityProcessed = 0;
	{
		// Gives the weight between 0 and 1 of the matching of value to its target.
		// If value exceed the target (higher), then returns 1, if below minVal, returns 0.
		const auto calculateWeight = [](const IrStd::Type::Decimal value, const IrStd::Type::Decimal target,
				const IrStd::Type::Decimal minVal) -> IrStd::Type::Decimal {
			return std::min(IrStd::Type::Decimal(1),
					std::max(IrStd::Type::Decimal(0),
					IrStd::Type::Decimal((value - minVal) / (target - minVal))));
		};

		// Compute the probability
		{
			const auto initialAmount = entry.getTrackOrder().getAmount();
			switch (entry.getTrackOrder().getType())
			{
			case TrackOrder::Type::MARKET:
			case TrackOrder::Type::LIMIT:
				probabilityProcessed += calculateWeight(-distance, 0,
						-(entry.getTrackOrder().getOrder().getRate() * 0.02)); // 2% of the rate
				probabilityProcessed += calculateWeight(-movements.first, initialAmount, 0);
				probabilityProcessed += calculateWeight(movements.second,
						entry.getTrackOrder().getOrder().getFirstOrderFinalAmount(initialAmount), 0);
				probabilityProcessed /= 3;
				break;
			case TrackOrder::Type::WITHDRAW:
				probabilityProcessed += calculateWeight(-movements.first, initialAmount, 0);
				break;
			default:
				IRSTD_UNREACHABLE(TraderTrackOrder);
			}
		}
	}

	// Add message for debug purpose
	IRSTD_LOG_INFO(TraderTrackOrder, entry.getTrackOrder().getIdForTrace()
			<< " vanished, probability to be processed: " << (probabilityProcessed * 100)
			<< "% and marked as " << ((entry.isCancel()) ? "canceled" : "proceed")
			<< " (last.timestamp=" << lastTimestampWhenPresent << ", distance=" << distance
			<< ", balance.movements=[" << movements.first << ", " << movements.second << "])");

	// If the order is expected to be canceled but it seems that it went through
	if (entry.isCancel() && probabilityProcessed > 0.8)
	{
		IRSTD_LOG_WARNING(TraderTrackOrder, entry.getTrackOrder().getIdForTrace()
				<< " is expected to be canceled but it shows high probability to be processed ("
				<< (probabilityProcessed * 100) << "%, distance=" << distance << ", movements=["
				<< movements.first << ", " << movements.second << "]), set as proceed");
		entry.unsetCancel();
	}
	else if (!entry.isCancel() && probabilityProcessed < 0.2)
	{
		std::stringstream messageStream;
		messageStream << entry.getTrackOrder().getIdForTrace()
				<< " is expected to be processed but it shows high probability to be canceled ("
				<< ((1 - probabilityProcessed) * 100) << "%, distance=" << distance << ", movements=["
				<< movements.first << ", " << movements.second << "]), set as ";

		if (entry.isPlaceHolder())
		{
			messageStream << "failed";
			addRecord(RecordType::FAILED, entry.getTrackOrder(), messageStream.str().c_str());
			entry.setCancel(timestamp, RemoveCause::FAILED);
		}
		else
		{
			messageStream << "cancel";
			addRecord(RecordType::CANCEL, entry.getTrackOrder(), messageStream.str().c_str());
			entry.setCancel(timestamp, RemoveCause::CANCEL);
		}

		IRSTD_LOG_WARNING(TraderTrackOrder, messageStream.str());
	}

	// If the order is marked as cancel, do nothing, it is expected to be removed
	if (entry.isCancel())
	{
		switch (entry.getCancelCause())
		{
		case RemoveCause::FAILED:
			{
				const auto action = TrackOrderAction::failed(entry.getTrackOrder());
				actionList.push_back(action);
			}
			break;
		case RemoveCause::TIMEOUT:
			{
				const auto action = TrackOrderAction::timeout(entry.getTrackOrder());
				actionList.push_back(action);
			}
			break;
		case RemoveCause::CANCEL:
			break;

		case RemoveCause::NONE:
		default:
			IRSTD_UNREACHABLE(TraderTrackOrder);
		}
	}
	else
	{
		// Todo: set the order as proceed ini the balance movements
		// so that 2 orders cannot use the same movement to make a decision
		handleCompletedOrder(entry, entry.getTrackOrder().getAmount(), actionList, lastTimestampWhenPresent);
	}
}

/**
 * When a entry vanished call this function to asses if the order has been processed,
 * or canceled or can be ignored.
 */
void Trader::TrackOrderList::handleCompletedOrder(
		const TrackOrderEntry& entry,
		const IrStd::Type::Decimal initialAmount,
		std::vector<TrackOrderAction>& actionList,
		const IrStd::Type::Timestamp lastTimestampWhenPresent)
{
	const auto& trackOrder = entry.getTrackOrder();
	const auto amount = trackOrder.getAmount();

	if (amount < 0)
	{
		IRSTD_LOG_ERROR(TraderTrackOrder, "Amount (" << amount << ") for " << trackOrder.getIdForTrace()
				<< " is negative, which means there is an inconsitency in the order list");
		return;
	}
	// If the amount is too small, simply ignore
	if (entry.isAmountNeglectable())
	{
		// Do nothing
		return;
	}

	const auto timestamp = getCurrentTimestamp();
	const auto finalAmount = entry.getTrackOrder().getOrder().getFirstOrderFinalAmount(amount);

	// Check if the balance movements are only generated by this order (variation is within 0.5%)
	// and if so, calculate eh effective fee
	{
		const auto movements = entry.getBalanceMovements(timestamp, lastTimestampWhenPresent, m_balanceMovements);
		if ((std::abs(amount - (-movements.first)) < amount * 0.05)
					&& (std::abs(finalAmount - (movements.second)) < finalAmount * 0.05))
		{
			// Compute the estimated fee
			const auto actualFinalAmountNoFee = -movements.first * entry.getTrackOrder().getOrder().getRate();
			const auto actualFinalAmount = movements.second;
			const auto feePercent = IrStd::Type::doubleFormat((actualFinalAmountNoFee - actualFinalAmount) / actualFinalAmountNoFee * 100, 2);

			// Boundaries for unrealistic fees
			if (feePercent >= 0 && feePercent < 0.5)
			{
				IRSTD_LOG_INFO(TraderTrackOrder, "Estimated fee for " << entry.getTrackOrder().getIdForTrace()
						<< " is " << feePercent << "%");
			}
		}
	}

	// Consume the balance variations
	{
		IrStd::Type::Decimal notConsumedInitial = 0;
		IrStd::Type::Decimal notConsumedFinal = 0;

		switch (entry.getTrackOrder().getType())
		{
		case TrackOrder::Type::MARKET:
		case TrackOrder::Type::LIMIT:
			notConsumedInitial = m_balanceMovements.consume(lastTimestampWhenPresent,
					-amount, entry.getTrackOrder().getOrder().getInitialCurrency());
			notConsumedFinal = m_balanceMovements.consume(lastTimestampWhenPresent,
					finalAmount, entry.getTrackOrder().getOrder().getFirstOrderFinalCurrency());
			break;
		case TrackOrder::Type::WITHDRAW:
			notConsumedInitial = m_balanceMovements.consume(lastTimestampWhenPresent,
					-amount, entry.getTrackOrder().getOrder().getInitialCurrency());
			break;
		default:
			IRSTD_UNREACHABLE(TraderTrackOrder);
		}

		if (notConsumedInitial < -amount * 0.01 || notConsumedFinal > finalAmount * 0.01)
		{
			std::stringstream strStream;
			if (notConsumedInitial < -amount * 0.01)
			{
				strStream << -notConsumedInitial << " " << entry.getTrackOrder().getOrder().getInitialCurrency();
			}
			if (notConsumedFinal > finalAmount * 0.01)
			{
				strStream << ((strStream.str().empty()) ? "" : " and ") << notConsumedFinal
						<< " " << entry.getTrackOrder().getOrder().getFirstOrderFinalCurrency();
			}
			IRSTD_LOG_WARNING(TraderTrackOrder, entry.getTrackOrder().getIdForTrace()
					<< " is processed but the full amount movement is not detected, missing " << strStream.str());
		}
	}

	{
		const auto action = TrackOrderAction::process(trackOrder, initialAmount);
		actionList.push_back(action);
	}
}

/**
 * Order are matched with the current order list
 *
 * - An order from the original list can match more than one order from the server list
 * - BUT An order from the server list can match only one from the original list
 * - Only placeholder orders can be matched from the order list, orthers are already matched
 */
bool Trader::TrackOrderList::update(
		std::vector<TrackOrder> list,
		const IrStd::Type::Timestamp timestampBalanceBeforeOrder)
{
	// If the order list is the same, do nothing
	if (isIdentical(list))
	{
		m_timestampUnsync[1] = m_timestampUnsync[0];
		m_timestampUnsync[0] = timestampBalanceBeforeOrder;
		return false;
	}

	// If the updated flag is set
	bool isListUpdated = m_isUpdated;
	m_isUpdated = false;
	std::vector<TrackOrderAction> actionList;

	// Here the track list orders are different
	{
		auto scope = m_lockOrders.writeScope();

		// Copy and clear the original track order list
		auto originalList = m_list;
		auto updatedList = list;
		const auto trackOrderOriginalList = m_list;
		m_list.clear();

		matchWithSameId(originalList, updatedList, actionList, m_timestampUnsync[1]);

		// Handle remaining non-placeholder orders
		for (auto it = originalList.begin(); it != originalList.end();)
		{
			if (it->isPlaceHolder())
			{
				++it;
			}
			else
			{
				// Last known to be present timestamp is the previous timestamp minus few seconds
				// to make sure changes in the balance are captured
				handleVanishedOrder(*it, actionList, m_timestampUnsync[1]);
				it = originalList.erase(it);
			}
		}

		matchPlaceHolders(originalList, updatedList, actionList);

		// Sort the list with matched placeholders first.
		// This to ensure that they are processed first, as they are more likely
		// to be completed than others
		{
			std::sort(originalList.begin(), originalList.end(), [](const TrackOrderEntry& entry1, const TrackOrderEntry& entry2) { 
				if (entry1.isMatchedPlaceHolder() && !entry2.isMatchedPlaceHolder())
				{
					return true;
				}
				if (entry2.isMatchedPlaceHolder() && !entry1.isMatchedPlaceHolder())
				{
					return false;
				}
				// The comparison is strict, so it is important to return false in order
				// to prevent a sig 11 to happen in this case.
				return false;
			});
		}

		// These are the orders from the original list that did not matched any order
		for (auto& entry : originalList)
		{
			IRSTD_ASSERT(TraderTrackOrder, entry.isPlaceHolder(),
					"This entry is not a placeholder: " << entry.getTrackOrder().getIdForTrace());

			// If the order is not activated or if its discovery time did not expire
			// or if it has the cancel state and it did not expire add it back to the list
			if (!entry.isActivatedPlaceHolder()
					// If activated but register time is not reached yet
					|| (getCurrentTimestamp() - entry.getActivatedPlaceHolderTimestamp()) < m_timeoutOrderRegisteredMs
					// If cancel and cancel time not reached yet
					|| (entry.isCancel() && !entry.isCancelTimeout(getCurrentTimestamp(), m_timeoutOrderRegisteredMs)))
			{
				m_list.push_back(entry);
			}
			else
			{
				IRSTD_ASSERT(TraderTrackOrder, entry.isActivatedPlaceHolder(),
						"This entry is not an activated placeholder: " << entry.getTrackOrder().getIdForTrace());

				// Look for the last balance update before the order creation time
				const auto lastTimestampWhenPresent = entry.getTrackOrder().getCreationTime();
				handleVanishedOrder(entry, actionList, lastTimestampWhenPresent);
			}
		}

		// These are the orders from the updated list that did not matched any order
		if (updatedList.size() > 0)
		{
			std::stringstream orderListStream;
			for (size_t i = 0; i<updatedList.size(); i++)
			{
				orderListStream << ((i) ? ", " : "") << updatedList[i].getIdForTrace();
				m_list.push_back(TrackOrderEntry(std::move(updatedList[i]), /*isPlaceHolder*/false));
			}
			IRSTD_LOG_WARNING(TraderTrackOrder, "The order(s) [" << orderListStream.str()
					<< "] did not match any known orders");
		}

		// Assess if the list has been updated or not
		isListUpdated |= !(m_list == trackOrderOriginalList);
		scope.release();

		// Process orders, this orders operates on a copy of the tracklist, hence no scope is needed
		for (auto& action : actionList)
		{
			const auto& trackOrder = action.m_trackOrder;

			switch (action.m_type)
			{
			case TrackOrderAction::Type::PROCESS:
				{
					const auto amount = trackOrder.getAmount();
					const auto& order = trackOrder.getOrder();
					const auto amountTotal = action.m_amount;

					bool triggerOnComplete = false;
					std::stringstream messageStream;
					if (order.isFirstValid(amount / 2.))
					{
						messageStream << "Processed " << amount << " " << order.getInitialCurrency();
						if (order.getFirstOrderFinalCurrency() != Currency::NONE)
						{
								messageStream << " -> " << order.getFirstOrderFinalAmount(amount)
										<< " " << order.getFirstOrderFinalCurrency();
						}

						IRSTD_LOG_INFO(TraderTrackOrder, trackOrder.getIdForTrace()
								<< " is" << ((amount < amountTotal) ? " partially" : "")
								<< " completed with amount " << amount);

						triggerOnComplete = true;
					}
					else
					{
						messageStream << "Ignoring Amount (" << amount << " " << order.getInitialCurrency()
								<< ") processed for " << trackOrder.getIdForTrace() << " (too small)";

						IRSTD_LOG_WARNING(TraderTrackOrder, messageStream.str().c_str());
					}

					// Add the record before proceeding with the next order for readability in the records
					addRecord((amount < amountTotal) ? RecordType::PARTIAL : RecordType::PROCEED,
								trackOrder, messageStream.str().c_str());

					if (triggerOnComplete)
					{
						m_eventManager.triggerOnOrderComplete(trackOrder, amount);
					}
				}
				break;
			case TrackOrderAction::Type::FAILED:
				m_eventManager.triggerOnOrderError(trackOrder);
				break;
			case TrackOrderAction::Type::TIMEOUT:
				m_eventManager.triggerOnOrderTimeout(trackOrder);
				break;
			default:
				IRSTD_UNREACHABLE(TraderTrackOrder);
			}
		}
	}

	m_timestampUnsync[1] = m_timestampUnsync[0];
	m_timestampUnsync[0] = timestampBalanceBeforeOrder;
	return isListUpdated;
}

void Trader::TrackOrderList::updateBalance(const Balance& balance) noexcept
{
	m_balanceMovements.update(balance);
}

void Trader::TrackOrderList::reserveBalance(Balance& balance) const
{
	Balance reserveMaxFunds;

	// Compute the maximum amount of funds that should be considered.
	{
		auto scope = m_lockOrders.readScope();
		for (const auto& entry : m_list)
		{
			const auto& order = entry.getTrackOrder().getOrder();
			// Only consider orders with a valid next orders
			if (!order.getNext())
			{
				continue;
			}
			// Expected final amount/current
			const auto finalAmount = order.getFirstOrderFinalAmount(entry.getTrackOrder().getAmount());
			reserveMaxFunds.add(order.getFirstOrderFinalCurrency(), finalAmount);
		}
	}

	// Check if any of this amount has been in the last X time
	{
		const auto timestamp = getCurrentTimestamp();

		// Look at all balance variations within the last m_timeoutOrderRegisteredMs * 2 time
		m_balanceMovements.get(timestamp, timestamp - m_timeoutOrderRegisteredMs * 2, [&](
				const IrStd::Type::Timestamp /*timestamp*/, const IrStd::Type::Decimal amount, const CurrencyPtr currency) {

			// If the amount is positive, the currency matches and there are funds in the balance
			const auto maxAmount = reserveMaxFunds.get(currency);
			const auto availableBalance = balance.get(currency);
			if (amount > 0 && maxAmount > 0 && availableBalance > 0)
			{
				const auto reserve = std::min(amount, std::min(availableBalance, maxAmount));
				reserveMaxFunds.add(currency, -reserve);
				balance.reserve(currency, reserve);
			}
		});
	}
}

bool Trader::TrackOrderList::cancelTimeout(
		const IrStd::Type::Timestamp timestamp,
		const std::function<bool(const TrackOrder&)>& cancelOrder)
{
	bool atLeastOne = false;

	{
		auto scope = m_lockOrders.readScope();
		for (auto& entry : m_list)
		{
			if (!entry.isCancel() && !entry.isPlaceHolder() && entry.getTrackOrder().isTimeout(timestamp))
			{
				IRSTD_LOG_INFO(TraderTrackOrder, "Timeout for order: " << entry.getTrackOrder()
						<< ", current.timestamp=" << timestamp);

				// Make a copy of the tracking order
				const auto trackOrder = entry.getTrackOrder();
				// Invalid the timeout to make sure it does not call the function twice
				const size_t timeoutS = trackOrder.getOrder().getTimeout();
				scope.release();

				// Set timeout to entry 
				{
					std::stringstream messageStream;
					messageStream << "Timeout expired (" << timeoutS << "s)";
					remove(RemoveCause::TIMEOUT, trackOrder.getId(), messageStream.str().c_str(), /*mustExists*/false);
				}

				// Send the cancel order
				cancelOrder(trackOrder);
				atLeastOne = true;;
			}
		}
	}

	return atLeastOne;
}

IrStd::Type::Timestamp Trader::TrackOrderList::getCurrentTimestamp() const noexcept
{
	return IrStd::Type::Timestamp::now();
}

void Trader::TrackOrderList::addRecord(
		const RecordType type,
		const TrackOrder& track,
		const char* const pMessage) noexcept
{
	const auto contextId = ((track.getContext()) ? track.getContext()->getId() : 0);
	const auto strategyId = ((track.getContext()) ? track.getContext<OperationContext>()->getStrategyId() : Id());
	const OrderState state{
		type,
		track.getId(),
		track.getType(),
		track.getOrder().getInitialCurrency(),
		track.getOrder().getFirstOrderFinalCurrency(),	
		track.getAmount(),
		track.getOrder().getRate(),
		contextId,
		std::string(pMessage),
		strategyId
	};
	m_orderRecordList.push(getCurrentTimestamp(), state);
}

void Trader::TrackOrderList::getRecords(const std::function<void(
		const IrStd::Type::Timestamp,
		const char* const,
		const Id,
		const char* const,
		const CurrencyPtr,
		const CurrencyPtr,
		const IrStd::Type::Decimal,
		const IrStd::Type::Decimal,
		const size_t,
		const char* const,
		const Id)>& callback) const noexcept
{
	m_orderRecordList.read([&](const IrStd::Type::Timestamp& timestamp, const OrderState& state) {
		// Identify the type of record
		const char* pRecordType;
		switch (state.m_type)
		{
		case RecordType::PLACE:
			pRecordType = "place";
			break;
		case RecordType::PARTIAL:
			pRecordType = "partial";
			break;
		case RecordType::PROCEED:
			pRecordType = "proceed";
			break;
		case RecordType::CANCEL:
			pRecordType = "cancel";
			break;
		case RecordType::FAILED:
			pRecordType = "failed";
			break;
		case RecordType::TIMEOUT:
			pRecordType = "timeout";
			break;
		default:
			IRSTD_UNREACHABLE(TraderTrackOrder);
		}
		callback(timestamp, pRecordType, state.m_id, TrackOrder::getTypeToString(state.m_orderType),
				state.m_initialCurrency, state.m_finalCurrency, state.m_amount, state.m_rate, state.m_contextId,
				state.m_message.c_str(), state.m_strategyId);
	}, 20);
}

// ---- Trader::TrackOrderList (print) ----------------------------------------

void Trader::TrackOrderList::toStream(std::ostream& out) const
{
	auto scope = m_lockOrders.readScope();
	out << "Active order(s): " << m_list.size() << std::endl;
	for (auto& entry : m_list)
	{
		out << "  ";
		entry.toStream(out);
		out << std::endl;
	}
}

std::ostream& operator<<(std::ostream& os, const Trader::TrackOrderList& list)
{
	list.toStream(os);
	return os;
}
