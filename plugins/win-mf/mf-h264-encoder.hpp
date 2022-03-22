#pragma once

#include <obs-module.h>

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#undef WIN32_MEAN_AND_LEAN

#include <mfapi.h>
#include <mfidl.h>

#include <wmcodecdsp.h>

#include <vector>
#include <queue>
#include <memory>
#include <atomic>

#include <util/windows/ComPtr.hpp>

#include "mf-encoder-descriptor.hpp"
#include "mf-common.hpp"

namespace MF {
	enum H264Profile {
		H264ProfileBaseline,
		H264ProfileMain,
		H264ProfileHigh
	};

	enum H264RateControl {
		H264RateControlCBR,
		H264RateControlConstrainedVBR,
		H264RateControlVBR,
		H264RateControlCQP
	};

	struct H264QP {
		UINT16 defaultQp;
		UINT16 i;
		UINT16 p;
		UINT16 b;

		UINT64 Pack(bool packDefault) {
			int shift = packDefault ? 0 : 16;
			UINT64 packedQp;
			if (packDefault)
				packedQp = defaultQp;

			packedQp |= i << shift;
			shift += 16;
			packedQp |= p << shift;
			shift += 16;
			packedQp |= b << shift;

			return packedQp;
		}
	};

	enum H264EntropyEncoding {
		H264EntropyEncodingCABLC,
		H264EntropyEncodingCABAC
	};

	struct H264Frame {
	public:
		H264Frame(bool keyframe, UINT64 pts, UINT64 dts,
				std::unique_ptr<std::vector<uint8_t>> data)
			: keyframe(keyframe), pts(pts), dts(dts),
			  data(std::move(data))
		{}
		bool Keyframe() { return keyframe; }
		BYTE *Data() { return data.get()->data(); }
		DWORD DataLength() { return (DWORD)data.get()->size(); }
		INT64 Pts() { return pts; }
		INT64 Dts() { return dts; }

	private:
		H264Frame(H264Frame const&) = delete;
		H264Frame& operator=(H264Frame const&) = delete;
	private:
		bool keyframe;
		INT64 pts;
		INT64 dts;
		std::unique_ptr<std::vector<uint8_t>> data;
	};

	class H264Encoder {
	public:
		H264Encoder(const obs_encoder_t *encoder,
			std::shared_ptr<EncoderDescriptor> descriptor,
			UINT32 width,
			UINT32 height,
			UINT32 framerateNum,
			UINT32 framerateDen,
			H264Profile profile,
			UINT32 bitrate);

		~H264Encoder();

		bool Initialize(std::function<bool(void)> func);
		bool ProcessInput(UINT8 **data, UINT32 *linesize, UINT64 pts,
			Status *status);
		bool ProcessOutput(UINT8 **data, UINT32 *dataLength,
			UINT64 *pts, UINT64 *dts, bool *keyframe,
			Status *status);
		bool ExtraData(UINT8 **data, UINT32 *dataLength);

		const obs_encoder_t *ObsEncoder() { return encoder; }

	public:
		bool SetBitrate(UINT32 bitrate);
		bool SetQP(H264QP &qp);
		bool SetMaxBitrate(UINT32 maxBitrate);
		bool SetRateControl(H264RateControl rateControl);
		bool SetKeyframeInterval(UINT32 seconds);
		bool SetLowLatency(bool lowLatency);
		bool SetBufferSize(UINT32 bufferSize);
		bool SetBFrameCount(UINT32 bFrames);
		bool SetEntropyEncoding(H264EntropyEncoding entropyEncoding);
		bool SetMinQP(UINT32 minQp);
		bool SetMaxQP(UINT32 maxQp);

	private:
		H264Encoder(H264Encoder const&) = delete;
		H264Encoder& operator=(H264Encoder const&) = delete;

	private:
		HRESULT InitializeEventGenerator();
		HRESULT InitializeExtraData();
		HRESULT CreateMediaTypes(ComPtr<IMFMediaType> &inputType,
			ComPtr<IMFMediaType> &outputType);
		HRESULT EnsureCapacity(ComPtr<IMFSample> &sample, DWORD length);
		HRESULT CreateEmptySample(ComPtr<IMFSample> &sample,
			ComPtr<IMFMediaBuffer> &buffer, DWORD length);

		HRESULT ProcessInput(ComPtr<IMFSample> &sample);
		HRESULT ProcessOutput();

		HRESULT DrainEvent(bool block);
		HRESULT DrainEvents();
	private:
		const obs_encoder_t *encoder;
		std::shared_ptr<EncoderDescriptor> descriptor;
		const UINT32 width;
		const UINT32 height;
		const UINT32 framerateNum;
		const UINT32 framerateDen;
		const UINT32 initialBitrate;
		const H264Profile profile;

		bool createOutputSample;
		ComPtr<IMFTransform> transform;
		ComPtr<ICodecAPI> codecApi;

		std::vector<BYTE> extraData;

		// The frame returned by ProcessOutput
		// Valid until the next call to ProcessOutput
		std::unique_ptr<H264Frame> activeFrame;

		// Queued input samples that the encoder was not ready
		// to process
		std::queue<ComPtr<IMFSample>> inputSamples;

		// Queued output samples that have not been returned from
		// ProcessOutput yet
		std::queue<std::unique_ptr<H264Frame>> encodedFrames;

		ComPtr<IMFMediaEventGenerator> eventGenerator;
		std::atomic<UINT32> inputRequests = 0;
		std::atomic<UINT32> outputRequests = 0;
		std::atomic<UINT32> pendingRequests = 0;
	};
}
