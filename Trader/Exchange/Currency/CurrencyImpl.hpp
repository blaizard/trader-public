#pragma once

namespace Trader
{
	class CurrencyImpl
	{
	public:
		constexpr CurrencyImpl(const char* const pId, const char* const pName,
			std::initializer_list<const char* const> pMatches, const bool isFiat, const double minAmount = 0)
				: m_pId(pId)
				, m_pName(pName)
				, m_pMatches(pMatches)
				, m_isFiat(isFiat)
				, m_minAmount(minAmount)
		{
		}

		constexpr const char* getId() const noexcept
		{
			return m_pId;
		}

		std::initializer_list<const char* const> getMatches() const noexcept
		{
			return m_pMatches;
		}

		IrStd::Type::Decimal getMinAmount() const noexcept
		{
			return IrStd::Type::Decimal{m_minAmount};
		}

		bool isFiat() const noexcept
		{
			return m_isFiat;
		}

		bool operator==(const CurrencyImpl &c) const noexcept
		{
			return this == &c;
		}

		bool operator!=(const CurrencyImpl &c) const noexcept
		{
			return this != &c;
		}

	private:
		const char* const m_pId;
		const char* const m_pName;
		std::initializer_list<const char* const> m_pMatches;
		const bool m_isFiat;
		const double m_minAmount; /// minimal amount allowed for trading
	};
}
