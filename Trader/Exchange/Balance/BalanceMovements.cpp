#include "Trader/Exchange/Balance/BalanceMovements.hpp"

IRSTD_TOPIC_REGISTER(Trader, Balance, Movements);
IRSTD_TOPIC_USE_ALIAS(TraderBalanceMovements, Trader, Balance, Movements);

// ---- Trader::BalanceMovements ----------------------------------------------

Trader::BalanceMovements::BalanceMovements()
		: m_timestamp(0)
{
}

void Trader::BalanceMovements::update(const Balance& balance)
{
	auto scope = m_lock.writeScope();
	m_timestamp = IrStd::Type::Timestamp::now();
	const bool isNew = m_fundList.empty();

	// Compare currencies that are registered
	for (const auto& it : m_fundList)
	{
		const auto currency = it.first;
		const auto amount = it.second;
		const auto newAmount = balance.getWithReserve(currency);

		// If the amount is different
		if (amount != newAmount)
		{
			m_fundList[currency] = newAmount;
			m_movements.push(m_timestamp, std::make_pair(newAmount - amount, currency));
		}
	}

	// Check if there are any new currencies
	balance.getCurrencies([&](const CurrencyPtr currency) {
		if (m_fundList.find(currency) == m_fundList.end())
		{
			const auto amount = balance.getWithReserve(currency);
			m_fundList[currency] = amount;
			// DO not register changes from teh first time (this is not a change but a settlement)
			if (!isNew)
			{
				m_movements.push(m_timestamp, std::make_pair(amount, currency));
			}
		}
	});
}

IrStd::Type::Timestamp Trader::BalanceMovements::getLastUpdateTimestamp() const noexcept
{
	return m_timestamp;
}

IrStd::Type::Decimal Trader::BalanceMovements::consume(
		const IrStd::Type::Timestamp oldTimestamp,
		const IrStd::Type::Decimal amount,
		const CurrencyPtr currency) noexcept
{
	auto scope = m_lock.writeScope();

	if (m_movements.size() == 0)
	{
		return amount;
	}

	auto pos = m_movements.find(oldTimestamp, /*oldest*/true);
	// The timestamp requested is newer than anything registered
	// this woudl happen if no changes in the balance has been detected
	if (pos == 0)
	{
		return amount;
	}
	IRSTD_ASSERT(TraderBalanceMovements, !m_movements.isIndexTooOld(pos),
			"Since this is protected by a lock, the returned value should never be too old");
	auto amountLeft = amount;
	auto isPositive = (amountLeft > 0);
	while (!m_movements.isIndexTooNew(pos) && amountLeft)
	{
		auto& entry = m_movements.loadForWrite(pos).second;
		if (entry.second == currency
				&& ((isPositive && entry.first > 0)
				|| (!isPositive && entry.first < 0)))
		{
			const auto amountConsumed = (isPositive) ? std::min(amountLeft, entry.first)
					: std::max(amountLeft, entry.first);
			amountLeft -= amountConsumed;
			entry.first -= amountConsumed;
		}

		++pos;
	}

	return amountLeft;
}

bool Trader::BalanceMovements::get(
		const IrStd::Type::Timestamp fromTimestamp,
		const IrStd::Type::Timestamp toTimestamp,
		std::function<void(const IrStd::Type::Timestamp, const IrStd::Type::Decimal, const CurrencyPtr)> callback) const noexcept
{
	auto scope = m_lock.readScope();
	return m_movements.readIntervalByKey(fromTimestamp, toTimestamp,
			[&](const IrStd::Type::Timestamp timestamp, const std::pair<IrStd::Type::Decimal, CurrencyPtr> change) {
		callback(timestamp, change.first, change.second);
	});
}

void Trader::BalanceMovements::clear() noexcept
{
	auto scope = m_lock.writeScope();
	m_fundList.clear();
}
