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

#include "Rect.hpp"

namespace OBS {
bool Rect::isZero() const
{
	return width_ == 0.0 || height_ == 0.0;
}

bool operator==(const Rect &lhs, const Rect &rhs)
{
	return lhs.width_ == rhs.width_ && lhs.height_ == rhs.height_;
}

bool operator!=(const Rect &lhs, const Rect &rhs)
{
	return lhs.width_ != rhs.width_ || lhs.height_ != rhs.height_;
}
} // namespace OBS
