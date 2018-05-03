#include <iomanip>
#include "IrStd/IrStd.hpp"

#include "Trader/Exchange/Transaction/PairTransaction.hpp"
#include "Trader/Exchange/Currency/Currency.hpp"

IRSTD_TOPIC_USE_ALIAS(TraderTransaction, Trader, Transaction);

// ---- Trader::PairTransaction -------------------------------------------

Trader::PairTransaction::PairTransaction(
		const CurrencyPtr intialCurrency,
		const CurrencyPtr finalCurrency)
		: Transaction(intialCurrency, finalCurrency)
{
}

bool Trader::PairTransaction::isValid(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	const auto boundaries = getBoundaries();
	if (!boundaries.checkInitialAmount(amount))
	{
		return false;
	}
	if (!boundaries.checkRate(rate))
	{
		return false;
	}
	const auto finalAmount = getFinalAmountImpl(amount, rate);
	if (!boundaries.checkFinalAmount(finalAmount))
	{
		return false;
	}
	return true;
}

void Trader::PairTransaction::toStream(std::ostream& os) const
{
	Transaction::toStreamPair(os);
	os << " with decimal=" << getDecimalPlace() << ", ";
	getBoundaries().toStream(os);
}

void Trader::PairTransaction::setBidPrice(
		const IrStd::Type::Decimal price,
		const IrStd::Type::Timestamp timestamp)
{
	IRSTD_THROW_ASSERT(TraderTransaction, !isInvertedTransaction(),
			"The bid price must be set to the original transaction (not its pair: "
			<< *this << ")");
	auto* pTransaction = static_cast<PairTransactionImpl*>(this);
	IRSTD_THROW_ASSERT(TraderTransaction, pTransaction->m_pInvertedTransaction,
			"Inverted trasanction for " << *pTransaction << " does not exists");
	pTransaction->setRate(price, timestamp);
}

void Trader::PairTransaction::setAskPrice(
		const IrStd::Type::Decimal price,
		const IrStd::Type::Timestamp timestamp)
{
	IRSTD_THROW_ASSERT(TraderTransaction, !isInvertedTransaction(),
			"The ask price must be set to the original transaction (not its pair: "
			<< *this << ")");
	auto* pTransaction = static_cast<PairTransactionImpl*>(this);
	IRSTD_THROW_ASSERT(TraderTransaction, pTransaction->m_pInvertedTransaction,
			"Inverted transaction for " << *pTransaction << " does not exists");

	const auto formatedPrice = IrStd::Type::doubleFormat(price, pTransaction->getDecimalPlace());
	pTransaction->m_pInvertedTransaction->setRate(1. / formatedPrice, timestamp);
}

std::shared_ptr<Trader::PairTransaction> Trader::PairTransaction::getSellTransaction() const
{
	const auto pInvertTransaction = getInvertPairTransaction();

	IRSTD_THROW_ASSERT(TraderTransaction, pInvertTransaction->m_pTransaction,
			"Base transaction for " << *pInvertTransaction << " does not exists");
	return pInvertTransaction->m_pTransaction;
}

std::shared_ptr<Trader::PairTransaction> Trader::PairTransaction::getBuyTransaction() const
{
	const auto pTransaction = getPairTransaction();

	IRSTD_THROW_ASSERT(TraderTransaction, pTransaction->m_pInvertedTransaction,
			"Inverted transaction for " << *pTransaction << " does not exists");
	return pTransaction->m_pInvertedTransaction;
}

