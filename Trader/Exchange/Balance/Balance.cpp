#include "Trader/Exchange/Balance/Balance.hpp"
#include "Trader/Exchange/Exchange.hpp"

IRSTD_TOPIC_REGISTER(Trader, Balance);
IRSTD_TOPIC_USE_ALIAS(TraderBalance, Trader, Balance);

// ---- Trader::Balance -------------------------------------------------------

constexpr double Trader::Balance::INVALID_AMOUNT;

Trader::Balance::Balance()
		: m_pExchange(nullptr)
		, m_initialEstimate(Trader::Balance::INVALID_AMOUNT)
{
}

Trader::Balance::Balance(Exchange& exchange)
		: m_pExchange(&exchange)
		, m_initialEstimate(Trader::Balance::INVALID_AMOUNT)
{
}

Trader::Balance::Balance(Balance&& rhs)
{
	*this = std::move(rhs);
}

Trader::Balance& Trader::Balance::operator=(Balance&& rhs)
{
	auto scope1 = m_lock.writeScope();
	auto scope2 = rhs.m_lock.readScope();

	m_fundList = std::move(rhs.m_fundList);
	m_reservedFundList = std::move(rhs.m_reservedFundList);
	m_pExchange = rhs.m_pExchange;
	m_initialEstimate = rhs.m_initialEstimate;

	return *this;
}

void Trader::Balance::compareFunds(
		const Balance& balance,
		const std::function<void(const CurrencyPtr, const IrStd::Type::Decimal)>& callback) const noexcept
{
	// Lock both balances for reading
	auto scope1 = m_lock.readScope();
	auto scope2 = balance.m_lock.readScope();

	// Check currencies that are present in the current balance
	getCurrencies([&](const CurrencyPtr currency) {
		const auto diffAmount = balance.getWithReserve(currency) - getWithReserve(currency);
		if (diffAmount)
		{
			callback(currency, diffAmount);
		}
	});

	// Check currencies that are only present in the second balance
	balance.getCurrencies([&](const CurrencyPtr currency) {
		if (m_fundList.find(currency) == m_fundList.end())
		{
			callback(currency, balance.getWithReserve(currency));
		}
	});
}

void Trader::Balance::setFunds(const Balance& balance)
{
	auto scope1 = m_lock.writeScope();
	auto scope2 = balance.m_lock.readScope();
	m_fundList = balance.m_fundList;
}

void Trader::Balance::setFundsAndUpdateReserve(
		const Balance& balance,
		const Exchange& exchange,
		const bool isBalanceincludeReserve)
{
	auto scope1 = m_lock.writeScope();
	auto scope2 = balance.m_lock.readScope();
	m_fundList = balance.m_fundList;

	// Update the money reserved
	{
		m_reservedFundList.clear();
		exchange.getTrackOrderList().each([&](const TrackOrder& track) {
			const auto currency = track.getOrder().getInitialCurrency();
			const auto amount = track.getAmount();
			// If the balance does not include the reserve, add it
			if (!isBalanceincludeReserve)
			{
				const auto currentAmount = getWithReserveNoLock(currency);
				m_fundList[currency] = currentAmount + amount;
			}
			reserveNoLock(currency, amount);
		});
	}
}

void Trader::Balance::updateReserve(const Exchange& exchange)
{
	{
		auto scope = m_lock.writeScope();

		m_reservedFundList.clear();
		exchange.getTrackOrderList().each([&](const TrackOrder& track) {
			const auto currency = track.getOrder().getInitialCurrency();
			const auto amount = track.getAmount();
			reserveNoLock(currency, amount);
		});
	}
}

void Trader::Balance::clear() noexcept
{
	auto scope = m_lock.writeScope();
	m_fundList.clear();
	m_reservedFundList.clear();
}

bool Trader::Balance::empty() const noexcept
{
	return m_fundList.empty() && m_reservedFundList.empty();
}

