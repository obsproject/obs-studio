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

#include "gs-helper.h"

Helper::VertexBuffer::VertexBuffer(size_t maxVertices) {
	m_positions.resize(maxVertices);
	m_normals.resize(maxVertices);
	m_tangents.resize(maxVertices);
	m_colors.resize(maxVertices);
	m_uvs.resize(8);
	m_uvArrays.resize(8);
	for (size_t idx = 0; idx < 8; idx++) {
		m_uvs[idx].resize(maxVertices);
		m_uvArrays[idx].width = 2;
		m_uvArrays[idx].array = m_uvs[idx].data();
	}

	m_vertexData = gs_vbdata_create();
	m_vertexData->num = maxVertices;
	m_vertexData->points = m_positions.data();
	m_vertexData->normals = m_normals.data();
	m_vertexData->tangents = m_normals.data();
	m_vertexData->colors = m_colors.data();
	m_vertexData->num_tex = m_uvs.size();
	m_vertexData->tvarray = m_uvArrays.data();

	m_vertexBuffer = gs_vertexbuffer_create(m_vertexData, GS_DYNAMIC);
}

Helper::VertexBuffer::~VertexBuffer() {
	std::memset(m_vertexData, 0, sizeof(gs_vb_data));
	gs_vertexbuffer_destroy(m_vertexBuffer);
}

void Helper::VertexBuffer::set_uv_layers(size_t count) {
	size_t realcount = count > 8 ? 8 : count;
	m_uvs.resize(realcount);
}

size_t Helper::VertexBuffer::get_uv_layers() {
	return m_uvs.size();
}

gs_vertbuffer_t* Helper::VertexBuffer::update() {
	// Update Buffers
	size_t verts = this->size();
	m_positions.resize(verts);
	m_normals.resize(verts);
	m_tangents.resize(verts);
	m_colors.resize(verts);
	for (size_t layer = 0; layer < m_uvs.size(); layer++) {
		m_uvs[layer].resize(verts);
		m_uvArrays[layer].width = 2;
		m_uvArrays[layer].array = m_uvs[layer].data();
	}

	// Prepare Vertex Data
	for (size_t vert = 0; vert < verts; vert++) {
		Vertex& v = this->at(vert);

		vec3_copy(&(m_positions.at(vert)), &v.position);
		vec3_copy(&(m_normals.at(vert)), &v.normal);
		vec3_copy(&(m_tangents.at(vert)), &v.tangent);
		m_colors[vert] = v.color;
		for (size_t layer = 0; layer < m_uvs.size(); layer++) {
			vec2_copy(&m_uvs[layer][vert], &v.uv[layer]);
		}
	}

	// Upload to GPU
	m_vertexData->num = verts;
	m_vertexData->points = m_positions.data();
	m_vertexData->normals = m_normals.data();
	m_vertexData->tangents = m_normals.data();
	m_vertexData->colors = m_colors.data();
	m_vertexData->num_tex = m_uvArrays.size();
	m_vertexData->tvarray = m_uvArrays.data();
	gs_vertexbuffer_flush(m_vertexBuffer);
	return m_vertexBuffer;
}

Helper::Vertex::Vertex() {
	position.x =
		position.y =
		position.z =
		normal.x =
		normal.y =
		normal.z =
		tangent.x =
		tangent.y =
		tangent.z = 0;
	color = 0;
	for (size_t idx = 0; idx < 8; idx++) {
		uv[idx].x = uv[idx].y = 0;
	}
}
