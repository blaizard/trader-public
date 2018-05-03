#include "PairTransactionMap.hpp"

IRSTD_TOPIC_REGISTER(Trader, PairTransactionMap);
IRSTD_TOPIC_USE_ALIAS(TraderPairTransactionMap, Trader, PairTransactionMap);

// ---- Trader::PairTransactionMap --------------------------------------------

void Trader::PairTransactionMap::getTransactions(
		const CurrencyPtr currency,
		const std::function<void(const CurrencyPtr)>& callback) const
{
	getTransactions(currency, [&](const CurrencyPtr currencyTo, const PairTransactionPointer) {
		callback(currencyTo);
	});
}

void Trader::PairTransactionMap::getTransactions(
		const CurrencyPtr currency,
		const std::function<void(const CurrencyPtr, const PairTransactionPointer)>& callback) const
{
	auto scope = m_lock.readScope();

	const auto it1 = m_map.find(currency);
	if (it1 != m_map.end())
	{
		for (const auto& it2 : it1->second)
		{
			const auto currencyTo = it2.first;
			callback(currencyTo, it2.second);
		}
	}
}

void Trader::PairTransactionMap::getTransactions(const std::function<void(const CurrencyPtr, const CurrencyPtr)>& callback) const
{
	getTransactions([&](const CurrencyPtr currency1, const CurrencyPtr currency2, const PairTransactionPointer) {
		callback(currency1, currency2);
	});
}

void Trader::PairTransactionMap::getTransactions(const std::function<void(const CurrencyPtr, const CurrencyPtr,
		const PairTransactionPointer)>& callback) const
{
	auto scope = m_lock.readScope();

	// Loop through all transactions
	for (const auto& it1 : m_map)
	{
		const auto currencyFrom = it1.first;
		for (const auto& it2 : it1.second)
		{
			const auto currencyTo = it2.first;
			callback(currencyFrom, currencyTo, it2.second);
		}
	}
}

const Trader::PairTransactionMap::PairTransactionPointer Trader::PairTransactionMap::getTransaction(
		const CurrencyPtr from,
		const CurrencyPtr to) const noexcept
{
	IRSTD_ASSERT(TraderPairTransactionMap, from != to,
			"Cannot get a transaction for the same currency (from = to = " << from << ")");

	auto scope = m_lock.readScope();

	const auto it1 = m_map.find(from);
	if (it1 == m_map.end())
	{
		return nullptr;
	}
	const auto& subMap = it1->second;
	const auto it2 = subMap.find(to);
	if (it2 == subMap.end())
	{
		return nullptr;
	}
	return it2->second;
}

Trader::PairTransactionMap::PairTransactionPointer Trader::PairTransactionMap::getTransactionForWrite(
		const CurrencyPtr from,
		const CurrencyPtr to) noexcept
{
	IRSTD_ASSERT(TraderPairTransactionMap, from != to,
			"Cannot get a transaction for the same currency (from = to = " << from << ")");

	auto scope = m_lock.readScope();

	auto it1 = m_map.find(from);
	if (it1 == m_map.end())
	{
		return nullptr;
	}
	auto& subMap = it1->second;
	auto it2 = subMap.find(to);
	if (it2 == subMap.end())
	{
		return nullptr;
	}
	return it2->second;
}

bool Trader::PairTransactionMap::operator==(const PairTransactionMap& map) const noexcept
{
	auto scope1 = m_lock.readScope();
	auto scope2 = map.m_lock.readScope();

	{
		const auto cmp = [](decltype(*m_map.begin()) a, decltype(a) b) {
			return a.first == b.first;
		};

		// If the keys are different
		if (m_map.size() != map.m_map.size() || !std::equal(m_map.begin(), m_map.end(), map.m_map.begin(), cmp))
		{
			std::cout << "Keys are different" << std::endl;
			return false;
		}
	}

	{
		const auto cmp = [](decltype(*m_map.begin()->second.begin()) a, decltype(a) b) {
			return a.first == b.first && *(a.second) == *(b.second);
		};

		// Look through the map and check each elements
		for (const auto& it : m_map)
		{
			const auto key = it.first;
			const auto it2 = map.m_map.find(key);
			if (it.second.size() != it2->second.size() || !std::equal(it.second.begin(), it.second.end(), it2->second.begin(), cmp))
			{
				std::cout << "Key " << key << " has different data" << std::endl;
				return false;
			}
		}
	}

	return true;
}

bool Trader::PairTransactionMap::operator!=(const PairTransactionMap& map) const noexcept
{
	return !(*this == map);
}

Trader::PairTransactionMap& Trader::PairTransactionMap::operator=(const PairTransactionMap& map)
{
	auto scope1 = m_lock.writeScope();
	auto scope2 = map.m_lock.readScope();

	m_map = map.m_map;

	return *this;
}

void Trader::PairTransactionMap::clear() noexcept
{
	auto scope = m_lock.writeScope();
	m_map.clear();
}
