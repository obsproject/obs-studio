#pragma once

#include <string>
#include <cstdint>

namespace NeuralStudio {

    enum class StereoFormat {
        MONO,
        SIDE_BY_SIDE,      // Left|Right horizontal
        TOP_BOTTOM,        // Left top, Right bottom
        SEPARATE_STREAMS,  // Two independent streams
        ANAGLYPH           // Red/Cyan (legacy)
    };

    struct VRHeadsetProfile {
        // Identity
        std::string name;  // "Meta Quest 3"
        std::string id;    // "quest3"

        // Display Properties
        uint32_t eyeWidth;    // Per-eye resolution width
        uint32_t eyeHeight;   // Per-eye resolution height
        float fovHorizontal;  // Horizontal FOV (degrees)
        float fovVertical;    // Vertical FOV (degrees)
        float ipd;            // Interpupillary distance (mm)

        // Encoding
        std::string codec;          // "h265", "av1", "vp9"
        uint32_t bitrate;           // kbps
        uint32_t framerate;         // fps (90, 120, etc.)
        StereoFormat stereoFormat;  // SBS, TB, SEPARATE_STREAMS

        // Spatial Metadata
        bool embedDepthMap;         // Include depth channel
        bool embedSpatialAudio;     // Ambisonics metadata
        std::string spatialFormat;  // "equirectangular", "cubemap", "mesh"

        // Streaming
        uint16_t srtPort;  // Unique SRT port for this profile
        bool enabled;      // Whether to stream this profile
    };

    // Predefined Profiles
    namespace Profiles {
        inline VRHeadsetProfile Quest3()
        {
            return VRHeadsetProfile {.name = "Meta Quest 3",
                                     .id = "quest3",
                                     .eyeWidth = 1920,
                                     .eyeHeight = 1920,
                                     .fovHorizontal = 110.0f,
                                     .fovVertical = 96.0f,
                                     .ipd = 63.0f,
                                     .codec = "h265",
                                     .bitrate = 100000,
                                     .framerate = 90,
                                     .stereoFormat = StereoFormat::SIDE_BY_SIDE,
                                     .embedDepthMap = true,
                                     .embedSpatialAudio = true,
                                     .spatialFormat = "equirectangular",
                                     .srtPort = 9001,
                                     .enabled = false};
        }

        inline VRHeadsetProfile ValveIndex()
        {
            return VRHeadsetProfile {.name = "Valve Index",
                                     .id = "index",
                                     .eyeWidth = 1440,
                                     .eyeHeight = 1600,
                                     .fovHorizontal = 130.0f,
                                     .fovVertical = 105.0f,
                                     .ipd = 63.0f,
                                     .codec = "h265",
                                     .bitrate = 150000,
                                     .framerate = 120,
                                     .stereoFormat = StereoFormat::SIDE_BY_SIDE,
                                     .embedDepthMap = true,
                                     .embedSpatialAudio = true,
                                     .spatialFormat = "equirectangular",
                                     .srtPort = 9002,
                                     .enabled = false};
        }

        inline VRHeadsetProfile VivePro2()
        {
            return VRHeadsetProfile {.name = "HTC Vive Pro 2",
                                     .id = "vive_pro_2",
                                     .eyeWidth = 2448,
                                     .eyeHeight = 2448,
                                     .fovHorizontal = 120.0f,
                                     .fovVertical = 120.0f,
                                     .ipd = 63.0f,
                                     .codec = "av1",
                                     .bitrate = 200000,
                                     .framerate = 90,
                                     .stereoFormat = StereoFormat::SIDE_BY_SIDE,
                                     .embedDepthMap = true,
                                     .embedSpatialAudio = true,
                                     .spatialFormat = "equirectangular",
                                     .srtPort = 9003,
                                     .enabled = false};
        }
    }  // namespace Profiles

}  // namespace NeuralStudio
