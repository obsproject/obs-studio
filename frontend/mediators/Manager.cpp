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

#include "Manager.hpp"

#include "obs.hpp"

namespace OBS::mediator {

Manager::Manager()
{
	// Connect to OBS signals to create/remove mediators for each managed object type
}

std::shared_ptr<OBS::mediator::Canvas> Manager::canvas(OBSCanvas canvas)
{
	return canvases.acquire(obs_canvas_get_uuid(canvas), canvas);
}

void Manager::pruneAll()
{
	canvases.prune();
	//scenes.prune();
	//sources.prune();
}

} // namespace OBS::mediator
