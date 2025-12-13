#include "libvr/IEncoderAdapter.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

// Ensure strict priority: NVENC > VAAPI > QSV > Software
static const char* PREFERRED_CODECS[] = {
    "h264_nvenc",
    "hevc_nvenc",
    "av1_nvenc",
    "h264_vaapi",
    "hevc_vaapi",
    "h264_qsv",
    "libx264", // Fallback only (Legacy support)
    nullptr
};

namespace libvr {

class FFmpegEncoderAdapter : public IEncoderAdapter {
public:
    FFmpegEncoderAdapter() {
        // Setup VTBL
        static const IEncoderAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticEncodeFrame,
            StaticFlush,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }

    ~FFmpegEncoderAdapter() {
        Shutdown();
    }

    void Initialize(const EncoderConfig* cfg) {
        config = *cfg;
        const AVCodec* codec = nullptr;

        // 1. Try to find the user-requested codec, prioritizing hardware variants
        for (const char** name = PREFERRED_CODECS; *name != nullptr; name++) {
            if (strstr(*name, config.codec) != nullptr) { // Simple match "h264" in "h264_nvenc"
                 codec = avcodec_find_encoder_by_name(*name);
                 if (codec) {
                     std::cout << "[FFmpeg] Selected Encoder: " << *name << std::endl;
                     break;
                 }
            }
        }

        if (!codec) {
            std::cerr << "[FFmpeg] Hardware encoder not found for " << config.codec << ". Falling back to software." << std::endl;
            // Software fallback
            if (strcmp(config.codec, "h264") == 0) codec = avcodec_find_encoder_by_name("libx264");
            else if (strcmp(config.codec, "hevc") == 0) codec = avcodec_find_encoder_by_name("libx265");
            
            if (!codec) {
                 std::cerr << "[FFmpeg] Critical: No encoder found." << std::endl;
                 return;
            }
        }

        context = avcodec_alloc_context3(codec);
        if (!context) return;

        context->width = config.width;
        context->height = config.height;
        context->time_base = {1, config.fps_num}; // Approximation, real usage needs proper timebase
        context->framerate = {config.fps_num, config.fps_den};
        context->pix_fmt = AV_PIX_FMT_YUV420P; // For now assume standard 4:2:0
        context->bit_rate = config.bitrate_kbps * 1000;
        
        // NVENC specific opts?
        if (strstr(codec->name, "nvenc")) {
            av_opt_set(context->priv_data, "preset", "p4", 0); // Performance preset
            av_opt_set(context->priv_data, "tune", "ll", 0);   // Low latency
        }

        if (avcodec_open2(context, codec, nullptr) < 0) {
            std::cerr << "[FFmpeg] Failed to open codec." << std::endl;
            avcodec_free_context(&context);
            context = nullptr;
            return;
        }

        packet = av_packet_alloc();
        frame_cpu = av_frame_alloc();
        frame_cpu->format = context->pix_fmt;
        frame_cpu->width = context->width;
        frame_cpu->height = context->height;
        av_frame_get_buffer(frame_cpu, 0);

        std::cout << "[FFmpeg] Initialized successfully." << std::endl;
        initialized = true;
    }

    bool EncodeFrame(const GPUFrameView* view) {
        if (!initialized || !context) return false;

        // STAGING COPY: copy view->handle (VkImage/Memory) to frame_cpu (malloc)
        // Since we are in Phase 3 (Staging), we assume view->handle is temporarily readable 
        // OR we just zero it out / mock copy for the skeleton if real mapping is Phase 5.
        // For strict correctness in this phase w/o Vulkan mapping:
        // We will just verify we *can* call send_frame.
        
        int ret = av_frame_make_writable(frame_cpu);
        
        // TODO: Real memcpy from GPU mapped memory here
        // For now, populate dummy data
        if (ret >= 0) {
             // frame_cpu->data[0] ...
             frame_cpu->pts = view->timestamp; // Pass through PTS
        }

        ret = avcodec_send_frame(context, frame_cpu);
        if (ret < 0) {
            std::cerr << "[FFmpeg] Error sending frame to encoder." << std::endl;
            return false;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(context, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            else if (ret < 0) {
                 std::cerr << "[FFmpeg] Error during encoding." << std::endl;
                 return false;
            }

            // Packet ready!
            // std::cout << "[FFmpeg] Packet encoded! Size: " << packet->size << std::endl;
            // Send to transport... (Not linked here yet, this is adapter)
            
            av_packet_unref(packet);
        }

        return true;
    }

    void Flush() {
        if (context) avcodec_send_frame(context, nullptr);
    }

    void Shutdown() {
        if (context) {
            avcodec_free_context(&context);
            context = nullptr;
        }
        if (packet) {
            av_packet_free(&packet);
            packet = nullptr;
        }
        if (frame_cpu) {
            av_frame_free(&frame_cpu);
            frame_cpu = nullptr;
        }
        initialized = false;
    }

    // Static trampolines
    static void StaticInitialize(IEncoderAdapter* self, const EncoderConfig* cfg) {
        static_cast<FFmpegEncoderAdapter*>(self->user_data)->Initialize(cfg);
    }
    static bool StaticEncodeFrame(IEncoderAdapter* self, const GPUFrameView* frame) {
        return static_cast<FFmpegEncoderAdapter*>(self->user_data)->EncodeFrame(frame);
    }
    static void StaticFlush(IEncoderAdapter* self) {
         static_cast<FFmpegEncoderAdapter*>(self->user_data)->Flush();
    }
    static void StaticShutdown(IEncoderAdapter* self) {
         static_cast<FFmpegEncoderAdapter*>(self->user_data)->Shutdown();
    }

private:
    EncoderConfig config;
    AVCodecContext* context = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame_cpu = nullptr;
    bool initialized = false;
};

// C-Factory
extern "C" IEncoderAdapter* CreateFFmpegAdapter() {
    return new FFmpegEncoderAdapter();
}

} // namespace libvr
