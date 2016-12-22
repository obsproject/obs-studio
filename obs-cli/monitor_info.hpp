/******************************************************************************
    Copyright (C) 2016 by João Portela <email@joaoportela.net>

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

/* Functions to access monitor information. */

#pragma once

#include<vector>

struct MonitorInfo {
	// same value as its index in all_monitors list.
	size_t monitor_id;
	long height, width;
	long top, left;

	static const MonitorInfo NotFound;
};

/**
* Detects the resolution and location of all monitors connected.
*/
bool detect_monitors();

/**
* Gets monitor at index
*
* Ensure that you call detect_monitors() before calling this method.
*/
MonitorInfo monitor_at_index(int m);

/**
* Prints all monitor properties
*
* Ensure that you call detect_monitors() before calling this method.
*/
void print_monitors_info();
