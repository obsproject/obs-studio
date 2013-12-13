/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include <sstream>
#include "platform.hpp"

#include <util/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool GetDataFilePath(const char *data, string &output)
{
	stringstream str;
	str << "../../data/obs-studio/" << data;
	output = str.str();
	return os_file_exists(output.c_str());
}
