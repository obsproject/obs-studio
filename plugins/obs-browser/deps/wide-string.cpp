/******************************************************************************
 Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "wide-string.hpp"
#include <string.h>
#include <util/platform.h>

using namespace std;

wstring to_wide(const char *utf8)
{
	if (!utf8 || !*utf8)
		return wstring();

	size_t isize = strlen(utf8);
	size_t osize = os_utf8_to_wcs(utf8, isize, nullptr, 0);

	if (!osize)
		return wstring();

	wstring wide;
	wide.resize(osize);
	os_utf8_to_wcs(utf8, isize, &wide[0], osize + 1);
	return wide;
}

wstring to_wide(const std::string &utf8)
{
	if (utf8.empty())
		return wstring();

	size_t osize = os_utf8_to_wcs(utf8.c_str(), utf8.size(), nullptr, 0);

	if (!osize)
		return wstring();

	wstring wide;
	wide.resize(osize);
	os_utf8_to_wcs(utf8.c_str(), utf8.size(), &wide[0], osize + 1);
	return wide;
}
