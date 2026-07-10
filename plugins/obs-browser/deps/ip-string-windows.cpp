/******************************************************************************
 Copyright (C) 2026 by Warchamp7 <warchamp7@obsproject.com>

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

#include "ip-string.hpp"

#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

bool checkForIpv4String(const std::string &path)
{
	sockaddr_in sa4;

	return inet_pton(AF_INET, path.c_str(), &sa4.sin_addr) == 1;
}

bool checkForIpv6String(const std::string &path)
{
	sockaddr_in6 sa6;

	return inet_pton(AF_INET6, path.c_str(), &sa6.sin6_addr) == 1;
}
