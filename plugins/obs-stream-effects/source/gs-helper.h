/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once
#include "plugin.h"

extern "C" {
#pragma warning (push)
#pragma warning (disable: 4201)
#include "libobs/graphics/graphics.h"
#pragma warning (pop)
}

#include <vector>

namespace Helper {
	struct Vertex {
		Vertex();

		vec3
			position,
			normal,
			tangent;
		vec2 uv[8];
		uint32_t
			color;
	};

	class VertexBuffer : public std::vector<Vertex> {
		public:
		VertexBuffer(size_t maxVertices);
		virtual ~VertexBuffer();

		void set_uv_layers(size_t count);
		size_t get_uv_layers();

		gs_vertbuffer_t* update();

		private:
		gs_vb_data *m_vertexData;
		gs_vertbuffer_t *m_vertexBuffer;
		std::vector<gs_tvertarray> m_uvArrays;
		std::vector<vec3> m_positions, m_normals, m_tangents;
		std::vector<uint32_t> m_colors;
		std::vector<std::vector<vec2>> m_uvs;
	};
}
