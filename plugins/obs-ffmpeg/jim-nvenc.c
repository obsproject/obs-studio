#include "jim-nvenc.h"
#include <util/circlebuf.h>
#include <util/darray.h>
#include <util/dstr.h>
#include <obs-avc.h>
#include <libavutil/rational.h>
#define INITGUID
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <obs-hevc.h>

/* ========================================================================= */
/* a hack of the ages: nvenc backward compatibility                          */

#define CONFIGURED_NVENC_MAJOR 12
#define CONFIGURED_NVENC_MINOR 0
#define CONFIGURED_NVENC_VER \
	(CONFIGURED_NVENC_MAJOR | (CONFIGURED_NVENC_MINOR << 24))

/* we cannot guarantee structures haven't changed, so purposely break on
 * version change to force the programmer to update or remove backward
 * compatibility NVENC code. */
#if CONFIGURED_NVENC_VER != NVENCAPI_VERSION
#error NVENC version changed, update or remove NVENC compatibility code
#endif

#undef NVENCAPI_STRUCT_VERSION
#define NVENCAPI_STRUCT_VERSION(ver)                              \
	((uint32_t)(enc->codec == CODEC_AV1 ? NVENCAPI_VERSION    \
					    : NVENC_COMPAT_VER) | \
	 ((ver) << 16) | (0x7 << 28))

#define NV_ENC_CONFIG_COMPAT_VER (NVENCAPI_STRUCT_VERSION(7) | (1 << 31))
#define NV_ENC_PIC_PARAMS_COMPAT_VER (NVENCAPI_STRUCT_VERSION(4) | (1 << 31))
#define NV_ENC_LOCK_BITSTREAM_COMPAT_VER NVENCAPI_STRUCT_VERSION(1)
#define NV_ENC_REGISTER_RESOURCE_COMPAT_VER NVENCAPI_STRUCT_VERSION(3)

/* ========================================================================= */

#define EXTRA_BUFFERS 5

#define do_log(level, format, ...)               \
	blog(level, "[jim-nvenc: '%s'] " format, \
	     obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

#define error_hr(msg) error("%s: %s: 0x%08lX", __FUNCTION__, msg, (uint32_t)hr);

struct nv_bitstream;
struct nv_texture;

struct handle_tex {
	uint32_t handle;
	ID3D11Texture2D *tex;
	IDXGIKeyedMutex *km;
};

/* ------------------------------------------------------------------------- */
/* Main Implementation Structure                                             */

enum codec_type {
	CODEC_H264,
	CODEC_HEVC,
	CODEC_AV1,
};

static const char *get_codec_name(enum codec_type type)
{
	switch (type) {
	case CODEC_H264:
		return "H264";
	case CODEC_HEVC:
		return "HEVC";
	case CODEC_AV1:
		return "AV1";
	}

	return "Unknown";
}

struct nvenc_data {
	obs_encoder_t *encoder;
	enum codec_type codec;
	GUID codec_guid;

	void *session;
	NV_ENC_INITIALIZE_PARAMS params;
	NV_ENC_CONFIG config;
	int rc_lookahead;
	int buf_count;
	int output_delay;
	int buffers_queued;
	size_t next_bitstream;
	size_t cur_bitstream;
	bool encode_started;
	bool first_packet;
	bool can_change_bitrate;
	int32_t bframes;

	DARRAY(struct nv_bitstream) bitstreams;
	DARRAY(struct nv_texture) textures;
	DARRAY(struct handle_tex) input_textures;
	struct circlebuf dts_list;

	DARRAY(uint8_t) packet_data;
	int64_t packet_pts;
	bool packet_keyframe;

	ID3D11Device *device;
	ID3D11DeviceContext *context;

	uint32_t cx;
	uint32_t cy;

	uint8_t *header;
	size_t header_size;

	uint8_t *sei;
	size_t sei_size;
};

/* ------------------------------------------------------------------------- */
/* Bitstream Buffer                                                          */

struct nv_bitstream {
	void *ptr;
};

#define NV_FAIL(format, ...) nv_fail(enc->encoder, format, __VA_ARGS__)
#define NV_FAILED(x) nv_failed(enc->encoder, x, __FUNCTION__, #x)

static bool nv_bitstream_init(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	NV_ENC_CREATE_BITSTREAM_BUFFER buf = {
		NV_ENC_CREATE_BITSTREAM_BUFFER_VER};

	if (NV_FAILED(nv.nvEncCreateBitstreamBuffer(enc->session, &buf))) {
		return false;
	}

	bs->ptr = buf.bitstreamBuffer;
	return true;
}

static void nv_bitstream_free(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	if (bs->ptr) {
		nv.nvEncDestroyBitstreamBuffer(enc->session, bs->ptr);
	}
}

/* ------------------------------------------------------------------------- */
/* Texture Resource                                                          */

struct nv_texture {
	void *res;
	ID3D11Texture2D *tex;
	void *mapped_res;
};

static bool nv_texture_init(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	const bool p010 = obs_p010_tex_active();

	D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = enc->cx;
	desc.Height = enc->cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;

	ID3D11Device *const device = enc->device;
	ID3D11Texture2D *tex;
	HRESULT hr = device->lpVtbl->CreateTexture2D(device, &desc, NULL, &tex);
	if (FAILED(hr)) {
		error_hr("Failed to create texture");
		return false;
	}

	tex->lpVtbl->SetEvictionPriority(tex, DXGI_RESOURCE_PRIORITY_MAXIMUM);

	uint32_t struct_ver = enc->codec == CODEC_AV1
				      ? NV_ENC_REGISTER_RESOURCE_VER
				      : NV_ENC_REGISTER_RESOURCE_COMPAT_VER;

	NV_ENC_REGISTER_RESOURCE res = {struct_ver};
	res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	res.resourceToRegister = tex;
	res.width = enc->cx;
	res.height = enc->cy;
	res.bufferFormat = p010 ? NV_ENC_BUFFER_FORMAT_YUV420_10BIT
				: NV_ENC_BUFFER_FORMAT_NV12;

	if (NV_FAILED(nv.nvEncRegisterResource(enc->session, &res))) {
		tex->lpVtbl->Release(tex);
		return false;
	}

	nvtex->res = res.registeredResource;
	nvtex->tex = tex;
	nvtex->mapped_res = NULL;
	return true;
}

static void nv_texture_free(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	if (nvtex->res) {
		if (nvtex->mapped_res) {
			nv.nvEncUnmapInputResource(enc->session,
						   nvtex->mapped_res);
		}
		nv.nvEncUnregisterResource(enc->session, nvtex->res);
		nvtex->tex->lpVtbl->Release(nvtex->tex);
	}
}

/* ------------------------------------------------------------------------- */
/* Implementation                                                            */

static const char *h264_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC H.264";
}

