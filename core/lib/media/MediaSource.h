#pragma once

#include "common/frame.h"
#include <string>
#include <memory>
#include <vector>

// Forward decl for shared media type
struct mp_media;
typedef struct mp_media mp_media_t;

namespace libvr {

    class MediaSource
    {
          public:
        MediaSource();
        ~MediaSource();

        bool Load(const std::string &path);
        void Unload();

        // Updates playback state (decodes frames)
        // Should be called every frame
        void Update();

        // Returns the current video frame if available
        // Note: Returns a CPU buffer view (obs_source_frame) usually.
        // For MVP we might just log or return a blank view if we don't have texture upload yet.
        bool GetFrame(GPUFrameView &outFrame);

          private:
        mp_media_t *media = nullptr;
        std::string currentPath;

        // Internal buffer for RGBA conversion (simple CPU conversion for mvp)
        std::vector<uint8_t> rgbBuffer;
        int width = 0;
        int height = 0;
    };

}  // namespace libvr
