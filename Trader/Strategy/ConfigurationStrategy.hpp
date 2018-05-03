#pragma once

#include "Trader/Generic/Configuration/Configuration.hpp"

namespace Trader
{
	class ConfigurationStrategy : public Configuration<ConfigurationStrategy>
	{
	public:
		enum class Trigger
		{
			ON_RATE_CHANGE,
			EVERY_SECOND,
			EVERY_MINUTE,
			EVERY_HOUR,
			EVERY_DAY
		};

		ConfigurationStrategy()
				: Configuration<ConfigurationStrategy>({
					/**
					 * Define the event that will trigger the strategy
					 */
					{"trigger", IrStd::Type::toIntegral(Trigger::ON_RATE_CHANGE)},
					/**
					 * List of exchanges to which the strategy is attached
					 */
					{"exchangeList", {}}
				})
		{
		}

		ConfigurationStrategy(const IrStd::Type::Gson::Map& strategySpecificConfig,
			const IrStd::Type::Gson::Map& instanceSpecificConfig = {})
				: ConfigurationStrategy()
		{
			merge(strategySpecificConfig, /*mustExists*/false);
			merge(instanceSpecificConfig, /*mustExists*/true);
		}

		Trigger getTrigger() const noexcept
		{
			const size_t triggerValue = m_json.getNumber("trigger");
			return static_cast<Trigger>(triggerValue);
		}

		void eachExchangeId(const std::function<void(const Id&)>& callback) const noexcept
		{
			const auto& exchangeList = m_json.getArray("exchangeList");
			for (size_t i=0; i<exchangeList.size(); ++i)
			{
				const Id id{exchangeList.getString(i).val()};
				callback(id);
			}
		}
	};
}