#ifdef ENABLE_HEVC
static const char *hevc_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC HEVC";
}
#endif

static const char *av1_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC AV1";
}

static inline int nv_get_cap(struct nvenc_data *enc, NV_ENC_CAPS cap)
{
	if (!enc->session)
		return 0;

	NV_ENC_CAPS_PARAM param = {NV_ENC_CAPS_PARAM_VER};
	int v;

	param.capsToQuery = cap;
	nv.nvEncGetEncodeCaps(enc->session, enc->codec_guid, &param, &v);
	return v;
}

static bool nvenc_update(void *data, obs_data_t *settings)
{
	struct nvenc_data *enc = data;

	/* Only support reconfiguration of CBR bitrate */
	if (enc->can_change_bitrate) {
		int bitrate = (int)obs_data_get_int(settings, "bitrate");

		enc->config.rcParams.averageBitRate = bitrate * 1000;
		enc->config.rcParams.maxBitRate = bitrate * 1000;

		NV_ENC_RECONFIGURE_PARAMS params = {0};
		params.version = NV_ENC_RECONFIGURE_PARAMS_VER;
		params.reInitEncodeParams = enc->params;
		params.resetEncoder = 1;
		params.forceIDR = 1;

		if (NV_FAILED(nv.nvEncReconfigureEncoder(enc->session,
							 &params))) {
			return false;
		}
	}

	return true;
}

static HANDLE get_lib(struct nvenc_data *enc, const char *lib)
{
	HMODULE mod = GetModuleHandleA(lib);
	if (mod)
		return mod;

	mod = LoadLibraryA(lib);
	if (!mod)
		error("Failed to load %s", lib);
	return mod;
}

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID, void **);

static bool init_d3d11(struct nvenc_data *enc, obs_data_t *settings)
{
	HMODULE dxgi = get_lib(enc, "DXGI.dll");
	HMODULE d3d11 = get_lib(enc, "D3D11.dll");
	CREATEDXGIFACTORY1PROC create_dxgi;
	PFN_D3D11_CREATE_DEVICE create_device;
	IDXGIFactory1 *factory;
	IDXGIAdapter *adapter;
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	HRESULT hr;

	if (!dxgi || !d3d11) {
		return false;
	}

	create_dxgi = (CREATEDXGIFACTORY1PROC)GetProcAddress(
		dxgi, "CreateDXGIFactory1");
	create_device = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(
		d3d11, "D3D11CreateDevice");

	if (!create_dxgi || !create_device) {
		error("Failed to load D3D11/DXGI procedures");
		return false;
	}

	hr = create_dxgi(&IID_IDXGIFactory1, &factory);
	if (FAILED(hr)) {
		error_hr("CreateDXGIFactory1 failed");
		return false;
	}

	hr = factory->lpVtbl->EnumAdapters(factory, 0, &adapter);
	factory->lpVtbl->Release(factory);
	if (FAILED(hr)) {
		error_hr("EnumAdapters failed");
		return false;
	}

	hr = create_device(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
			   D3D11_SDK_VERSION, &device, NULL, &context);
	adapter->lpVtbl->Release(adapter);
	if (FAILED(hr)) {
		error_hr("D3D11CreateDevice failed");
		return false;
	}

	enc->device = device;
	enc->context = context;
	return true;
}

static bool init_session(struct nvenc_data *enc)
{
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {
		NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER};
	params.device = enc->device;
	params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	params.apiVersion = enc->codec == CODEC_AV1 ? NVENCAPI_VERSION
						    : NVENC_COMPAT_VER;

	if (NV_FAILED(nv.nvEncOpenEncodeSessionEx(&params, &enc->session))) {
		return false;
	}
	return true;
}

static void initialize_params(struct nvenc_data *enc, const GUID *nv_preset,
			      NV_ENC_TUNING_INFO nv_tuning, uint32_t width,
			      uint32_t height, uint32_t fps_num,
			      uint32_t fps_den)
{
	int darWidth, darHeight;
	av_reduce(&darWidth, &darHeight, width, height, 1024 * 1024);

	NV_ENC_INITIALIZE_PARAMS *params = &enc->params;
	memset(params, 0, sizeof(*params));
	params->version = NV_ENC_INITIALIZE_PARAMS_VER;
	params->encodeGUID = enc->codec_guid;
	params->presetGUID = *nv_preset;
	params->encodeWidth = width;
	params->encodeHeight = height;
	params->darWidth = enc->codec == CODEC_AV1 ? width : darWidth;
	params->darHeight = enc->codec == CODEC_AV1 ? height : darHeight;
	params->frameRateNum = fps_num;
	params->frameRateDen = fps_den;
	params->enableEncodeAsync = 0;
	params->enablePTD = 1;
	params->encodeConfig = &enc->config;
	params->tuningInfo = nv_tuning;
}

