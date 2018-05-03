#pragma once

#include "IrStd/IrStd.hpp"

namespace Trader
{
	template <class T>
	class Configuration
	{
	public:
		Configuration(const IrStd::Type::Gson::Map& map)
				: m_json(map)
		{
		}

		// For performance concerns
		Configuration(const Configuration<T>& configuration) = delete;
		Configuration(Configuration<T>&& configuration)
				: m_json(std::move(configuration.m_json))
		{
		}

		/**
		 * \param mustExists keys from map must be present in m_json
		 */
		void merge(const IrStd::Type::Gson::Map& map, const bool mustExists)
		{
			// Make sure keys of the map are correctly spelled
			IrStd::Json config(map);
			if (mustExists)
			{
				for (const auto& element : config.getObject())
				{
					IRSTD_ASSERT(m_json.is(element.first), "The configuration key '"
							<< element.first << "' is not valid");
				}
			}
			m_json.merge(config);
		}

		void toStream(std::ostream& os) const
		{
			m_json.toStream(os);
		}

		/**
		 * \brief Get the json object related with the configuration
		 */
		const IrStd::Json& getJson() const noexcept
		{
			return m_json;
		}

	protected:
		IrStd::Json m_json;
	};
}

template <class T>
std::ostream& operator<<(std::ostream& os, const Trader::Configuration<T>& configuration)
{
	configuration.toStream(os);
	return os;
}
