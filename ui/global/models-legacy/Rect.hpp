/******************************************************************************
    Copyright (C) 2025 by Patrick Heyer <PatTheMav@users.noreply.github.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <algorithm>
#include <limits>

namespace {
template<typename numberType> inline double safeConvertToDouble(numberType number)
{
	constexpr double minDoubleValue = 0.0;
	constexpr double maxDoubleValue = std::numeric_limits<double>::max();

	if (number > maxDoubleValue) {
		return maxDoubleValue;
	} else if (number < minDoubleValue) {
		return minDoubleValue;
	} else {
		return static_cast<double>(number);
	}
}
} // namespace

namespace OBS {

struct Rect {
	double width_ = 0.0;
	double height_ = 0.0;

	Rect() = default;

	template<typename widthType, typename heightType> Rect(widthType width, heightType height)
	{
		setWidth(width);
		setHeight(height);
	}

	template<typename returnType> returnType getWidth() const
	{
		static_assert(std::is_convertible_v<double, returnType>,
			      "Cannot convert <double> to requested return type");

		return static_cast<returnType>(width_);
	};

	template<typename numberType> void setWidth(numberType width)
	{
		static_assert(std::is_same_v<numberType, double> || std::is_convertible_v<numberType, double>,
			      "Cannot convert provided argument to <double>");

		if (std::is_same<numberType, double>::value) {
			width_ = std::max(static_cast<double>(width), 0.0);
		} else {
			width_ = safeConvertToDouble(width);
		}
	}

	template<typename returnType> returnType getHeight() const
	{
		static_assert(std::is_convertible_v<double, returnType>,
			      "Cannot convert <double> to requested return type");

		return static_cast<returnType>(height_);
	};

	template<typename numberType> void setHeight(numberType height)
	{
		static_assert(std::is_same_v<numberType, double> || std::is_convertible_v<numberType, double>,
			      "Cannot convert provided argument to <double>");

		if (std::is_same<numberType, double>::value) {
			height_ = std::max(static_cast<double>(height), 0.0);
		} else {
			height_ = safeConvertToDouble(height);
		}
	}

	bool isZero() const;

	friend bool operator==(const Rect &lhs, const Rect &rhs);
	friend bool operator!=(const Rect &lhs, const Rect &rhs);
};
} // namespace OBS
