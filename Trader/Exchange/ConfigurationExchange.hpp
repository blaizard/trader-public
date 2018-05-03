#pragma once

#include "Trader/Generic/Configuration/Configuration.hpp"

namespace Trader
{
	class ConfigurationExchange : public Configuration<ConfigurationExchange>
	{
	public:
		enum class RatesPolling : size_t
		{
			NONE = 0,
			UPDATE_RATES_IMPL,
			UPDATE_RATES_SPECIFIC_CURRENCY_IMPL,
			UPDATE_RATES_SPECIFIC_PAIR_IMPL
		};

		ConfigurationExchange()
				: Configuration<ConfigurationExchange>({
					/**
					 * The path of the output directory (where all output related to the
					 * exchange will be stored).
					 */
					{"outputDirectory", "."},
					/**
					 * When set, the balance read from the API is also including the amount
					 * of money already invested (tied to orders)
					 */
					{"balanceIncludeReserve", false},
					/**
					 * Set to true to enable rates recording.
					 */
					{"ratesRecording", true},
					/**
					 * Define the function used for rate polling
					 */
					{"ratesPolling", IrStd::Type::toIntegral(RatesPolling::UPDATE_RATES_IMPL)},
					/**
					 * The polling period in milliseconds of the new rates polling
					 */
					{"ratesPollingPeriodMs", /*Every second*/1000},
					/**
					 * The polling period in milliseconds of the order update
					 */
					{"orderPollingPeriodMs", /*Every 10 seconds*/10000},
					/**
					 * The polling period in milliseconds of the properties update
					 */
					{"propertiesPollingPeriodMs", /*Every 10 minutes*/10 * 60 * 1000},
					/**
					 * Maximum time it takes from an order created by the Trader and before it gets registered
					 * on the server side.
					 */
					{"orderRegisterTimeoutMs", /*Max 20 seconds*/20000},
					/**
					 * Enable order diversification. If activated, it will ensure that only a single order
					 * with the same transaction is activated at time. It will also ensure that at no all
					 * the funds are used for a single order.
					 */
					{"orderDiversification", true},
					/**
					 * If set, the exchange will not perform order related operations.
					 * and obviously also balance related.
					 */
					{"readOnly", false}
				})
		{
		}

		ConfigurationExchange(const IrStd::Type::Gson::Map& map)
				: ConfigurationExchange()
		{
			merge(map, /*mustExists*/true);
		}

		uint64_t getRatesPollingPeriodMs() const noexcept
		{
			return m_json.getNumber("ratesPollingPeriodMs");
		}

		uint64_t getOrderPollingPeriodMs() const noexcept
		{
			return m_json.getNumber("orderPollingPeriodMs");
		}

		uint64_t getPropertiesPollingPeriodMs() const noexcept
		{
			return m_json.getNumber("propertiesPollingPeriodMs");
		}

		uint64_t getOrderRegisterTimeoutMs() const noexcept
		{
			return m_json.getNumber("orderRegisterTimeoutMs");
		}

		bool isRatesPolling() const noexcept
		{
			return (getRatesPolling() != RatesPolling::NONE);
		}

		bool isRatesRecording() const noexcept
		{
			return m_json.getBool("ratesRecording");
		}

		void setBalanceIncludeReserve(const bool value) noexcept
		{
			m_json.getBool("balanceIncludeReserve").val(value);
		}

		bool isBalanceIncludeReserve() const noexcept
		{
			return m_json.getBool("balanceIncludeReserve");
		}

		bool isOrderDiversification() const noexcept
		{
			return m_json.getBool("orderDiversification");
		}

		/**
		 * Set read only mode
		 */
		void setReadOnly(const bool value) noexcept
		{
			m_json.getBool("readOnly").val(value);
		}

		bool isReadOnly() const noexcept
		{
			return m_json.getBool("readOnly");
		}

		RatesPolling getRatesPolling() const noexcept
		{
			const size_t ratesPollingValue = m_json.getNumber("ratesPolling");
			return static_cast<RatesPolling>(ratesPollingValue);
		}

		const char* getOutputDirectory() const noexcept
		{
			return m_json.getString("outputDirectory");
		}

		void setOutputDirectory(const std::string& outputDirectory) noexcept
		{
			m_json.getString("outputDirectory").val(m_json, outputDirectory);
		}
	};
}
