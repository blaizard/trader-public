#pragma once

#include <ostream>
#include "IrStd/IrStd.hpp"

namespace Trader
{
	class Boundaries
	{
	public:
		Boundaries();

		Boundaries getInvert() const noexcept;

		bool checkInitialAmount(const IrStd::Type::Decimal cost) const noexcept;
		bool checkFinalAmount(const IrStd::Type::Decimal amount) const noexcept;
		bool checkRate(const IrStd::Type::Decimal rate) const noexcept;

		void setInitialAmount(const IrStd::Type::Decimal minInitialAmount, const IrStd::Type::Decimal maxInitialAmount = 0) noexcept;
		void setFinalAmount(const IrStd::Type::Decimal minInitialAmount, const IrStd::Type::Decimal maxInitialAmount = 0) noexcept;
		void setRate(const IrStd::Type::Decimal minRate, const IrStd::Type::Decimal maxRate = 0) noexcept;

		/**
		 * Merge 2 boundaries together
		 */
		void merge(const Boundaries& boundaries) noexcept;

		void toStream(std::ostream& out) const;

	private:
		class Interval
		{
		public:
			Interval(const IrStd::Type::Decimal min = 0, const IrStd::Type::Decimal max = 0)
					: m_min(min)
					, m_max(max)
			{
			}

			IrStd::Type::Decimal getMin() const noexcept
			{
				return m_min;
			}

			IrStd::Type::Decimal getMax() const noexcept
			{
				return m_max;
			}

			IrStd::Type::Decimal getInvertMin() const noexcept
			{
				return (m_min) ? IrStd::Type::Decimal(1.) / m_min : IrStd::Type::Decimal(0);
			}

			IrStd::Type::Decimal getInvertMax() const noexcept
			{
				return (m_max) ? IrStd::Type::Decimal(1.) / m_max : IrStd::Type::Decimal(0);
			}

			bool check(const IrStd::Type::Decimal number) const noexcept
			{
				return (!m_min || number >= m_min) && (!m_max || number <= m_max);
			}

			void merge(const Interval& interval) noexcept
			{
				// Keep the most restrictive
				m_min = (interval.m_min > m_min) ? interval.m_min : m_min;
				m_max = (interval.m_max < m_max) ? interval.m_max : m_max;
			}

			void toStream(const char* const name, std::ostream& out, bool& separator) const
			{
				if (m_min && m_max)
				{
					out << ((separator) ? ", " : "") << name << "=[" << m_min << ", " << m_max << "]";
					separator = true;
				}
				else if (m_min)
				{
					out << ((separator) ? ", " : "") << name << "\u2265" << m_min;
					separator = true;
				}
				else if (m_max)
				{
					out << ((separator) ? ", " : "") << name << "\u2264" << m_max;
					separator = true;
				}
			}

		private:
			IrStd::Type::Decimal m_min;
			IrStd::Type::Decimal m_max;
		};

		//! Intial amount
		Interval m_initialAmount;
		//! Final amount
		Interval m_finalAmount;
		//! Rate
		Interval m_rate;
	};
}
