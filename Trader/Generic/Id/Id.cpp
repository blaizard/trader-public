#include "Trader/Generic/Id/Id.hpp"

#define INVALID_ID "invalid"

// ----  Trader::Id -----------------------------------------------------------

size_t Trader::Id::m_uniqueId = 0;

Trader::Id::Id()
		: Id(INVALID_ID)
{
}

Trader::Id::Id(IrStd::Type::ShortString id)
{
	std::memset(m_value.data(), 0, MAX_SIZE);
	std::strncpy(m_value.data(), id, MAX_SIZE - 1);
}

Trader::Id Trader::Id::unique(const char* const pPrefix)
{
	std::string str(pPrefix);
	str.append(IrStd::Type::ShortString(++m_uniqueId));
	return Id(str.c_str());
}

bool Trader::Id::isValid() const noexcept
{
	return (*this != Id(INVALID_ID));
}

bool Trader::Id::operator==(const Id& v) const noexcept
{
	return (std::memcmp(m_value.data(), v.m_value.data(), MAX_SIZE) == 0);
}

bool Trader::Id::operator!=(const Id& v) const noexcept
{
	return !(*this == v);
}

bool Trader::Id::operator<(const Id& v) const noexcept
{
	return (std::memcmp(m_value.data(), v.m_value.data(), MAX_SIZE) < 0);
}

bool Trader::Id::operator>(const Id& v) const noexcept
{
	return (std::memcmp(m_value.data(), v.m_value.data(), MAX_SIZE) > 0);
}

void Trader::Id::toStream(std::ostream& os) const
{
	os << m_value.data();
}

std::ostream& operator<<(std::ostream& os, const Trader::Id& id)
{
	id.toStream(os);
	return os;
}

const char* Trader::Id::c_str() const noexcept
{
	return m_value.data();
}

Trader::Id::operator const char*() const noexcept
{
	return m_value.data();
}
