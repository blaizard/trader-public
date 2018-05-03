#pragma once

#include "IrStd/IrStd.hpp"

namespace Trader
{
	class Id
	{
	private:
		static constexpr size_t MAX_SIZE = 32;

	public:
		Id();
		Id(IrStd::Type::ShortString id);

		static Id unique(const char* const pPrefix = "auto-");

		bool operator==(const Id& v) const noexcept;
		bool operator!=(const Id& v) const noexcept;
		// Needed for map/set/...
		bool operator<(const Id& v) const noexcept;
		bool operator>(const Id& v) const noexcept;

		bool isValid() const noexcept;

		const char* c_str() const noexcept;
		operator const char*() const noexcept;

		void toStream(std::ostream& os) const;

	private:
		static size_t m_uniqueId;
		std::array<char, MAX_SIZE> m_value;
	};
}

std::ostream& operator<<(std::ostream& os, const Trader::Id& id);
