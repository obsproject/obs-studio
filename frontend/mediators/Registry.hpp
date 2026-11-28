/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <memory>
#include <string>
#include <unordered_map>

namespace OBS::mediator {

// This class holds a list of mediators for a given libobs type keyed by UUID.
// If a mediator already exists for a given UUID, a shared_ptr to it will be returned.
// If not, a new one will be created.
template<class MediatorType> class Registry {
	std::unordered_map<std::string, std::shared_ptr<MediatorType>> map;

public:
	template<typename OBSType> std::shared_ptr<MediatorType> acquire(const std::string &uuid, OBSType object)
	{
		std::shared_ptr<MediatorType> &entry = map[uuid];
		if (entry && !entry->isRemoved()) {
			return entry;
		}

		auto mediator = std::make_shared<MediatorType>(std::forward<OBSType>(object));

		entry = mediator;

		return mediator;
	}

	// Remove any mediators for objects that have been marked as removed.
	void prune()
	{
		for (auto it = map.begin(); it != map.end();) {
			if (!it->second) {
				it = map.erase(it);
			} else {
				++it;
			}
		}
	}
};

} // namespace OBS::mediator
