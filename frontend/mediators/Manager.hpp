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

#include <mediators/Canvas.hpp>
#include <mediators/Registry.hpp>

namespace OBS::mediator {

// This class owns and manages the lifetime of all mediators for libobs objects, and provides a way to acquire
// a mediator for a given object type via libobs pointer.
class Manager {
	OBS::mediator::Registry<OBS::mediator::Canvas> canvases{};
	// OBS::mediator::Registry<OBS::mediator::Scene> scenes{};
	// OBS::mediator::Registry<OBS::mediator::Source> sources{};

	std::vector<OBSSignal> signalHandlers;

public:
	Manager();

	std::shared_ptr<OBS::mediator::Canvas> canvas(OBSCanvas canvas);
	//std::shared_ptr<OBS::mediator::Scene> scene(OBSScene canvas);
	//std::shared_ptr<OBS::mediator::Source> source(OBSSource canvas);

	void pruneAll();
};
} // namespace OBS::mediator
