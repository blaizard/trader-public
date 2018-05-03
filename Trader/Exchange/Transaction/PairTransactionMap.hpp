#pragma once

#include <memory>

#include "IrStd/IrStd.hpp"
#include "Trader/Exchange/Transaction/PairTransaction.hpp"

IRSTD_TOPIC_USE(Trader, PairTransactionMap);

namespace Trader
{
	class PairTransactionMap
	{
	public:
		typedef std::shared_ptr<Trader::PairTransaction> PairTransactionPointer;

		/**
		 * Create a transaction pair between 2 currencies
		 */
		template<class T>
		PairTransactionPointer registerPair(const T& pairImpl)
		{
			IRSTD_ASSERT_CHILDOF(T, PairTransactionImpl);

			const CurrencyPtr from = pairImpl.getInitialCurrency();
			const CurrencyPtr to = pairImpl.getFinalCurrency();
			IRSTD_ASSERT(from != to, "from=" << from << ", to=" << to);

			{
				auto scope = m_lock.writeScope();

				// Make sure the entry does not exists yet
				auto it1 = m_map.find(from);
				if (it1 != m_map.end())
				{
					auto it2 = m_map[from].find(to);
					IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, PairTransactionMap),
							it2 == m_map[from].end(), "The pair " << from << "/"
							<< to << " already exists");
				}
				m_map[from][to] = std::make_shared<T>(pairImpl);
			}

			IRSTD_LOG_DEBUG(IRSTD_TOPIC(Trader, PairTransactionMap), "Define pair " << pairImpl);

			return m_map[from][to];
		}

		/**
		 * Create an inverted transaction pair between 2 currencies
		 */
		template<class T, class U>
		PairTransactionPointer registerInvertPair(const U& pairImpl)
		{
			const CurrencyPtr to = pairImpl.getInitialCurrency();
			const CurrencyPtr from = pairImpl.getFinalCurrency();
			IRSTD_ASSERT(from != to);

			IRSTD_ASSERT_CHILDOF(T, InvertPairTransactionImpl);
			
			auto scope = m_lock.writeScope();

			// Get and make sure the inverted entry does exists
			const auto it1 = m_map.find(to);
			IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, PairTransactionMap), it1 != m_map.end(),
					"There is no transaction for " << to);
			const auto it2 = it1->second.find(from);
			IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, PairTransactionMap), it2 != it1->second.end(),
					"There is no transaction for " << to << "/" << from);
			auto pMirrorTransaction = it2->second;
			IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, PairTransactionMap), !pMirrorTransaction->isInvertedTransaction(),
					"This transaction has already been mirrored");

			{
				// Make sure the entry does not exists yet
				auto it1 = m_map.find(from);
				if (it1 != m_map.end())
				{
					auto it2 = m_map[from].find(to);
					IRSTD_THROW_ASSERT(IRSTD_TOPIC(Trader, PairTransactionMap), it2 == m_map[from].end(),
							"The pair " << from << "/" << to << " already exists");
				}
				// Create the invertede transaction
				m_map[from][to] = std::make_shared<T>(pMirrorTransaction);
				// Link this transaction with the original transaction
				static_cast<PairTransactionImpl*>(pMirrorTransaction.get())->m_pInvertedTransaction = m_map[from][to];
			}

			IRSTD_LOG_DEBUG(IRSTD_TOPIC(Trader, PairTransactionMap), "Define inverse pair " << *m_map[from][to]);

			return m_map[from][to];
		}

		/**
		 * \brief Get the list of transaction pairs for a given currency.
		 *
		 * \param from The currency to list the transactions from.
		 *
		 * \return A map containing the transactions.
		 */
		void getTransactions(const CurrencyPtr currency, const std::function<void(const CurrencyPtr,
				const PairTransactionPointer)>& callback) const;
		void getTransactions(const CurrencyPtr currency, const std::function<void(const CurrencyPtr)>& callback) const;

		/**
		 * \brief Get the list of transaction pairs.
		 *
		 * \param callback The callback to be called to preocess each transactions.
		 *
		 * \note These functions cannot be noexcept, as the callback might throw an exception
		 */
		void getTransactions(const std::function<void(const CurrencyPtr, const CurrencyPtr,
				const PairTransactionPointer)>& callback) const;
		void getTransactions(const std::function<void(const CurrencyPtr, const CurrencyPtr)>& callback) const;

		/**
		 * \brief Get a specific transaction pair.
		 *
		 * \param from The intial currency.
		 * \param to The final currency.
		 *
		 * \return The transaction or a nullptr if none available.
		 */
		const PairTransactionPointer getTransaction(const CurrencyPtr from, const CurrencyPtr to) const noexcept;
		PairTransactionPointer getTransactionForWrite(const CurrencyPtr from, const CurrencyPtr to) noexcept;

		/**
		 * Equality operator
		 */
		bool operator==(const PairTransactionMap& map) const noexcept;
		bool operator!=(const PairTransactionMap& map) const noexcept;

		/**
		 * Assignment operator
		 */
		PairTransactionMap& operator=(const PairTransactionMap& map);

		/**
		 * Clear the map
		 */
		void clear() noexcept;
	private:
		mutable IrStd::RWLock m_lock;
		std::map<CurrencyPtr, std::map<CurrencyPtr, PairTransactionPointer>> m_map;
	};
}
