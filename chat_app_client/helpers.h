#pragma once
#include <map>
#include <string>

namespace ChatApp
{
	template<typename T, typename U>
	inline U GetEnumValue(const T& enumValue, const std::map<T, U>& map, const U& defaultValue)
	{
		auto it = map.find(enumValue);
		if (it != map.end()) {
			return it->second;
		}

		return defaultValue;
	}
}