static inline GUID get_nv_preset2(const char *preset2)
{
	if (astrcmpi(preset2, "p1") == 0) {
		return NV_ENC_PRESET_P1_GUID;
	} else if (astrcmpi(preset2, "p2") == 0) {
		return NV_ENC_PRESET_P2_GUID;
	} else if (astrcmpi(preset2, "p3") == 0) {
		return NV_ENC_PRESET_P3_GUID;
	} else if (astrcmpi(preset2, "p4") == 0) {
		return NV_ENC_PRESET_P4_GUID;
	} else if (astrcmpi(preset2, "p6") == 0) {
		return NV_ENC_PRESET_P6_GUID;
	} else if (astrcmpi(preset2, "p7") == 0) {
		return NV_ENC_PRESET_P7_GUID;
	} else {
		return NV_ENC_PRESET_P5_GUID;
	}
}

static inline NV_ENC_TUNING_INFO get_nv_tuning(const char *tuning)
{
	if (astrcmpi(tuning, "ll") == 0) {
		return NV_ENC_TUNING_INFO_LOW_LATENCY;
	} else if (astrcmpi(tuning, "ull") == 0) {
		return NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
	} else {
		return NV_ENC_TUNING_INFO_HIGH_QUALITY;
	}
}

static inline NV_ENC_MULTI_PASS get_nv_multipass(const char *multipass)
{
	if (astrcmpi(multipass, "qres") == 0) {
		return NV_ENC_TWO_PASS_QUARTER_RESOLUTION;
	} else if (astrcmpi(multipass, "fullres") == 0) {
		return NV_ENC_TWO_PASS_FULL_RESOLUTION;
	} else {
		return NV_ENC_MULTI_PASS_DISABLED;
	}
}

