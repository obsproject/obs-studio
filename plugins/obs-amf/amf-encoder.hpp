#pragma once

#include <obs-module.h>
#include <obs-encoder.h>
#include "AMF/include/core/Factory.h"
#include <d3d11.h>
#include <d3d11_1.h>
#include <util/circlebuf.h>
#include "strings.hpp"
#include <cinttypes>
#include <condition_variable>
#include <vector>
#include <atlbase.h>

#define AMF_PRESENT_TIMESTAMP L"PTS"
#define SET_AMF_VALUE_OR_FAIL(object, name, val)                               \
	result = object->SetProperty(name, val);                               \
	if (result != AMF_OK) {                                                \
		PLOG_ERROR("Failed to set %ls, error code %d.", name, result); \
		goto fail;                                                     \
	}
#define SET_AMF_VALUE(object, name, val)                                \
	result = object->SetProperty(name, val);                        \
	if (result != AMF_OK) {                                         \
		PLOG_WARNING("Failed to set %ls, error code %d.", name, \
			     result);                                   \
	}

#ifdef __cplusplus
extern "C" {
#endif

using namespace amf;

struct handle_tex {
	uint32_t handle;
	CComPtr<ID3D11Texture2D> tex;
	IDXGIKeyedMutex *km;
};

struct amf_data {
	enum CODEC_ID { CODEC_ID_H264, CODEC_ID_HEVC };
	obs_encoder_t *encoder;

	void *session;
	amf::AMFContextPtr context;
	amf::AMFComponentPtr encoder_amf;
	CODEC_ID codec;

	ATL::CComPtr<ID3D11Device> pD3D11Device;
	ATL::CComPtr<ID3D11DeviceContext> pD3D11Context;

	int frameW;
	int frameH;
	AMFRate frame_rate;

	AMFBufferPtr header;
	AMFBufferPtr sei;

	std::chrono::nanoseconds query_wait_time;
	amf::AMFBufferPtr packet_data;

	/// Timings
	double_t timestamp_step;
	DARRAY(struct handle_tex) input_textures;
	CComPtr<ID3D11Texture2D> texture;
};

enum class Presets : int8_t {
	None = -1,
	Streaming,
	Recording,
	LowLatency,
};

enum class ViewMode : uint8_t { Basic, Advanced };

AMF_RESULT init_d3d11(obs_data_t *settings, struct amf_data *enc);
bool amf_encode_tex(void *data, uint32_t handle, int64_t pts, uint64_t lock_key,
		    uint64_t *next_key, struct encoder_packet *packet,
		    bool *received_packet);
void amf_destroy(void *data);

bool amf_extra_data(void *data, uint8_t **header, size_t *size);

AMF_RESULT amf_create_encoder(obs_data_t *settings, amf_data *enc);

void register_amf_h264_encoder();
void register_amf_hevc_encoder();

#ifdef __cplusplus
}
#endif