IrStd::Type::Decimal Trader::PairTransaction::getFeeFinalCurrency(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const
{
	const IrStd::Type::Decimal fee = (amount * rate) - getFinalAmount(amount, rate);
	IRSTD_THROW_ASSERT(fee >= 0, "The fee (" << fee << ") of this transaction cannot be correct");
	return fee;
}

IrStd::Type::Decimal Trader::PairTransaction::getFeeInitialCurrency(const IrStd::Type::Decimal amount, const IrStd::Type::Decimal rate) const
{
	return getFeeFinalCurrency(amount, rate) / rate;
}


void Trader::PairTransaction::setFeeFixed(const IrStd::Type::Decimal fee)
{
	auto pTransaction = getPairTransactionForWrite();
	IRSTD_THROW_ASSERT(fee >= 0, "The fixed fee (" << fee << ") of this transaction cannot be correct");
	pTransaction->m_feeFixed = fee;
}

void Trader::PairTransaction::setFeePercent(const IrStd::Type::Decimal fee)
{
	auto pTransaction = getPairTransactionForWrite();
	IRSTD_THROW_ASSERT(fee >= 0, "The percent fee (" << fee << "%) of this transaction cannot be correct");
	pTransaction->m_feePercent = fee;
}


IrStd::Type::Decimal Trader::PairTransaction::getFeeFixed() const
{
	const auto pTransaction = getPairTransaction();
	return pTransaction->m_feeFixed;
}

IrStd::Type::Decimal Trader::PairTransaction::getFeePercent() const
{
	const auto pTransaction = getPairTransaction();
	return pTransaction->m_feePercent;
}

const Trader::PairTransactionImpl* Trader::PairTransaction::getPairTransaction() const
{
	if (isInvertedTransaction())
	{
		const auto* pInvertTransaction = static_cast<const InvertPairTransactionImpl*>(this);
		IRSTD_THROW_ASSERT(TraderTransaction, pInvertTransaction->m_pTransaction,
				"Base transaction for " << *pInvertTransaction << " does not exists");
		return static_cast<const PairTransactionImpl*>(pInvertTransaction->m_pTransaction.get());
	}
	return static_cast<const PairTransactionImpl*>(this);
}

Trader::PairTransactionImpl* Trader::PairTransaction::getPairTransactionForWrite()
{
	if (isInvertedTransaction())
	{
		auto* pInvertTransaction = static_cast<InvertPairTransactionImpl*>(this);
		IRSTD_THROW_ASSERT(TraderTransaction, pInvertTransaction->m_pTransaction,
				"Base transaction for " << *pInvertTransaction << " does not exists");
		return static_cast<PairTransactionImpl*>(pInvertTransaction->m_pTransaction.get());
	}
	return static_cast<PairTransactionImpl*>(this);
}

const Trader::InvertPairTransactionImpl* Trader::PairTransaction::getInvertPairTransaction() const
{
	if (isInvertedTransaction())
	{
		return static_cast<const InvertPairTransactionImpl*>(this);
	}
	const auto* pTransaction = static_cast<const PairTransactionImpl*>(this);
	IRSTD_THROW_ASSERT(TraderTransaction, pTransaction->m_pInvertedTransaction,
			"Inverted transaction for " << *pTransaction << " does not exists");
	return static_cast<const InvertPairTransactionImpl*>(pTransaction->m_pInvertedTransaction.get());
}

bool Trader::PairTransaction::operator==(const PairTransaction& transaction) const noexcept
{
	// If same object, simply return true
	if (this == &transaction)
	{
		return true;
	}

	// Otherwise check all parameters
	return (this->Trader::Transaction::operator==(transaction)
			&& getFeeFixed() == transaction.getFeeFixed()
			&& getFeePercent() == transaction.getFeePercent());
}

bool Trader::PairTransaction::operator!=(const PairTransaction& transaction) const noexcept
{
	return !(*this == transaction);
}

// ---- Trader::PairTransactionImpl -------------------------------------------

Trader::PairTransactionImpl::PairTransactionImpl(
		const CurrencyPtr intialCurrency,
		const CurrencyPtr finalCurrency,
		const IrStd::Type::Gson data)
		: PairTransaction(intialCurrency, finalCurrency)
		, m_boundaries()
		, m_feePercent(0.)
		, m_feeFixed(0.)
		, m_data(data)
		, m_pInvertedTransaction(nullptr)
{
	// Set default boundaries
	m_boundaries.setInitialAmount(intialCurrency->getMinAmount());
	m_boundaries.setFinalAmount(finalCurrency->getMinAmount());
}

void Trader::PairTransactionImpl::setBoundaries(const Boundaries& boundaries) noexcept
{
	m_boundaries.merge(boundaries);
}

bool Trader::PairTransactionImpl::isInvertedTransaction() const noexcept
{
	return false;
}

const Trader::Boundaries Trader::PairTransactionImpl::getBoundaries() const noexcept
{
	return m_boundaries;
}

Trader::Boundaries* Trader::PairTransactionImpl::getBoundariesForWrite() noexcept
{
	return &m_boundaries;
}

const IrStd::Type::Gson& Trader::PairTransactionImpl::getData() const noexcept
{
	return m_data;
}

std::pair<IrStd::Type::ShortString, IrStd::Type::ShortString> Trader::PairTransactionImpl::getAmountAndRateForOrder(
		IrStd::Type::Decimal amount,
		IrStd::Type::Decimal rate) const
{
	const auto decimalPlace = getOrderDecimalPlace();
	const auto minPrecision = IrStd::Type::Decimal::getMinPrecision(decimalPlace);

	// Use the decimal place here
	const auto processedAmount = amount;
	IRSTD_THROW_ASSERT(getBoundaries().checkInitialAmount(processedAmount), "amount=" << amount
			<< ", minPrecision=" << minPrecision << ", processedAmount=" << processedAmount);
	const auto amountFormated = IrStd::Type::ShortString(processedAmount, getDecimalPlace(), IrStd::TypeFormat::FLAG_FLOOR);

	const auto rateFormated = IrStd::Type::ShortString(rate, decimalPlace,
			IrStd::TypeFormat::FLAG_ROUND);
	return std::make_pair(amountFormated, rateFormated);
}

IrStd::Type::Decimal Trader::PairTransactionImpl::getFinalAmountImpl(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	return (amount - m_feeFixed) * rate * (1. - m_feePercent / 100.);
}

IrStd::Type::Decimal Trader::PairTransactionImpl::getInitialAmountImpl(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	IRSTD_ASSERT(m_feePercent < 100.);
	IRSTD_ASSERT(rate);
	return amount / ((1. - m_feePercent / 100.) * rate) + m_feeFixed;
}

void Trader::PairTransactionImpl::toStream(std::ostream& os) const
{
	PairTransaction::toStream(os);
	os << ", fee=" << m_feeFixed << "+" << m_feePercent << "%";
}

// ---- Trader::InvertPairTransactionImpl -------------------------------------

Trader::InvertPairTransactionImpl::InvertPairTransactionImpl(std::shared_ptr<PairTransaction>& pTransaction)
		: PairTransaction(pTransaction->getFinalCurrency(), pTransaction->getInitialCurrency())
		, m_pTransaction(pTransaction)
{
	// Note, do not copy the decimal place as it must be different
}

bool Trader::InvertPairTransactionImpl::isInvertedTransaction() const noexcept
{
	return true;
}

std::pair<IrStd::Type::ShortString, IrStd::Type::ShortString> Trader::InvertPairTransactionImpl::getAmountAndRateForOrder(
		IrStd::Type::Decimal amount,
		IrStd::Type::Decimal rate) const
{
	const auto decimalPlace = m_pTransaction->getOrderDecimalPlace();
	const auto minPrecision = IrStd::Type::Decimal::getMinPrecision(decimalPlace);

	// Use the decimal place for the amount
	const auto processedAmount = getFinalAmountImpl(amount, rate);
	IRSTD_THROW_ASSERT(getBoundaries().checkFinalAmount(processedAmount), "amount=" << amount
			<< ", rate=" << rate << ", minPrecision= " << minPrecision
			<< ", processedAmount=" << processedAmount);
	const auto amountFormated = IrStd::Type::ShortString(processedAmount, m_pTransaction->getDecimalPlace(), IrStd::TypeFormat::FLAG_FLOOR);

	const auto processedRate = 1. / rate;
	IRSTD_THROW_ASSERT(getBoundaries().checkRate(processedRate), "rate=" << rate
			<< ", processedRate=" << processedRate);
	const auto rateFormated = IrStd::Type::ShortString(processedRate, decimalPlace, IrStd::TypeFormat::FLAG_ROUND);

	return std::make_pair(amountFormated, rateFormated);
}

IrStd::Type::Decimal Trader::InvertPairTransactionImpl::getFinalAmountImpl(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	const auto pTransaction = getTransaction<PairTransactionImpl>();
	return (amount - pTransaction->m_feeFixed) * rate * (1. - pTransaction->m_feePercent / 100.);
}

IrStd::Type::Decimal Trader::InvertPairTransactionImpl::getInitialAmountImpl(
		const IrStd::Type::Decimal amount,
		const IrStd::Type::Decimal rate) const noexcept
{
	IRSTD_ASSERT(rate);
	const auto pTransaction = getTransaction<PairTransactionImpl>();
	IRSTD_ASSERT(pTransaction->m_feePercent < 100.);
	return amount / ((1. - pTransaction->m_feePercent / 100.) * rate) + pTransaction->m_feeFixed;
}

const Trader::Boundaries Trader::InvertPairTransactionImpl::getBoundaries() const noexcept
{
	return m_pTransaction->getBoundaries().getInvert();
}

Trader::Boundaries* Trader::InvertPairTransactionImpl::getBoundariesForWrite() noexcept
{
	return nullptr;
}

const IrStd::Type::Gson& Trader::InvertPairTransactionImpl::getData() const noexcept
{
	const auto pTransaction = getTransaction<PairTransactionImpl>();
	return pTransaction->getData();
}

void Trader::InvertPairTransactionImpl::toStream(std::ostream& os) const
{
	PairTransaction::toStream(os);
	os << ", fee=" << getTransaction<PairTransactionImpl>()->m_feeFixed
			<< "+" << getTransaction<PairTransactionImpl>()->m_feePercent << "%";
}

// ---- Trader::PairTransactionNop --------------------------------------------

Trader::PairTransactionNop::PairTransactionNop()
{
	using namespace Currency;
	for (const auto& currency : {TRADER_CURRENCY_LIST})
	{
		m_nopPairTransactionMap[currency] = std::make_shared<PairTransactionImpl>(currency, currency);
		m_nopPairTransactionMap[currency]->setRate(1, IrStd::Type::Timestamp::now());
	}
}

const std::shared_ptr<Trader::PairTransaction> Trader::PairTransactionNop::get(CurrencyPtr currency) const noexcept
{
	const auto it = m_nopPairTransactionMap.find(currency);
	IRSTD_ASSERT(TraderTransaction, it != m_nopPairTransactionMap.end(), "This currency is not known: " << currency);
	return it->second;
}
