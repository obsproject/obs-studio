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
#include <mutex>
#include <vector>
#include <thread>

class FFmpegCodecContext: public std::unique_ptr<AVCodecContext, void(*)(AVCodecContext*)> {
	public:
		FFmpegCodecContext();
		FFmpegCodecContext(AVStream *stream, const AVCodec *codec);
	private:
		std::vector<uint8_t> m_extradata;
};

class FFmpegPacket: public AVPacket {
	public:
		FFmpegPacket();

		FFmpegPacket(const FFmpegPacket&) = delete;
		FFmpegPacket& operator=(const FFmpegPacket&) = delete;

		FFmpegPacket(FFmpegPacket&& other);
		~FFmpegPacket();

		void FreeData();
		AVPacket GetOffsetPacket(int offset);
};

class FFmpegSource {
	public:
		FFmpegSource(obs_source_t *source);
		~FFmpegSource();
		void Shutdown();
		void Update(obs_data_t *settings);
		bool IsValid();
		obs_source_t *m_source;
	protected:
		AVStream *FindVideoStream();
		void DecodeThread();
		std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> m_formatContext;
		FFmpegCodecContext m_codecContext;
	private:
		void OutputFrame(AVFrame *avFrame);
		void ProcessFrame(FFmpegPacket& pkt);
		int ReadFrame(FFmpegPacket *pkt);
		std::atomic<bool> m_alive;
		std::mutex m_mutex;
		std::thread m_thread;
};

const AVRational kNSTimeBase = {
	.num = 1,
	.den = 1000000000,
};
#endif /* __cplusplus */

EXTERNC void *ffmpeg_source_create(obs_data_t *settings, obs_source_t *source);
EXTERNC void ffmpeg_source_destroy(void *data);
EXTERNC void ffmpeg_source_update(void *data, obs_data_t *settings);
