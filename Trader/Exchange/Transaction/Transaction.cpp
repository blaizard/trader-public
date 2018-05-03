#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Transaction/Transaction.hpp"

IRSTD_TOPIC_REGISTER(Trader, Transaction);
IRSTD_TOPIC_USE_ALIAS(TraderTransaction, Trader, Transaction);

Trader::Transaction::Transaction(
		const CurrencyPtr initialCurrency,
		const CurrencyPtr finalCurrency)
		: m_data({0,0})
		, m_initalCurrency(initialCurrency)
		, m_finalCurrency(finalCurrency)
		, m_decimalPlace(14)
		, m_decimalPlaceOrder(14)
		, m_isFirst(true)
{
}

Trader::Transaction::Transaction(const Transaction& transaction)
		: m_data(transaction.m_data.load())
		, m_initalCurrency(transaction.m_initalCurrency)
		, m_finalCurrency(transaction.m_finalCurrency)
		, m_decimalPlace(transaction.m_decimalPlace)
		, m_decimalPlaceOrder(transaction.m_decimalPlaceOrder)
		, m_isFirst(transaction.m_isFirst)
{
}

bool Trader::Transaction::operator==(const Transaction& transaction) const noexcept
{
	return (this == &transaction)
			|| (m_initalCurrency == transaction.m_initalCurrency
			&& m_finalCurrency == transaction.m_finalCurrency
			&& m_decimalPlace == transaction.m_decimalPlace
			&& m_decimalPlaceOrder == transaction.m_decimalPlaceOrder);
}

bool Trader::Transaction::operator!=(const Transaction& transaction) const noexcept
{
	return !(*this == transaction);
}

void Trader::Transaction::setRate(const IrStd::Type::Decimal rate, const IrStd::Type::Timestamp timestamp)
{
	const auto currentTimestamp = getTimestamp();

	if (timestamp < currentTimestamp)
	{
		IRSTD_LOG_ERROR(TraderTransaction, "The new timestamp of the transaction ("
				<< timestamp << ") is anterior than the current timestamp ("
				<< currentTimestamp << "), ignoring");
		return;
	}

	// Get and format the rate
	const auto currentRate = getRate();
	const IrStd::Type::Decimal newRate = IrStd::Type::doubleFormat(rate,
			getDecimalPlace(), IrStd::TypeFormat::FLAG_ROUND);

	if (newRate != currentRate && timestamp == currentTimestamp)
	{
	//	IRSTD_LOG_WARNING(TraderTransaction, "The timestamp of the transaction " << *this
	//			<< " did not changed (" << timestamp << ") but the rate did ("
	//			<< currentRate << " -> " << newRate << ")");
	}

	IRSTD_THROW_ASSERT(TraderTransaction, newRate > 0., "Rate for " << *this
			<< " must be greater than zero, rate=" << rate << ", formated.rate=" << newRate);

	// Update only if rates are different
	if (m_isFirst || newRate != currentRate)
	{
		const auto data = m_data.load();
		// Don't push the first element, it is invalid
		if (!m_isFirst)
		{
			m_previousRates.push(data.m_timestamp, data.m_rate);
		}
		m_isFirst = false;
		m_data.store({newRate, timestamp});
	}
}

IrStd::Type::Decimal Trader::Transaction::getRate() const noexcept
{
	//IRSTD_THROW_ASSERT(TraderTransaction, !m_isFirst, "There is no data yet");
	const auto data = m_data.load();
	return data.m_rate;
}

IrStd::Type::Decimal Trader::Transaction::getRate(const int position) const
{
	IRSTD_ASSERT(TraderTransaction, position <= 0, "The rate history position cannot be in the future");
	IRSTD_THROW_ASSERT(TraderTransaction, !m_isFirst, "There is no data yet");
	IRSTD_THROW_ASSERT(TraderTransaction, -position <= static_cast<int>(m_previousRates.size()),
			"Only history not older than " << m_previousRates.size()
			<< " previous data points is supported, requested " << position
			<< " for " << *this);

	return (position == 0) ? getRate() : m_previousRates.head(-(position + 1)).second;
}

