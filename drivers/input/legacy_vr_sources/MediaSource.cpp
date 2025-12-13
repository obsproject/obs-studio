#include "libvr/MediaSource.h"
#include <iostream>
#include <cstring> 

extern "C" {
#include "media-playback.h"
#include "media.h" 
}

namespace libvr {

// callbacks
static void v_cb(void *opaque, struct obs_source_frame *frame) {
    // Callback when a frame is ready
}
static void v_preload_cb(void *opaque, struct obs_source_frame *frame) {}
static void v_seek_cb(void *opaque, struct obs_source_frame *frame) {}
static void a_cb(void *opaque, struct obs_source_audio *audio) {}
static void stop_cb(void *opaque) {}

MediaSource::MediaSource() {
    media = (mp_media_t*)calloc(1, sizeof(mp_media_t));
}

MediaSource::~MediaSource() {
    Unload();
    free(media);
}

bool MediaSource::Load(const std::string& path) {
    Unload();

    struct mp_media_info info = {}; // Zero init
    info.opaque = this;
    info.path = path.c_str(); // const char*
    info.format = nullptr;
    info.buffering = 0;
    info.speed = 100;
    info.force_range = VIDEO_RANGE_DEFAULT;
    info.hardware_decoding = false;
    info.is_local_file = true;
    
    info.v_cb = v_cb;
    info.v_preload_cb = v_preload_cb;
    info.v_seek_cb = v_seek_cb;
    info.a_cb = a_cb;
    info.stop_cb = stop_cb;

    if (!mp_media_init(media, &info)) {
        std::cerr << "[MediaSource] Failed to init mp_media for: " << path << std::endl;
        return false;
    }

    mp_media_play(media, true, false); // Loop=true
    std::cout << "[MediaSource] Loaded: " << path << std::endl;
    currentPath = path;
    return true;
}

void MediaSource::Unload() {
    if (media && currentPath.length() > 0) {
        mp_media_free(media);
        memset(media, 0, sizeof(mp_media_t)); // reset
        currentPath = "";
    }
}

void MediaSource::Update() {
   // media-playback has its own thread usually. 
   // We might need to check EOF or similar here.
}

bool MediaSource::GetFrame(GPUFrameView& outFrame) {
    if (!media || currentPath.empty()) return false;
    
    struct obs_source_frame* f = &media->obsframe;
    if (!f->data[0]) return false;

    // Populate simplified view
    outFrame.width = f->width;
    outFrame.height = f->height;
    // outFrame.stride = f->linesize[0]; // Not always linear, but good enough guess for planar Y
    // outFrame.format/textureId do not exist in GPUFrameView.
    // handle is void*.
    outFrame.handle = f->data[0]; // Point handle to CPU buffer for now (unsafe but verifies flow)
    
    return true;
}

} // namespace libvr
