#include "Trader/Generic/Event/Context.hpp"

IRSTD_TOPIC_REGISTER(Trader, Context);

// ---- Trader::Context -------------------------------------------------------

namespace
{
	static size_t ContextIdCounter = 0;
}

Trader::Context::Context()
		: m_id(++::ContextIdCounter)
{
}

void Trader::Context::toStream(std::ostream& os) const
{
	os << "id=" << getId();
}

size_t Trader::Context::getId() const noexcept
{
	return m_id;
}

std::ostream& operator<<(std::ostream& os, const Trader::ContextHandle& context)
{
	if (context)
	{
		context->toStream(os);
	}
	else
	{
		os << "<invalid>";
	}
	return os;
}
