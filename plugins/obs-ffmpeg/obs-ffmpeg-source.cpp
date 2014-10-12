#include "obs-ffmpeg-source.h"
#include "obs-ffmpeg-formats.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>

#define fflog(level, source, format, ...) \
	blog(level, "%s: " format, \
			obs_source_get_name(source), ##__VA_ARGS__)

FFmpegCodecContext::FFmpegCodecContext():
	std::unique_ptr<AVCodecContext, void(*)(AVCodecContext*)>(nullptr,
			[](AVCodecContext *c) {})
{
}

FFmpegCodecContext::FFmpegCodecContext(AVStream *stream, const AVCodec *codec):
	std::unique_ptr<AVCodecContext,
			void(*)(AVCodecContext*)>(avcodec_alloc_context3(codec),
				[](AVCodecContext *c) { avcodec_close(c); av_free(c); }),
	m_extradata(stream->codec->extradata,
			stream->codec->extradata + stream->codec->extradata_size)
{
	(*this)->extradata = m_extradata.data();
	(*this)->extradata_size = m_extradata.size();

	if ((*this)->width == 0) {
		(*this)->width = stream->codec->width;
	}
	if ((*this)->height == 0) {
		(*this)->height = stream->codec->height;
	}
	
	if ((*this)->pix_fmt == AV_PIX_FMT_NONE) {
		(*this)->pix_fmt = stream->codec->pix_fmt;
	}

	if (avcodec_open2(this->get(), codec, nullptr) != 0) {
		this->reset(nullptr);
		throw std::runtime_error("Couldn't open FFmpeg codec");
	}
}

FFmpegPacket::FFmpegPacket()
{
	av_init_packet(this);
}

FFmpegPacket::FFmpegPacket(FFmpegPacket&& other):
	AVPacket(std::move(other))
{
	other.data = nullptr;
}

FFmpegPacket::~FFmpegPacket()
{
	FreeData();
}

void FFmpegPacket::FreeData()
{
	if (this->data) {
		av_free_packet(this);
	}
}

AVPacket FFmpegPacket::GetOffsetPacket(int offset)
{
	AVPacket pkt = *this;
	pkt.data += offset;
	pkt.size -= offset;
	return pkt;
}

FFmpegSource::FFmpegSource(obs_source_t *source):
	m_source(source),
	m_formatContext(avformat_alloc_context(), &avformat_free_context),
	m_alive(true),
	m_thread(&FFmpegSource::DecodeThread, this)
{
}

FFmpegSource::~FFmpegSource()
{
	Shutdown();
}

void FFmpegSource::Shutdown()
{
	m_alive = false;
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void FFmpegSource::Update(obs_data_t *settings)
{
	std::unique_lock<std::mutex> lck(m_mutex);
	// First invalidate the existing source
	m_codecContext.reset(nullptr);
	m_formatContext.reset(nullptr);
	
	// Try to find the user's desired input format
	AVInputFormat *iFormat = nullptr;
	const char *desiredFormat = obs_data_get_string(settings, "format");
	if (strlen(desiredFormat) > 0) {
		iFormat = av_find_input_format(desiredFormat);
		if (iFormat == nullptr) {
			throw std::runtime_error("Cannot find desired FFmpeg input format");
		}
	}

	// Instantiate the formatContext
	AVFormatContext *fmt = nullptr;
	if (avformat_open_input(&fmt,
				obs_data_get_string(settings, "filename"),
				iFormat,
				nullptr) != 0) {
		throw std::runtime_error("Cannot open FFmpeg input");
	}
	assert(fmt != nullptr);
	m_formatContext = std::unique_ptr<AVFormatContext,
					void(*)(AVFormatContext*)>(fmt, [](AVFormatContext *c) {
						avformat_close_input(&c);
					});

	// Find stream info
	if (avformat_find_stream_info(m_formatContext.get(), nullptr) != 0) {
		throw std::runtime_error("Cannot find stream info for FFmpeg input");
	}

	// Find video stream
	AVStream *videoStream = FindVideoStream();
	if (videoStream == nullptr) {
		throw std::runtime_error("FFmpeg input had no video stream");
	}

	// Find video codec
	const AVCodec *videoCodec = avcodec_find_decoder(videoStream->codec->codec_id);
	if (videoCodec == nullptr) {
		throw std::runtime_error("Cannot find decoder for FFmpeg video stream");
	}

	// Instantiate the codec context
	m_codecContext = FFmpegCodecContext(videoStream, videoCodec);

#if DEBUG
	av_dump_format(m_formatContext.get(), 0, m_formatContext->filename, 0);
#endif /* DEBUG */
}

bool FFmpegSource::IsValid()
{
	return (m_codecContext != nullptr);
}

AVStream *FFmpegSource::FindVideoStream()
{
	for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
		if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			return m_formatContext->streams[i];
		}
	}
	return nullptr;
}