void Trader::Balance::set(const CurrencyPtr currency, const IrStd::Type::Decimal amount)
{
	auto scope = m_lock.writeScope();
	m_fundList[currency] = amount;
}

void Trader::Balance::add(const CurrencyPtr currency, const IrStd::Type::Decimal amount)
{
	auto scope = m_lock.writeScope();
	const auto currentAmount = getWithReserveNoLock(currency);
	m_fundList[currency] = currentAmount + amount;
}

void Trader::Balance::reserve(const CurrencyPtr currency, const IrStd::Type::Decimal amount)
{
	auto scope = m_lock.writeScope();
	reserveNoLock(currency, amount);
}

void Trader::Balance::reserveNoLock(const CurrencyPtr currency, const IrStd::Type::Decimal amount)
{
	IRSTD_ASSERT(TraderBalance, m_lock.isWriteScope(), "This operation can only be processed under a valid scope");
	auto it = m_reservedFundList.find(currency);
	if (it == m_reservedFundList.end())
	{
		m_reservedFundList[currency] = amount;
	}
	else
	{
		m_reservedFundList[currency] += amount;
	}
}

IrStd::Type::Decimal Trader::Balance::getWithReserveNoLock(const CurrencyPtr currency) const noexcept
{
	IRSTD_ASSERT(TraderBalance, m_lock.isScope(), "This operation can only be processed under a valid scope");
	auto it = m_fundList.find(currency);
	return (it == m_fundList.end()) ? IrStd::Type::Decimal(0) : it->second;
}

IrStd::Type::Decimal Trader::Balance::getWithReserve(const CurrencyPtr currency) const noexcept
{
	auto scope = m_lock.readScope();
	return getWithReserveNoLock(currency);
}

IrStd::Type::Decimal Trader::Balance::get(const CurrencyPtr currency) const noexcept
{
	auto scope = m_lock.readScope();
	auto amount = getWithReserveNoLock(currency);

	// Subtract the reserved part
	const auto it = m_reservedFundList.find(currency);
	if (it != m_reservedFundList.end())
	{
		amount -= it->second;
	}

	return amount;
}

void Trader::Balance::getCurrencies(const std::function<void(const CurrencyPtr)>& callback) const noexcept
{
	auto scope = m_lock.readScope();
	for (const auto it : m_fundList)
	{
		callback(it.first);
	}
}

IrStd::Type::Decimal Trader::Balance::estimate(
	const CurrencyPtr currency,
	const IrStd::Type::Decimal amount,
	const Exchange* const pExchange) const noexcept
{
	IRSTD_ASSERT(TraderBalance, pExchange, "Balance::estimate can only be called with an associated exchange");

	if (amount == 0)
	{
		return 0;
	}
	{
		const auto pOrderChain = pExchange->identifyOrderChain(currency, pExchange->getEstimateCurrency());
		if (pOrderChain)
		{
			return pOrderChain->getFinalAmount(amount, /*includeFee*/false);
		}
		else
		{
			IRSTD_LOG_WARNING(TraderBalance, "No order chain available for pair " << currency << "/"
					<< pExchange->getEstimateCurrency() << " (" << pExchange->getId() << ")");
		}
	}
	return Trader::Balance::INVALID_AMOUNT;
}

IrStd::Type::Decimal Trader::Balance::estimate(
	const Exchange* const pExchange) const noexcept
{
	IRSTD_ASSERT(TraderBalance, pExchange, "Balance::estimate can only be called with an associated exchange");
	IrStd::Type::Decimal value = 0;

	{
		auto scope = m_lock.readScope();
		for (const auto& fund : m_fundList)
		{
			const auto curEstimate = estimate(fund.first, fund.second, pExchange);
			if (curEstimate == Trader::Balance::INVALID_AMOUNT)
			{
				return Trader::Balance::INVALID_AMOUNT;
			}
			value += curEstimate;
		}
	}

	return value;
}

