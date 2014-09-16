#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <obs-module.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif /* __cplusplus */

#ifdef __cplusplus
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class FFmpegCodecContext: public std::unique_ptr<AVCodecContext, void(*)(AVCodecContext *)> {
	public:
		FFmpegCodecContext(AVStream *stream, AVCodec *codec);
	private:
		std::vector<uint8_t> m_extradata;
};

class FFmpegSource {
	public:
		FFmpegSource(obs_data_t settings, obs_source_t source);
		~FFmpegSource();
		void update(obs_data_t settings);
		obs_source_t m_source;
	protected:
		void decodeLoop();
	private:
		bool isValid();
		AVStream *findVideoStream();
		std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> m_formatContext;
		std::unique_ptr<FFmpegCodecContext> m_codecContext;
		std::mutex m_mutex;
		std::condition_variable m_updateCondition;
		bool m_alive;
		std::thread m_thread;
};

class FFmpegPacket: public AVPacket {
	public:
		FFmpegPacket();

		// no copy semantics
		FFmpegPacket(const FFmpegPacket&) = delete;
		FFmpegPacket& operator=(const FFmpegPacket&) = delete;

		FFmpegPacket(FFmpegPacket&& other);
		~FFmpegPacket();

		int readFrame(AVFormatContext *ctx);
		void updateFrame(struct obs_source_frame *frame);
		AVPacket getOffsetPacket(int offset);
};

const AVRational kNSTimeBase = {
	.num = 1,
	.den = 1000000000,
};
#endif /* __cplusplus */

EXTERNC void *ffmpeg_source_create(obs_data_t settings, obs_source_t source);
EXTERNC void ffmpeg_source_destroy(void *data);
EXTERNC void ffmpeg_source_update(void *data, obs_data_t settings);