static bool init_encoder_base(struct nvenc_data *enc, obs_data_t *settings,
			      int bf, bool compatibility, bool *lossless)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int bitrate = (int)obs_data_get_int(settings, "bitrate");
	int max_bitrate = (int)obs_data_get_int(settings, "max_bitrate");
	int cqp = (int)obs_data_get_int(settings, "cqp");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	const char *preset = obs_data_get_string(settings, "preset");
	const char *preset2 = obs_data_get_string(settings, "preset2");
	const char *tuning = obs_data_get_string(settings, "tune");
	const char *multipass = obs_data_get_string(settings, "multipass");
	const char *profile = obs_data_get_string(settings, "profile");
	bool lookahead = obs_data_get_bool(settings, "lookahead");
	bool vbr = astrcmpi(rc, "VBR") == 0;
	bool psycho_aq = !compatibility &&
			 obs_data_get_bool(settings, "psycho_aq");
	NVENCSTATUS err;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	enc->cx = voi->width;
	enc->cy = voi->height;

	/* -------------------------- */
	/* get preset                 */

	GUID nv_preset = get_nv_preset2(preset2);
	NV_ENC_TUNING_INFO nv_tuning = get_nv_tuning(tuning);
	NV_ENC_MULTI_PASS nv_multipass = compatibility
						 ? NV_ENC_MULTI_PASS_DISABLED
						 : get_nv_multipass(multipass);

	if (obs_data_has_user_value(settings, "preset") &&
	    !obs_data_has_user_value(settings, "preset2") &&
	    enc->codec == CODEC_H264) {
		if (astrcmpi(preset, "mq") == 0) {
			nv_preset = NV_ENC_PRESET_P5_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_TWO_PASS_QUARTER_RESOLUTION;

		} else if (astrcmpi(preset, "hq") == 0) {
			nv_preset = NV_ENC_PRESET_P5_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "default") == 0) {
			nv_preset = NV_ENC_PRESET_P3_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "hp") == 0) {
			nv_preset = NV_ENC_PRESET_P1_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "ll") == 0) {
			nv_preset = NV_ENC_PRESET_P3_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_LOW_LATENCY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "llhq") == 0) {
			nv_preset = NV_ENC_PRESET_P4_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_LOW_LATENCY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "llhp") == 0) {
			nv_preset = NV_ENC_PRESET_P2_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_LOW_LATENCY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;
		}
	} else if (obs_data_has_user_value(settings, "preset") &&
		   !obs_data_has_user_value(settings, "preset2") &&
		   enc->codec == CODEC_HEVC) {
		if (astrcmpi(preset, "mq") == 0) {
			nv_preset = NV_ENC_PRESET_P6_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_TWO_PASS_QUARTER_RESOLUTION;

		} else if (astrcmpi(preset, "hq") == 0) {
			nv_preset = NV_ENC_PRESET_P6_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "default") == 0) {
			nv_preset = NV_ENC_PRESET_P5_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "hp") == 0) {
			nv_preset = NV_ENC_PRESET_P1_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "ll") == 0) {
			nv_preset = NV_ENC_PRESET_P3_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_LOW_LATENCY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "llhq") == 0) {
			nv_preset = NV_ENC_PRESET_P4_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_LOW_LATENCY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;

		} else if (astrcmpi(preset, "llhp") == 0) {
			nv_preset = NV_ENC_PRESET_P2_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_LOW_LATENCY;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;
		}
	}

	const bool rc_lossless = astrcmpi(rc, "lossless") == 0;
	*lossless = rc_lossless;
	if (rc_lossless) {
		*lossless =
			nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE);
		if (*lossless) {
			nv_tuning = NV_ENC_TUNING_INFO_LOSSLESS;
			nv_multipass = NV_ENC_MULTI_PASS_DISABLED;
		} else {
			warn("lossless encode is not supported, ignoring");
			nv_preset = NV_ENC_PRESET_P5_GUID;
			nv_tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY;
			nv_multipass = NV_ENC_TWO_PASS_QUARTER_RESOLUTION;
		}
	}

	/* -------------------------- */
	/* get preset default config  */

	uint32_t config_ver = enc->codec == CODEC_AV1
				      ? NV_ENC_CONFIG_VER
				      : NV_ENC_CONFIG_COMPAT_VER;

	NV_ENC_PRESET_CONFIG preset_config = {NV_ENC_PRESET_CONFIG_VER,
					      {config_ver}};

	err = nv.nvEncGetEncodePresetConfigEx(enc->session, enc->codec_guid,
					      nv_preset, nv_tuning,
					      &preset_config);
	if (nv_failed(enc->encoder, err, __FUNCTION__,
		      "nvEncGetEncodePresetConfig")) {
		return false;
	}

	/* -------------------------- */
	/* main configuration         */

	enc->config = preset_config.presetCfg;

	uint32_t gop_size =
		(keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;

	NV_ENC_CONFIG *config = &enc->config;

	initialize_params(enc, &nv_preset, nv_tuning, voi->width, voi->height,
			  voi->fps_num, voi->fps_den);

	config->gopLength = gop_size;
	config->frameIntervalP = 1 + bf;

	enc->bframes = bf;

	/* lookahead */
	const bool use_profile_lookahead = config->rcParams.enableLookahead;
	lookahead = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOOKAHEAD) &&
		    (lookahead || use_profile_lookahead);
	if (lookahead) {
		enc->rc_lookahead = use_profile_lookahead
					    ? config->rcParams.lookaheadDepth
					    : 8;
	}

	int buf_count = max(4, config->frameIntervalP * 2 * 2);
	if (lookahead) {
		buf_count = max(buf_count, config->frameIntervalP +
						   enc->rc_lookahead +
						   EXTRA_BUFFERS);
	}

	buf_count = min(64, buf_count);
	enc->buf_count = buf_count;

	const int output_delay = buf_count - 1;
	enc->output_delay = output_delay;

	if (lookahead) {
		const int lkd_bound = output_delay - config->frameIntervalP - 4;
		if (lkd_bound >= 0) {
			config->rcParams.enableLookahead = 1;
			config->rcParams.lookaheadDepth =
				max(enc->rc_lookahead, lkd_bound);
			config->rcParams.disableIadapt = 0;
			config->rcParams.disableBadapt = 0;
		} else {
			lookahead = false;
		}
	}

	/* psycho aq */
	if (!compatibility) {
		if (nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ)) {
			config->rcParams.enableAQ = psycho_aq;
			config->rcParams.aqStrength = 8;
			config->rcParams.enableTemporalAQ = psycho_aq;
		} else {
			warn("Ignoring Psycho Visual Tuning request since GPU is not capable");
		}
	}

	/* -------------------------- */
	/* rate control               */

	enc->can_change_bitrate =
		nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE) &&
		!lookahead;

	config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;

	if (astrcmpi(rc, "cqp") == 0 || rc_lossless) {
		if (*lossless)
			cqp = 0;

		int cqp_val = enc->codec == CODEC_AV1 ? cqp * 4 : cqp;

		config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
		config->rcParams.constQP.qpInterP = cqp_val;
		config->rcParams.constQP.qpInterB = cqp_val;
		config->rcParams.constQP.qpIntra = cqp_val;
		enc->can_change_bitrate = false;

		bitrate = 0;
		max_bitrate = 0;

	} else if (astrcmpi(rc, "vbr") != 0) { /* CBR by default */
		config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
	}

	config->rcParams.averageBitRate = bitrate * 1000;
	config->rcParams.maxBitRate = vbr ? max_bitrate * 1000 : bitrate * 1000;
	config->rcParams.vbvBufferSize = bitrate * 1000;
	config->rcParams.multiPass = nv_multipass;

	/* -------------------------- */
	/* initialize                 */

	info("settings:\n"
	     "\tcodec:        %s\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %s\n"
	     "\ttuning:       %s\n"
	     "\tmultipass:    %s\n"
	     "\tprofile:      %s\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tb-frames:     %d\n"
	     "\tlookahead:    %s\n"
	     "\tpsycho_aq:    %s\n",
	     get_codec_name(enc->codec), rc, bitrate, cqp, gop_size, preset2,
	     tuning, multipass, profile, enc->cx, enc->cy, bf,
	     lookahead ? "true" : "false", psycho_aq ? "true" : "false");

	return true;
}

static bool init_encoder_h264(struct nvenc_data *enc, obs_data_t *settings,
			      int bf, bool compatibility)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	const char *profile = obs_data_get_string(settings, "profile");
	bool lossless;

	if (!init_encoder_base(enc, settings, bf, compatibility, &lossless)) {
		return false;
	}

	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_H264 *h264_config = &config->encodeCodecConfig.h264Config;
	NV_ENC_CONFIG_H264_VUI_PARAMETERS *vui_params =
		&h264_config->h264VUIParameters;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	uint32_t gop_size =
		(keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;

	h264_config->idrPeriod = gop_size;

	bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	if (repeat_headers) {
		h264_config->repeatSPSPPS = 1;
		h264_config->disableSPSPPS = 0;
		h264_config->outputAUD = 1;
	}

	h264_config->sliceMode = 3;
	h264_config->sliceModeData = 1;

	h264_config->useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_DISABLED;

	vui_params->videoSignalTypePresentFlag = 1;
	vui_params->videoFullRangeFlag = (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		vui_params->colourPrimaries = 6;
		vui_params->transferCharacteristics = 6;
		vui_params->colourMatrix = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 1;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_SRGB:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 13;
		vui_params->colourMatrix = 1;
		break;
	}

	if (astrcmpi(rc, "lossless") == 0) {
		h264_config->qpPrimeYZeroTransformBypassFlag = 1;
	} else if (astrcmpi(rc, "vbr") != 0) { /* CBR */
		h264_config->outputBufferingPeriodSEI = 1;
	}

	h264_config->outputPictureTimingSEI = 1;

	/* -------------------------- */
	/* profile                    */

	if (astrcmpi(profile, "main") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
	} else if (astrcmpi(profile, "baseline") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	} else if (!lossless) {
		config->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
	}

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, &enc->params))) {
		return false;
	}

	return true;
}