size_t Trader::Transaction::getNbRates() const noexcept
{
	return (m_isFirst) ? 0 : m_previousRates.size() + 1;
}

bool Trader::Transaction::getRates(
		const IrStd::Type::Timestamp fromTimestamp,
		const IrStd::Type::Timestamp toTimestamp,
		std::function<void(const IrStd::Type::Timestamp, const IrStd::Type::Decimal)> callback) const noexcept
{
	bool isFirst = true;
	const auto curData = m_data.load();
	const bool isComplete = m_previousRates.readIntervalByKey(fromTimestamp, toTimestamp,
			[&](const IrStd::Type::Timestamp timestamp, const IrStd::Type::Decimal rate) {
				// Include the current rate if the timestamp is included
				if (isFirst)
				{
					const auto data = m_data.load();
					if (fromTimestamp >= data.m_timestamp)
					{
						callback(data.m_timestamp, data.m_rate);
					}
					isFirst = false;
				}
				callback(timestamp, rate);
			});

	// If nothing has been processed, check if the first order is a candidate
	if (isFirst && fromTimestamp >= curData.m_timestamp)
	{
		callback(curData.m_timestamp, curData.m_rate);
	}

	return isComplete;
}

void Trader::Transaction::setDecimalPlace(const size_t decimalPlace) noexcept
{
	IRSTD_ASSERT(TraderTransaction, decimalPlace < 15, "Precision is too high and not supported");
	m_decimalPlace = decimalPlace;
}

size_t Trader::Transaction::getDecimalPlace() const noexcept
{
	return m_decimalPlace;
}

void Trader::Transaction::setOrderDecimalPlace(const size_t decimalPlace) noexcept
{
	IRSTD_ASSERT(TraderTransaction, decimalPlace < 15, "Precision is too high and not supported");
	m_decimalPlaceOrder = decimalPlace;
}

size_t Trader::Transaction::getOrderDecimalPlace() const noexcept
{
	return m_decimalPlaceOrder;
}

IrStd::Type::Timestamp Trader::Transaction::getTimestamp() const noexcept
{
	const auto data = m_data.load();
	return data.m_timestamp;
}

IrStd::Type::Decimal Trader::Transaction::getFinalAmount(const IrStd::Type::Decimal amount, const bool includeFee) const noexcept
{
	return (includeFee) ? getFinalAmountImpl(amount, getRate()) : (amount * getRate());
}

IrStd::Type::Decimal Trader::Transaction::getFinalAmount(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate, const bool includeFee) const noexcept
{
	return (includeFee) ? getFinalAmountImpl(amount, rate) : (amount * rate);
}

IrStd::Type::Decimal Trader::Transaction::getInitialAmount(const IrStd::Type::Decimal amount, const bool includeFee) const noexcept
{
	IRSTD_ASSERT(TraderTransaction, getRate(), "Transaction::getRate() == 0");
	return (includeFee) ? getInitialAmountImpl(amount, getRate()) : (amount / getRate());
}

IrStd::Type::Decimal Trader::Transaction::getInitialAmount(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate, const bool includeFee) const noexcept
{
	IRSTD_ASSERT(TraderTransaction, rate, "rate == 0");
	return (includeFee) ? getInitialAmountImpl(amount, rate) : (amount / rate);
}

Trader::CurrencyPtr Trader::Transaction::getInitialCurrency() const noexcept
{
	return m_initalCurrency;
}

Trader::CurrencyPtr Trader::Transaction::getFinalCurrency() const noexcept
{
	return m_finalCurrency;
}

void Trader::Transaction::toStream(std::ostream& out) const
{
	toStreamPair(out);
	out << "(decimal=" << m_decimalPlace << ")";
}

void Trader::Transaction::toStreamPair(std::ostream& out) const
{
	out << getInitialCurrency() << "/" << getFinalCurrency();
}

std::ostream& operator<<(std::ostream& os, const Trader::Transaction& transaction)
{
	transaction.toStream(os);
	return os;
}
