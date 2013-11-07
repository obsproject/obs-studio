/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "util/windows/ComPtr.hpp"

class DShowSource {
	ComPtr<IGraphBuilder>         graph;
	ComPtr<ICaptureGraphBuilder2> capture;
	ComPtr<IMediaControl>         control;

	ComPtr<IBaseFilter>           videoFilter;
	ComPtr<IBaseFilter>           audioFilter;

	ComPtr<CaptureFilter>         captureFilter;

	
};