static bool init_encoder_hevc(struct nvenc_data *enc, obs_data_t *settings,
			      int bf, bool compatibility)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	const char *profile = obs_data_get_string(settings, "profile");
	bool lossless;

	if (!init_encoder_base(enc, settings, bf, compatibility, &lossless)) {
		return false;
	}

	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_HEVC *hevc_config = &config->encodeCodecConfig.hevcConfig;
	NV_ENC_CONFIG_HEVC_VUI_PARAMETERS *vui_params =
		&hevc_config->hevcVUIParameters;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	uint32_t gop_size =
		(keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;

	hevc_config->idrPeriod = gop_size;

	bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	if (repeat_headers) {
		hevc_config->repeatSPSPPS = 1;
		hevc_config->disableSPSPPS = 0;
		hevc_config->outputAUD = 1;
	}

	hevc_config->sliceMode = 3;
	hevc_config->sliceModeData = 1;

	hevc_config->useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_DISABLED;

	vui_params->videoSignalTypePresentFlag = 1;
	vui_params->videoFullRangeFlag = (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		vui_params->colourPrimaries = 6;
		vui_params->transferCharacteristics = 6;
		vui_params->colourMatrix = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 1;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_SRGB:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 13;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_2100_PQ:
		vui_params->colourPrimaries = 9;
		vui_params->transferCharacteristics = 16;
		vui_params->colourMatrix = 9;
		vui_params->chromaSampleLocationFlag = 1;
		vui_params->chromaSampleLocationTop = 2;
		vui_params->chromaSampleLocationBot = 2;
		break;
	case VIDEO_CS_2100_HLG:
		vui_params->colourPrimaries = 9;
		vui_params->transferCharacteristics = 18;
		vui_params->colourMatrix = 9;
		vui_params->chromaSampleLocationFlag = 1;
		vui_params->chromaSampleLocationTop = 2;
		vui_params->chromaSampleLocationBot = 2;
	}

	hevc_config->pixelBitDepthMinus8 = obs_p010_tex_active() ? 2 : 0;

	if (astrcmpi(rc, "cbr") == 0) {
		hevc_config->outputBufferingPeriodSEI = 1;
	}

	hevc_config->outputPictureTimingSEI = 1;

	/* -------------------------- */
	/* profile                    */

	if (astrcmpi(profile, "main10") == 0) {
		config->profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
	} else if (obs_p010_tex_active()) {
		blog(LOG_WARNING, "[jim-nvenc] Forcing main10 for P010");
		config->profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
	} else {
		config->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
	}

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, &enc->params))) {
		return false;
	}

	return true;
}

static bool init_encoder_av1(struct nvenc_data *enc, obs_data_t *settings,
			     int bf, bool compatibility)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	bool lossless;

	if (!init_encoder_base(enc, settings, bf, compatibility, &lossless)) {
		return false;
	}

	NV_ENC_INITIALIZE_PARAMS *params = &enc->params;
	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_AV1 *av1_config = &config->encodeCodecConfig.av1Config;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	uint32_t gop_size =
		(keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;

	av1_config->idrPeriod = gop_size;

	av1_config->useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_DISABLED;

	av1_config->colorRange = (voi->range == VIDEO_RANGE_FULL);

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		av1_config->colorPrimaries = 6;
		av1_config->transferCharacteristics = 6;
		av1_config->matrixCoefficients = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		av1_config->colorPrimaries = 1;
		av1_config->transferCharacteristics = 1;
		av1_config->matrixCoefficients = 1;
		break;
	case VIDEO_CS_SRGB:
		av1_config->colorPrimaries = 1;
		av1_config->transferCharacteristics = 13;
		av1_config->matrixCoefficients = 1;
		break;
	case VIDEO_CS_2100_PQ:
		av1_config->colorPrimaries = 9;
		av1_config->transferCharacteristics = 16;
		av1_config->matrixCoefficients = 9;
		break;
	case VIDEO_CS_2100_HLG:
		av1_config->colorPrimaries = 9;
		av1_config->transferCharacteristics = 18;
		av1_config->matrixCoefficients = 9;
	}

	/* -------------------------- */
	/* profile                    */

	config->profileGUID = NV_ENC_AV1_PROFILE_MAIN_GUID;
	av1_config->tier = NV_ENC_TIER_AV1_0;

	av1_config->level = NV_ENC_LEVEL_AV1_AUTOSELECT;
	av1_config->chromaFormatIDC = 1;
	av1_config->pixelBitDepthMinus8 = obs_p010_tex_active() ? 2 : 0;
	av1_config->inputPixelBitDepthMinus8 = av1_config->pixelBitDepthMinus8;
	av1_config->numFwdRefs = 1;
	av1_config->numBwdRefs = 1;
	av1_config->repeatSeqHdr = 1;

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, &enc->params))) {
		return false;
	}

	return true;
}

static bool init_bitstreams(struct nvenc_data *enc)
{
	da_reserve(enc->bitstreams, enc->buf_count);
	for (int i = 0; i < enc->buf_count; i++) {
		struct nv_bitstream bitstream;
		if (!nv_bitstream_init(enc, &bitstream)) {
			return false;
		}

		da_push_back(enc->bitstreams, &bitstream);
	}

	return true;
}