void Trader::Balance::toJson(IrStd::Json& json, const Exchange* const pExchange) const
{
	IRSTD_ASSERT(TraderBalance, pExchange || m_pExchange, "Balance::toJson can only be called with an associated exchange");

	const auto pCurExchange = (pExchange) ? pExchange : m_pExchange;
	IrStd::Type::Decimal estimateBalance = 0;

	{
		auto scope = m_lock.readScope();
		for (const auto& fund : m_fundList)
		{
			const auto currency = fund.first;
			const auto amount = fund.second;
			// Look if there are reserved currency
			const auto it = m_reservedFundList.find(fund.first);
			const auto reserve = (it != m_reservedFundList.end()) ? it->second : IrStd::Type::Decimal(0.);
			const auto estimateAmount = estimate(currency, amount, pCurExchange);

			if (estimateAmount == Trader::Balance::INVALID_AMOUNT)
			{
				const IrStd::Json itemJson({{"amount", amount}, {"reserve", reserve}});
				json.add(currency->getId(), itemJson);
			}
			else 
			{
				const IrStd::Json itemJson({{"amount", amount}, {"reserve", reserve}, {"estimate", estimateAmount}});
				json.add(currency->getId(), itemJson);
				estimateBalance += estimateAmount;
			}
		}

		// Add estimate and diff from begining and estimate currency
		{
			const IrStd::Json itemJson({{"estimate", estimateBalance}, {"initial",
					(m_initialEstimate == Trader::Balance::INVALID_AMOUNT) ? IrStd::Type::Decimal(0.) : m_initialEstimate}});
			json.add("total", itemJson);
			json.add("estimate", pCurExchange->getEstimateCurrency()->getId());
		}
	}
}

void Trader::Balance::toStream(std::ostream& out) const
{
	auto scope = m_lock.readScope();
	out << std::left << "      " << std::setw(16) << std::left << "Funds" << "("
			<< std::setw(16) << "Reserved" << ")";
	if (m_pExchange)
	{
		out << "  ~" << std::setw(16) << m_pExchange->getEstimateCurrency();
	}
	out << std::endl;

	IrStd::Type::Decimal balanceValue = 0;
	for (const auto& fund : m_fundList)
	{
		const auto amount = fund.second;
		const auto currency = fund.first;

		out << std::setw(4) << currency << ": " << std::setw(16) << amount;

		// Look if there are reserved currency
		auto it = m_reservedFundList.find(fund.first);
		if (it != m_reservedFundList.end())
		{
			out << "(" << std::setw(16) << it->second << ")";
		}
		else
		{

			out << std::setw(18) << " ";
		}

		// Calculate the estimate if any
		if (m_pExchange)
		{
			out << "  " << std::setw(16);

			const auto curEstimate = estimate(currency, amount, m_pExchange);
			if (curEstimate == Trader::Balance::INVALID_AMOUNT)
			{
				out << "?";
			}
			else
			{
				out << curEstimate;
				balanceValue += curEstimate;
			}
		}

		out << std::endl;
	}

	if (m_pExchange)
	{
		if (m_initialEstimate == Trader::Balance::INVALID_AMOUNT || !m_initialEstimate)
		{
			m_initialEstimate = balanceValue;
		}
		out << std::right << std::setw(6 + 16 + 18 + 2) << "Total: "
				<< balanceValue << " " << m_pExchange->getEstimateCurrency();
		if (m_initialEstimate)
		{
			const auto percentage = (balanceValue / m_initialEstimate - 1) * 100;
			out << " (" << ((percentage > 0.) ? "+" : "")
					<< IrStd::Type::ShortString(percentage, 2)
					<< "%)";
		}
		out << std::endl;
	}
}

std::ostream& operator<<(std::ostream& os, Trader::Balance& balance)
{
	balance.toStream(os);
	return os;
}
