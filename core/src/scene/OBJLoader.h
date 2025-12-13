#pragma once

#include "scene/SceneManager.h"
#include <string>

namespace libvr {

    class OBJLoader
    {
          public:
        // Load an .obj file from disk.
        // Returns a Mesh with vertices and indices populated.
        // Returns an empty mesh (id=0) on failure.
        static Mesh Load(const std::string &path);
    };

}  // namespace libvr
