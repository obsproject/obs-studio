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

/* Class to manage main event loop. */

#pragma once

#include <mutex>
#include <condition_variable>

class EventLoop {
public:
	EventLoop();

	void run();
	void stop();
private:
	std::condition_variable stop_condition;
	std::mutex stop_mutex;
};