static bool init_textures(struct nvenc_data *enc)
{
	da_reserve(enc->textures, enc->buf_count);
	for (int i = 0; i < enc->buf_count; i++) {
		struct nv_texture texture;
		if (!nv_texture_init(enc, &texture)) {
			return false;
		}

		da_push_back(enc->textures, &texture);
	}

	return true;
}

static void nvenc_destroy(void *data);

static bool init_specific_encoder(struct nvenc_data *enc, obs_data_t *settings,
				  int bf, bool compatibility)
{
	switch (enc->codec) {
	case CODEC_HEVC:
		return init_encoder_hevc(enc, settings, bf, compatibility);
	case CODEC_H264:
		return init_encoder_h264(enc, settings, bf, compatibility);
	case CODEC_AV1:
		return init_encoder_av1(enc, settings, bf, compatibility);
	}

	return false;
}

static bool init_encoder(struct nvenc_data *enc, enum codec_type codec,
			 obs_data_t *settings, obs_encoder_t *encoder)
{
	int bf = (int)obs_data_get_int(settings, "bf");
	const bool support_10bit =
		nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_10BIT_ENCODE);
	const int bf_max = nv_get_cap(enc, NV_ENC_CAPS_NUM_MAX_BFRAMES);

	if (obs_p010_tex_active() && !support_10bit) {
		NV_FAIL(obs_module_text("NVENC.10bitUnsupported"));
		return false;
	}

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		break;
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG:
			NV_FAIL(obs_module_text("NVENC.8bitUnsupportedHdr"));
			return false;
		}
	}

	if (bf > bf_max) {
		blog(LOG_WARNING,
		     "[jim-nvenc] Max B-frames setting (%d) is more than encoder supports (%d).\n"
		     "Setting B-frames to %d",
		     bf, bf_max, bf_max);
		bf = bf_max;
	}

	if (!init_specific_encoder(enc, settings, bf, false)) {
		blog(LOG_WARNING, "[jim-nvenc] init_specific_encoder failed, "
				  "trying again with compatibility options");

		nv.nvEncDestroyEncoder(enc->session);
		enc->session = NULL;

		if (!init_session(enc)) {
			return false;
		}
		/* try without multipass and psycho aq */
		if (!init_specific_encoder(enc, settings, bf, true)) {
			return false;
		}
	}

	return true;
}

static void *nvenc_create_internal(enum codec_type codec, obs_data_t *settings,
				   obs_encoder_t *encoder)
{
	struct nvenc_data *enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;
	enc->codec = codec;
	enc->first_packet = true;

	NV_ENCODE_API_FUNCTION_LIST init = {NV_ENCODE_API_FUNCTION_LIST_VER};

	switch (enc->codec) {
	case CODEC_H264:
		enc->codec_guid = NV_ENC_CODEC_H264_GUID;
		break;
	case CODEC_HEVC:
		enc->codec_guid = NV_ENC_CODEC_HEVC_GUID;
		break;
	case CODEC_AV1:
		enc->codec_guid = NV_ENC_CODEC_AV1_GUID;
		break;
	}

	if (!init_nvenc(encoder)) {
		goto fail;
	}
	if (NV_FAILED(nv_create_instance(&init))) {
		goto fail;
	}
	if (!init_d3d11(enc, settings)) {
		goto fail;
	}
	if (!init_session(enc)) {
		goto fail;
	}
	if (!init_encoder(enc, codec, settings, encoder)) {
		goto fail;
	}
	if (!init_bitstreams(enc)) {
		goto fail;
	}
	if (!init_textures(enc)) {
		goto fail;
	}

#ifdef ENABLE_HEVC
	enc->codec = codec;
#endif
	return enc;

fail:
	nvenc_destroy(enc);
	return NULL;
}

static void *nvenc_create_base(enum codec_type codec, obs_data_t *settings,
			       obs_encoder_t *encoder)
{
	/* this encoder requires shared textures, this cannot be used on a
	 * gpu other than the one OBS is currently running on. */
	const int gpu = (int)obs_data_get_int(settings, "gpu");
	if (gpu != 0) {
		blog(LOG_INFO,
		     "[jim-nvenc] different GPU selected by user, falling back to ffmpeg");
		goto reroute;
	}

	if (obs_encoder_scaling_enabled(encoder)) {
		blog(LOG_INFO,
		     "[jim-nvenc] scaling enabled, falling back to ffmpeg");
		goto reroute;
	}

	if (!obs_p010_tex_active() && !obs_nv12_tex_active()) {
		blog(LOG_INFO,
		     "[jim-nvenc] nv12/p010 not active, falling back to ffmpeg");
		goto reroute;
	}

	struct nvenc_data *enc =
		nvenc_create_internal(codec, settings, encoder);

	if (enc) {
		return enc;
	}

reroute:
	switch (codec) {
	case CODEC_H264:
		return obs_encoder_create_rerouted(encoder, "ffmpeg_nvenc");
	case CODEC_HEVC:
		return obs_encoder_create_rerouted(encoder,
						   "ffmpeg_hevc_nvenc");
	}

	return NULL;
}

static void *h264_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_H264, settings, encoder);
}

#ifdef ENABLE_HEVC
static void *hevc_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_HEVC, settings, encoder);
}
#endif

static void *av1_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_AV1, settings, encoder);
}

static bool get_encoded_packet(struct nvenc_data *enc, bool finalize);

