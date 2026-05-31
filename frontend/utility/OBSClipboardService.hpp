/******************************************************************************
    Copyright (C) 2026 by Kunal Dubey <xakep8@protonmail.com>

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

#include <obs.hpp>
#include <vector>
#include <string_view>

class OBSClipboardService {
public:
	bool canPasteSceneItems(bool duplicate) const;
	bool canPasteFilters() const;
	bool canPasteTransform() const;
	bool canPasteTransition() const;

	void copySceneItems(const std::vector<OBSSceneItem> &items);
	void copyFilters(OBSSource source);
	void copyTransform(OBSSceneItem item);
	void copyTransition(OBSSceneItem item, bool show);

	void pasteSceneItems(OBSScene scene, bool duplicate);
	void pasteFilters(OBSSource destination);
	void pasteTransform(const std::vector<OBSSceneItem> &items);
	void pasteTransition(const std::vector<OBSSceneItem> &items, bool show);

private:
	void setMimeData(const std::string_view mimeType, OBSData payload);
	OBSData getMimeData(const std::string_view mimeType);
};