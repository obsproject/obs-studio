#include <mutex>

#include "obs-ffmpeg-source.h"
#include "obs-ffmpeg-formats.h"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstring>

#include <util/platform.h>

FFmpegSource::FFmpegSource(obs_data_t settings, obs_source_t source):
	m_source(source),
	m_formatContext(avformat_alloc_context(), &avformat_free_context),
	m_alive(true),
	m_thread(&FFmpegSource::decodeLoop, this)
{
	update(settings);
}

FFmpegSource::~FFmpegSource()
{
	{
		std::unique_lock<std::mutex> lck(m_mutex);
		m_alive = false;
	}
	m_updateCondition.notify_all();
	m_thread.join();
}

void FFmpegSource::update(obs_data_t settings)
{
	{
		std::unique_lock<std::mutex> lck(m_mutex);

		// Reset the source to a newly-initialized state
		// Create a new format context
		m_formatContext.reset(avformat_alloc_context());
		// Delete any existing AVCodecContexts
		m_codecContext.reset(nullptr);

		AVInputFormat *iFormat = nullptr;
		if (strlen(obs_data_get_string(settings, "format")) > 0) {
			iFormat = av_find_input_format(obs_data_get_string(settings, "format"));
			if (iFormat == nullptr) {
				std::cerr << "Failed to find desired input format '"  << obs_data_get_string(settings, "format") << "'" << std::endl;
			}
		}
		// Ownership of the format context is temporarily transferred
		// to avformat_open_input because it is freed in the case
		// of failure
		AVFormatContext *fmt = nullptr;
		if (avformat_open_input(&fmt,
					obs_data_get_string(settings, "filename"),
					iFormat,
					nullptr) != 0) {
			std::cerr << "Couldn't open ffmpeg input" << std::endl;
			return;
		} else {
			m_formatContext = std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)>(fmt, [](AVFormatContext *c) { avformat_close_input(&c); });
		}

		// Find stream info
		if (avformat_find_stream_info(this->m_formatContext.get(), nullptr) != 0) {
			std::cerr << "Couldn't find ffmpeg stream info" << std::endl;
			return;
		}

		auto videoStream = findVideoStream();
		if (videoStream == nullptr) {
			std::cerr << "FFmpeg input didn't have video stream" << std::endl;
			return;
		}

		const auto videoCodec = avcodec_find_decoder(videoStream->codec->codec_id);
		if(videoCodec == nullptr) {
			std::cerr << "Couldn't find decoder for FFmpeg video stream" << std::endl;
			return;
		}

		m_codecContext.reset(new FFmpegCodecContext(videoStream, videoCodec));
		if (*m_codecContext == nullptr) {
			std::cerr << "Couldn't create FFmpeg codec context" << std::endl;
			return;
		}

		av_dump_format(m_formatContext.get(), 0, m_formatContext->filename, 0);
	}
	m_updateCondition.notify_all();
}

bool FFmpegSource::isValid()
{
	return (m_codecContext != nullptr);
}