static void nvenc_destroy(void *data)
{
	struct nvenc_data *enc = data;

	if (enc->encode_started) {
		size_t next_bitstream = enc->next_bitstream;

		uint32_t struct_ver = enc->codec == CODEC_AV1
					      ? NV_ENC_PIC_PARAMS_VER
					      : NV_ENC_PIC_PARAMS_COMPAT_VER;
		NV_ENC_PIC_PARAMS params = {struct_ver};
		params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
		nv.nvEncEncodePicture(enc->session, &params);
		get_encoded_packet(enc, true);
	}
	for (size_t i = 0; i < enc->textures.num; i++) {
		nv_texture_free(enc, &enc->textures.array[i]);
	}
	for (size_t i = 0; i < enc->bitstreams.num; i++) {
		nv_bitstream_free(enc, &enc->bitstreams.array[i]);
	}
	if (enc->session) {
		nv.nvEncDestroyEncoder(enc->session);
	}
	for (size_t i = 0; i < enc->input_textures.num; i++) {
		ID3D11Texture2D *tex = enc->input_textures.array[i].tex;
		IDXGIKeyedMutex *km = enc->input_textures.array[i].km;
		tex->lpVtbl->Release(tex);
		km->lpVtbl->Release(km);
	}
	if (enc->context) {
		enc->context->lpVtbl->Release(enc->context);
	}
	if (enc->device) {
		enc->device->lpVtbl->Release(enc->device);
	}

	bfree(enc->header);
	bfree(enc->sei);
	circlebuf_free(&enc->dts_list);
	da_free(enc->textures);
	da_free(enc->bitstreams);
	da_free(enc->input_textures);
	da_free(enc->packet_data);
	bfree(enc);
}

static ID3D11Texture2D *get_tex_from_handle(struct nvenc_data *enc,
					    uint32_t handle,
					    IDXGIKeyedMutex **km_out)
{
	ID3D11Device *device = enc->device;
	IDXGIKeyedMutex *km;
	ID3D11Texture2D *input_tex;
	HRESULT hr;

	for (size_t i = 0; i < enc->input_textures.num; i++) {
		struct handle_tex *ht = &enc->input_textures.array[i];
		if (ht->handle == handle) {
			*km_out = ht->km;
			return ht->tex;
		}
	}

	hr = device->lpVtbl->OpenSharedResource(device,
						(HANDLE)(uintptr_t)handle,
						&IID_ID3D11Texture2D,
						&input_tex);
	if (FAILED(hr)) {
		error_hr("OpenSharedResource failed");
		return NULL;
	}

	hr = input_tex->lpVtbl->QueryInterface(input_tex, &IID_IDXGIKeyedMutex,
					       &km);
	if (FAILED(hr)) {
		error_hr("QueryInterface(IDXGIKeyedMutex) failed");
		input_tex->lpVtbl->Release(input_tex);
		return NULL;
	}

	input_tex->lpVtbl->SetEvictionPriority(input_tex,
					       DXGI_RESOURCE_PRIORITY_MAXIMUM);

	*km_out = km;

	struct handle_tex new_ht = {handle, input_tex, km};
	da_push_back(enc->input_textures, &new_ht);
	return input_tex;
}

static bool get_encoded_packet(struct nvenc_data *enc, bool finalize)
{
	void *s = enc->session;

	da_resize(enc->packet_data, 0);

	if (!enc->buffers_queued)
		return true;
	if (!finalize && enc->buffers_queued < enc->output_delay)
		return true;

	size_t count = finalize ? enc->buffers_queued : 1;

	for (size_t i = 0; i < count; i++) {
		size_t cur_bs_idx = enc->cur_bitstream;
		struct nv_bitstream *bs = &enc->bitstreams.array[cur_bs_idx];
		struct nv_texture *nvtex = &enc->textures.array[cur_bs_idx];

		/* ---------------- */

		uint32_t struct_ver =
			enc->codec == CODEC_AV1
				? NV_ENC_LOCK_BITSTREAM_VER
				: NV_ENC_LOCK_BITSTREAM_COMPAT_VER;

		NV_ENC_LOCK_BITSTREAM lock = {struct_ver};
		lock.outputBitstream = bs->ptr;
		lock.doNotWait = false;

		if (NV_FAILED(nv.nvEncLockBitstream(s, &lock))) {
			return false;
		}

		if (enc->first_packet) {
			NV_ENC_SEQUENCE_PARAM_PAYLOAD payload = {0};
			uint8_t buf[256];
			uint32_t size = 0;

			payload.version = NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER;
			payload.spsppsBuffer = buf;
			payload.inBufferSize = sizeof(buf);
			payload.outSPSPPSPayloadSize = &size;

			nv.nvEncGetSequenceParams(s, &payload);
			enc->header = bmemdup(buf, size);
			enc->header_size = size;
			enc->first_packet = false;
		}

		da_copy_array(enc->packet_data, lock.bitstreamBufferPtr,
			      lock.bitstreamSizeInBytes);

		enc->packet_pts = (int64_t)lock.outputTimeStamp;
		enc->packet_keyframe = lock.pictureType == NV_ENC_PIC_TYPE_IDR;

		if (NV_FAILED(nv.nvEncUnlockBitstream(s, bs->ptr))) {
			return false;
		}

		/* ---------------- */

		if (nvtex->mapped_res) {
			NVENCSTATUS err;
			err = nv.nvEncUnmapInputResource(s, nvtex->mapped_res);
			if (nv_failed(enc->encoder, err, __FUNCTION__,
				      "unmap")) {
				return false;
			}
			nvtex->mapped_res = NULL;
		}

		/* ---------------- */

		if (++enc->cur_bitstream == enc->buf_count)
			enc->cur_bitstream = 0;

		enc->buffers_queued--;
	}

	return true;
}