void FFmpegSource::DecodeThread()
{
	FFmpegPacket pkt;
	while (m_alive) {
		std::unique_lock<std::mutex> lck(m_mutex);
		if (!IsValid()) {
			continue;
		}
		if (ReadFrame(&pkt) < 0) {
			continue;
		}
		// If this packet isn't for the video stream, skip
		if (pkt.stream_index != FindVideoStream()->index) {
			continue;
		}
		ProcessFrame(pkt);
	}
}

void FFmpegSource::OutputFrame(AVFrame *avFrame)
{
	struct obs_source_frame frame;

	AVPicture pic;
	avpicture_alloc(&pic, AV_PIX_FMT_RGBA, avFrame->width, avFrame->height);
	auto swCtx = sws_getContext(avFrame->width, avFrame->height,
			static_cast<PixelFormat>(avFrame->format),
			avFrame->width, avFrame->height, AV_PIX_FMT_RGBA,
			SWS_BILINEAR, nullptr, nullptr, nullptr);
	assert(swCtx != nullptr);
	sws_scale(swCtx,
			avFrame->data,
			avFrame->linesize,
			0,
			avFrame->height,
			pic.data,
			pic.linesize);	
	assert(sizeof(frame.linesize) >= sizeof(pic.linesize));
	std::copy(pic.linesize, pic.linesize + sizeof(pic.linesize), frame.linesize);
	assert(sizeof(frame.data) >= sizeof(pic.data));
	memcpy(frame.data, pic.data, sizeof(pic.data));

	frame.timestamp = avFrame->pts;
	frame.format = ffmpeg_to_obs_video_format(AV_PIX_FMT_RGBA);
	frame.width = avFrame->width;
	frame.height = avFrame->height;

	obs_source_output_video(m_source, &frame);
	sws_freeContext(swCtx);
	avpicture_free(&pic);
}

void FFmpegSource::ProcessFrame(FFmpegPacket& pkt) {
	int offset = 0;
	std::unique_ptr<AVFrame, void(*)(void *)> frame(av_frame_alloc(), &av_free);
	while (offset < pkt.size) {
		AVPacket packetToDecode = pkt.GetOffsetPacket(offset);
		int framesAvailable = 0;
		const int ret = avcodec_decode_video2(m_codecContext.get(),
				frame.get(),
				&framesAvailable,
				&packetToDecode);
		if (ret < 0) {
			return;
		}
		if (framesAvailable) {
			if (frame->pts == AV_NOPTS_VALUE) {
				frame->pts = frame->pkt_pts;
			}

			frame->pts = av_rescale_q(frame->pts,
					FindVideoStream()->time_base,
					kNSTimeBase);

			OutputFrame(frame.get());
		}
		offset += ret;
	}
}

int FFmpegSource::ReadFrame(FFmpegPacket *pkt)
{
	pkt->FreeData();
	int ret = av_read_frame(m_formatContext.get(), pkt);
	if (ret < 0) {
		pkt->data = nullptr;
	}
	return ret;
}

static void TryUpdate(FFmpegSource& source, obs_data_t *settings)
{
	try {
		source.Update(settings);
	} catch(std::runtime_error& e) {
		fflog(LOG_ERROR, source.m_source, "%s", e.what());
	}
}

static std::once_flag initAVFlag;

void *ffmpeg_source_create(obs_data_t *settings, obs_source_t *source)
{
	// TODO: figure out how this might conflict with other FFmpeg modules
	std::call_once(initAVFlag,
			[]() {
				av_register_all();
				avdevice_register_all();
			});
	FFmpegSource *src = new FFmpegSource(source);
	assert(src != nullptr);
	TryUpdate(*src, settings);
	return (void *)src;
}

void ffmpeg_source_destroy(void *data)
{
	FFmpegSource *source = static_cast<FFmpegSource*>(data);
	assert(source != nullptr);
	delete source;
}

void ffmpeg_source_update(void *data, obs_data_t *settings) {
	FFmpegSource *src = static_cast<FFmpegSource*>(data);
	assert(src != nullptr);
	TryUpdate(*src, settings);
}
