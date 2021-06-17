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

#include "d3d11-subsystem.hpp"

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3d11_3.h>
#include <d3d11_4.h>

class gs_fence : public gs_obj {
	ComPtr<ID3D11DeviceContext4> context4;
	ComPtr<ID3D11Device5> device5;
	ComPtr<ID3D11Fence> fence;
	HANDLE hevent;

public:
	static bool available(gs_device *device);

public:
	gs_fence(gs_device *device);
	~gs_fence();

	void signal();

	void wait();

	bool timed_wait(uint32_t ms);

	void reset();
};