static bool nvenc_encode_tex(void *data, uint32_t handle, int64_t pts,
			     uint64_t lock_key, uint64_t *next_key,
			     struct encoder_packet *packet,
			     bool *received_packet)
{
	struct nvenc_data *enc = data;
	ID3D11Device *device = enc->device;
	ID3D11DeviceContext *context = enc->context;
	ID3D11Texture2D *input_tex;
	ID3D11Texture2D *output_tex;
	IDXGIKeyedMutex *km;
	struct nv_texture *nvtex;
	struct nv_bitstream *bs;
	NVENCSTATUS err;

	if (handle == GS_INVALID_HANDLE) {
		error("Encode failed: bad texture handle");
		*next_key = lock_key;
		return false;
	}

	bs = &enc->bitstreams.array[enc->next_bitstream];
	nvtex = &enc->textures.array[enc->next_bitstream];

	input_tex = get_tex_from_handle(enc, handle, &km);
	output_tex = nvtex->tex;

	if (!input_tex) {
		*next_key = lock_key;
		return false;
	}

	circlebuf_push_back(&enc->dts_list, &pts, sizeof(pts));

	/* ------------------------------------ */
	/* copy to output tex                   */

	km->lpVtbl->AcquireSync(km, lock_key, INFINITE);

	context->lpVtbl->CopyResource(context, (ID3D11Resource *)output_tex,
				      (ID3D11Resource *)input_tex);

	km->lpVtbl->ReleaseSync(km, *next_key);

	/* ------------------------------------ */
	/* map output tex so nvenc can use it   */

	NV_ENC_MAP_INPUT_RESOURCE map = {NV_ENC_MAP_INPUT_RESOURCE_VER};
	map.registeredResource = nvtex->res;
	if (NV_FAILED(nv.nvEncMapInputResource(enc->session, &map))) {
		return false;
	}

	nvtex->mapped_res = map.mappedResource;

	/* ------------------------------------ */
	/* do actual encode call                */

	NV_ENC_PIC_PARAMS params = {0};
	params.version = enc->codec == CODEC_AV1 ? NV_ENC_PIC_PARAMS_VER
						 : NV_ENC_PIC_PARAMS_COMPAT_VER;
	params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	params.inputBuffer = nvtex->mapped_res;
	params.bufferFmt = obs_p010_tex_active()
				   ? NV_ENC_BUFFER_FORMAT_YUV420_10BIT
				   : NV_ENC_BUFFER_FORMAT_NV12;
	params.inputTimeStamp = (uint64_t)pts;
	params.inputWidth = enc->cx;
	params.inputHeight = enc->cy;
	params.inputPitch = enc->cx;
	params.outputBitstream = bs->ptr;

	err = nv.nvEncEncodePicture(enc->session, &params);
	if (err != NV_ENC_SUCCESS && err != NV_ENC_ERR_NEED_MORE_INPUT) {
		nv_failed(enc->encoder, err, __FUNCTION__,
			  "nvEncEncodePicture");
		return false;
	}

	enc->encode_started = true;
	enc->buffers_queued++;

	if (++enc->next_bitstream == enc->buf_count) {
		enc->next_bitstream = 0;
	}

	/* ------------------------------------ */
	/* check for encoded packet and parse   */

	if (!get_encoded_packet(enc, false)) {
		return false;
	}

	/* ------------------------------------ */
	/* output encoded packet                */

	if (enc->packet_data.num) {
		int64_t dts;
		circlebuf_pop_front(&enc->dts_list, &dts, sizeof(dts));

		/* subtract bframe delay from dts */
		dts -= (int64_t)enc->bframes * packet->timebase_num;

		*received_packet = true;
		packet->data = enc->packet_data.array;
		packet->size = enc->packet_data.num;
		packet->type = OBS_ENCODER_VIDEO;
		packet->pts = enc->packet_pts;
		packet->dts = dts;
		packet->keyframe = enc->packet_keyframe;
	} else {
		*received_packet = false;
	}

	return true;
}

extern void h264_nvenc_defaults(obs_data_t *settings);
extern obs_properties_t *h264_nvenc_properties(void *unused);
#ifdef ENABLE_HEVC
extern void hevc_nvenc_defaults(obs_data_t *settings);
extern obs_properties_t *hevc_nvenc_properties(void *unused);
#endif
extern obs_properties_t *av1_nvenc_properties(void *unused);
extern void av1_nvenc_defaults(obs_data_t *settings);

static bool nvenc_extra_data(void *data, uint8_t **header, size_t *size)
{
	struct nvenc_data *enc = data;

	if (!enc->header) {
		return false;
	}

	*header = enc->header;
	*size = enc->header_size;
	return true;
}

static bool nvenc_sei_data(void *data, uint8_t **sei, size_t *size)
{
	struct nvenc_data *enc = data;

	if (!enc->sei) {
		return false;
	}

	*sei = enc->sei;
	*size = enc->sei_size;
	return true;
}

struct obs_encoder_info h264_nvenc_info = {
	.id = "jim_nvenc",
	.codec = "h264",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE,
	.get_name = h264_nvenc_get_name,
	.create = h264_nvenc_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
	.encode_texture = nvenc_encode_tex,
	.get_defaults = h264_nvenc_defaults,
	.get_properties = h264_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
};

#ifdef ENABLE_HEVC
struct obs_encoder_info hevc_nvenc_info = {
	.id = "jim_hevc_nvenc",
	.codec = "hevc",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE,
	.get_name = hevc_nvenc_get_name,
	.create = hevc_nvenc_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
	.encode_texture = nvenc_encode_tex,
	.get_defaults = hevc_nvenc_defaults,
	.get_properties = hevc_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
};
#endif

struct obs_encoder_info av1_nvenc_info = {
	.id = "jim_av1_nvenc",
	.codec = "av1",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE,
	.get_name = av1_nvenc_get_name,
	.create = av1_nvenc_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
	.encode_texture = nvenc_encode_tex,
	.get_defaults = av1_nvenc_defaults,
	.get_properties = av1_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
};
