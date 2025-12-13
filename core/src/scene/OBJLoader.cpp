#include "libvr/OBJLoader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <iostream>

namespace libvr {

struct VertexHash {
	size_t operator()(const std::string &key) const { return std::hash<std::string>{}(key); }
};

Mesh OBJLoader::Load(const std::string &path)
{
	Mesh mesh;
	mesh.id = 0;
	mesh.name = path; // Default name

	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "[OBJLoader] Failed to open file: " << path << std::endl;
		return mesh;
	}

	std::vector<float> temp_positions;
	std::vector<float> temp_uvs;
	std::vector<float> temp_normals;

	// Use string key "v/vt/vn" to prevent duplicates
	std::unordered_map<std::string, uint32_t> vertex_map;
	std::vector<Vertex> unique_vertices;
	std::vector<uint32_t> indices;

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#')
			continue;

		std::stringstream ss(line);
		std::string type;
		ss >> type;

		if (type == "v") {
			float x, y, z;
			ss >> x >> y >> z;
			temp_positions.push_back(x);
			temp_positions.push_back(y);
			temp_positions.push_back(z);
		} else if (type == "vt") {
			float u, v;
			ss >> u >> v;
			temp_uvs.push_back(u);
			temp_uvs.push_back(v);
		} else if (type == "vn") {
			float x, y, z;
			ss >> x >> y >> z;
			temp_normals.push_back(x);
			temp_normals.push_back(y);
			temp_normals.push_back(z);
		} else if (type == "f") {
			// Support "v/vt/vn" or "v//vn" or "v"
			// We assume triangulated or quad faces
			std::string vertex_str;
			std::vector<std::string> face_indices;
			while (ss >> vertex_str) {
				face_indices.push_back(vertex_str);
			}

			// Triangulate (0, 1, 2) then (0, 2, 3) etc.
			for (size_t i = 1; i + 1 < face_indices.size(); ++i) {
				std::string points[3] = {face_indices[0], face_indices[i], face_indices[i + 1]};

				for (int k = 0; k < 3; ++k) {
					std::string &key = points[k];

					if (vertex_map.count(key) == 0) {
						// Create new vertex
						Vertex v;
						// Defaults
						v.position[0] = 0;
						v.position[1] = 0;
						v.position[2] = 0;
						v.uv[0] = 0;
						v.uv[1] = 0;
						v.normal[0] = 0;
						v.normal[1] = 0;
						v.normal[2] = 0;

						// Parse "v_idx/vt_idx/vn_idx"
						size_t slash1 = key.find('/');
						size_t slash2 = (slash1 != std::string::npos)
									? key.find('/', slash1 + 1)
									: std::string::npos;

						int v_idx = 0, vt_idx = 0, vn_idx = 0;

						// Parse Position
						if (slash1 == std::string::npos) {
							v_idx = std::stoi(key);
						} else {
							v_idx = std::stoi(key.substr(0, slash1));

							// Parse UV
							if (slash2 != std::string::npos) {
								// v/vt/vn or v//vn
								std::string vt_str =
									key.substr(slash1 + 1, slash2 - slash1 - 1);
								if (!vt_str.empty())
									vt_idx = std::stoi(vt_str);

								std::string vn_str = key.substr(slash2 + 1);
								if (!vn_str.empty())
									vn_idx = std::stoi(vn_str);
							} else {
								// v/vt
								std::string vt_str = key.substr(slash1 + 1);
								if (!vt_str.empty())
									vt_idx = std::stoi(vt_str);
							}
						}

						// OBJ is 1-based, C++ 0-based
						if (v_idx > 0 && (v_idx - 1) * 3 + 2 < temp_positions.size()) {
							v.position[0] = temp_positions[(v_idx - 1) * 3 + 0];
							v.position[1] = temp_positions[(v_idx - 1) * 3 + 1];
							v.position[2] = temp_positions[(v_idx - 1) * 3 + 2];
						}

						if (vt_idx > 0 && (vt_idx - 1) * 2 + 1 < temp_uvs.size()) {
							v.uv[0] = temp_uvs[(vt_idx - 1) * 2 + 0];
							v.uv[1] = 1.0f - temp_uvs[(vt_idx - 1) * 2 + 1]; // Flip V?
						}

						if (vn_idx > 0 && (vn_idx - 1) * 3 + 2 < temp_normals.size()) {
							v.normal[0] = temp_normals[(vn_idx - 1) * 3 + 0];
							v.normal[1] = temp_normals[(vn_idx - 1) * 3 + 1];
							v.normal[2] = temp_normals[(vn_idx - 1) * 3 + 2];
						}

						unique_vertices.push_back(v);
						vertex_map[key] = (uint32_t)(unique_vertices.size() - 1);
					}
					indices.push_back(vertex_map[key]);
				}
			}
		}
	}

	mesh.vertices = unique_vertices;
	mesh.indices = indices;

	std::cout << "[OBJLoader] Loaded " << mesh.vertices.size() << " vertices, " << mesh.indices.size()
		  << " indices from " << path << std::endl;
	return mesh;
}

} // namespace libvr