void FFmpegSource::decodeLoop()
{
	struct obs_source_frame frame;

	FFmpegPacket pkt;
	// For some reason we have to local cache these.
	// It probably has something to do with how detached threads work
	FFmpegSource *ctx = this;

	while (ctx->m_alive) {
		std::unique_lock<std::mutex> lck(ctx->m_mutex);

		if (!ctx->isValid()) {
			ctx->m_updateCondition.wait(lck);
			continue;
		}

		auto videoStream = ctx->findVideoStream();
		if (pkt.readFrame(ctx->m_formatContext.get()) < 0) {
			// If we've finished this input, then wait for an update
			ctx->m_updateCondition.wait(lck);
			continue;
		}
		// If it's not a video stream packet, skip it
		if (pkt.stream_index != videoStream->index) {
			continue;
		}

		int offset = 0;
		std::unique_ptr<AVFrame, void(*)(void *)> avFrame(av_frame_alloc(), &av_free);
		while (offset < pkt.size) {
			AVPacket decodePacket = pkt.getOffsetPacket(offset);
			int framesAvailable = 0;
			const auto ret = avcodec_decode_video2(ctx->m_codecContext->get(), avFrame.get(), &framesAvailable, &decodePacket);
			if (ret < 0) {
				return;
			}

			if (framesAvailable) {
				if (avFrame->pts == AV_NOPTS_VALUE) {
					avFrame->pts = avFrame->pkt_pts;
				}

				// TODO: try to draw raw instead of converting to RGBA
				AVPicture pic;
				avpicture_alloc(&pic, AV_PIX_FMT_RGBA, avFrame->width, avFrame->height);
				auto swctx = sws_getContext(avFrame->width, avFrame->height, static_cast<PixelFormat>(avFrame->format), avFrame->width, avFrame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
				sws_scale(swctx, avFrame->data, avFrame->linesize, 0, avFrame->height, pic.data, pic.linesize);

				assert(sizeof(frame.linesize) >= sizeof(avFrame->linesize));
				std::copy_n(pic.linesize, sizeof(avFrame->linesize), frame.linesize);
				assert(sizeof(frame.data) >= sizeof(avFrame->data));
				std::copy_n(pic.data, sizeof(avFrame->data), frame.data);
				frame.timestamp = av_rescale_q(avFrame->pts, videoStream->time_base, kNSTimeBase);
				frame.format = ffmpeg_to_obs_video_format(AV_PIX_FMT_RGBA);
				assert(frame.format != VIDEO_FORMAT_NONE);
				frame.width = avFrame->width;
				frame.height = avFrame->height;

				obs_source_output_video(ctx->m_source, &frame);
				sws_freeContext(swctx);
				avpicture_free(&pic);
			}

			offset += ret;
		}
	}
#if DEBUG
	std::cout << "Finished FFmpeg source decoding" << std::endl;
#endif /* DEBUG */
}

AVStream *FFmpegSource::findVideoStream()
{
	for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
		if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			return m_formatContext->streams[i];
		}
	}
	return nullptr;
}

FFmpegCodecContext::FFmpegCodecContext(AVStream *stream, AVCodec *codec):
	std::unique_ptr<AVCodecContext, void(*)(AVCodecContext *)>(avcodec_alloc_context3(codec), [](AVCodecContext* c) { avcodec_close(c); av_free(c); }),
	m_extradata(stream->codec->extradata, stream->codec->extradata + stream->codec->extradata_size)
{
	(*this)->extradata = m_extradata.data();
	(*this)->extradata_size = m_extradata.size();

	if (codec->id == AV_CODEC_ID_RAWVIDEO && (*this)->pix_fmt == AV_PIX_FMT_NONE) {
		(*this)->pix_fmt = stream->codec->pix_fmt;
	}

	if ((*this)->width == 0) {
		(*this)->width = stream->codec->width;
		(*this)->height = stream->codec->height;
	}

	if (avcodec_open2(this->get(), codec, nullptr) != 0) {
		this->reset(nullptr);
		std::cerr << "Couldn't open FFmpeg codec" << std::endl;
		return;
	}
}

FFmpegPacket::FFmpegPacket() {
	av_init_packet(this);
}

FFmpegPacket::FFmpegPacket(FFmpegPacket&& other):
	AVPacket(std::move(other))
{
	other.data = nullptr;
}

int FFmpegPacket::readFrame(AVFormatContext *ctx)
{
	if (this->data) {
		av_free_packet(this);
	}
	auto ret = av_read_frame(ctx, this);
	if (ret < 0) {
		this->data = nullptr;
	}
	return ret;
}

AVPacket FFmpegPacket::getOffsetPacket(int offset)
{
	AVPacket pkt = *this;
	pkt.data = this->data + offset;
	pkt.size = this->size - offset;
	return pkt;
}

FFmpegPacket::~FFmpegPacket() {
	if (this->data) {
		av_free_packet(this);
	}
}

static std::once_flag initAVFlag;

void *ffmpeg_source_create(obs_data_t settings, obs_source_t source)
{
	std::call_once(initAVFlag, []() { av_register_all(); avdevice_register_all(); });
	FFmpegSource *src = new FFmpegSource(settings, source);
	return (void *)src;
}

void ffmpeg_source_destroy(void *data)
{
	FFmpegSource *source = static_cast<FFmpegSource*>(data);
	delete source;
}

void ffmpeg_source_update(void *data, obs_data_t settings) {
	FFmpegSource *source = static_cast<FFmpegSource*>(data);
	source->update(settings);
}
