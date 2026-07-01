#pragma once

#include <util/platform.h>
#include <stdint.h>

typedef enum {
	MODE_INVALID = -1,
	MODE_UNKNOWN = 0,
	MODE_1 = 1,
	MODE_2 = 2,
	MODE_1_2 = 3,
	MODE_1_2_1 = 4,
	MODE_1_2_2 = 5,
	MODE_1_2_2_1 = 6,
	MODE_1_2_2_2_1 = 7,
	MODE_6_1 = 11,
	MODE_7_1_BACK = 12,
	MODE_7_1_TOP_FRONT = 14,
	MODE_7_1_REAR_SURROUND = 33,
	MODE_7_1_FRONT_CENTER = 34,
	MODE_212 = 128
} CHANNEL_MODE;

typedef enum {
	AACENC_OK = 0x0000,
	AACENC_INVALID_HANDLE = 0x0020,
	AACENC_MEMORY_ERROR = 0x0021,
	AACENC_UNSUPPORTED_PARAMETER = 0x0022,
	AACENC_INVALID_CONFIG = 0x0023,
	AACENC_INIT_ERROR = 0x0040,
	AACENC_INIT_AAC_ERROR = 0x0041,
	AACENC_INIT_SBR_ERROR = 0x0042,
	AACENC_INIT_TP_ERROR = 0x0043,
	AACENC_INIT_META_ERROR = 0x0044,
	AACENC_INIT_MPS_ERROR = 0x0045,
	AACENC_ENCODE_ERROR = 0x0060,
	AACENC_ENCODE_EOF = 0x0080
} AACENC_ERROR;

typedef enum {
	IN_AUDIO_DATA = 0,
	IN_ANCILLRY_DATA = 1,
	IN_METADATA_SETUP = 2,
	OUT_BITSTREAM_DATA = 3,
	OUT_AU_SIZES = 4
} AACENC_BufferIdentifier;

typedef enum {
	AACENC_AOT = 0x0100,
	AACENC_BITRATE = 0x0101,
	AACENC_BITRATEMODE = 0x0102,
	AACENC_SAMPLERATE = 0x0103,
	AACENC_SBR_MODE = 0x0104,
	AACENC_GRANULE_LENGTH = 0x0105,
	AACENC_CHANNELMODE = 0x0106,
	AACENC_CHANNELORDER = 0x0107,
	AACENC_SBR_RATIO = 0x0108,
	AACENC_AFTERBURNER = 0x0200,
	AACENC_BANDWIDTH = 0x0203,
	AACENC_PEAK_BITRATE = 0x0207,
	AACENC_TRANSMUX = 0x0300,
	AACENC_HEADER_PERIOD = 0x0301,
	AACENC_SIGNALING_MODE = 0x0302,
	AACENC_TPSUBFRAMES = 0x0303,
	AACENC_AUDIOMUXVER = 0x0304,
	AACENC_PROTECTION = 0x0306,
	AACENC_ANCILLARY_BITRATE = 0x0500,
	AACENC_METADATA_MODE = 0x0600,
	AACENC_CONTROL_STATE = 0xFF00,
	AACENC_NONE = 0xFFFF
} AACENC_PARAM;

typedef enum {
	FDK_NONE = 0,
	FDK_TOOLS = 1,
	FDK_SYSLIB = 2,
	FDK_AACDEC = 3,
	FDK_AACENC = 4,
	FDK_SBRDEC = 5,
	FDK_SBRENC = 6,
	FDK_TPDEC = 7,
	FDK_TPENC = 8,
	FDK_MPSDEC = 9,
	FDK_MPEGFILEREAD = 10,
	FDK_MPEGFILEWRITE = 11,
	FDK_PCMDMX = 31,
	FDK_MPSENC = 34,
	FDK_TDLIMIT = 35,
	FDK_UNIDRCDEC = 38,
	FDK_MODULE_LAST
} FDK_MODULE_ID;

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct AACENCODER *HANDLE_AACENCODER;

typedef struct {
	uint32_t maxOutBufBytes;
	uint32_t maxAncBytes;
	uint32_t inBufFillLevel;
	uint32_t inputChannels;
	uint32_t frameLength;
	uint32_t nDelay;
	uint32_t nDelayCore;
	uint8_t confBuf[64];
	uint32_t confSize;
} AACENC_InfoStruct;

typedef struct {
	int numBufs;
	void **bufs;
	int *bufferIdentifiers;
	int *bufSizes;
	int *bufElSizes;
} AACENC_BufDesc;

typedef struct {
	int numInSamples;
	int numAncBytes;
} AACENC_InArgs;

typedef struct {
	int numOutBytes;
	int numInSamples;
	int numAncBytes;
	int bitResState;
} AACENC_OutArgs;

typedef AACENC_ERROR (*aacEncOpen_t)(HANDLE_AACENCODER *phAacEncoder, const uint32_t encModules,
				     const uint32_t maxChannels);
typedef AACENC_ERROR (*aacEncClose_t)(HANDLE_AACENCODER *phAacEncoder);
typedef AACENC_ERROR (*aacEncEncode_t)(const HANDLE_AACENCODER hAacEncoder, const AACENC_BufDesc *inBufDesc,
				       const AACENC_BufDesc *outBufDesc, const AACENC_InArgs *inargs,
				       AACENC_OutArgs *outargs);
typedef AACENC_ERROR (*aacEncInfo_t)(const HANDLE_AACENCODER hAacEncoder, AACENC_InfoStruct *pInfo);
typedef AACENC_ERROR (*aacEncoder_SetParam_t)(const HANDLE_AACENCODER hAacEncoder, const AACENC_PARAM param,
					      const uint32_t value);

static aacEncOpen_t aacEncOpen;
static aacEncClose_t aacEncClose;
static aacEncEncode_t aacEncEncode;
static aacEncInfo_t aacEncInfo;
static aacEncoder_SetParam_t aacEncoder_SetParam;

static bool load_libfdk_aac(void)
{
#if defined(_WIN32)
	const char *libfdk_aac_name_list[5] = {"libfdk-aac-2", "fdk-aac", NULL, NULL, NULL};
#elif defined(__APPLE__)
	const char *libfdk_aac_name_list[5] = {"libfdk-aac.2.dylib", "libfdk-aac.dylib", NULL, NULL, NULL};
#else
	const char *libfdk_aac_name_list[5] = {"libfdk-aac.so.2", "libfdk-aac.so", NULL, NULL, NULL};
#endif
	/* Try to load the fdk-aac dll/so from system default directories first,
	 * then from the module directory. */
	const char *dll_path = obs_get_module_binary_path(obs_current_module());
	char full_path[2][512];
	strncpy(full_path[0], dll_path, sizeof(full_path[0]));
	char *last_slash = strrchr(full_path[0], '/');
	if (last_slash) {
		*(last_slash + 1) = '\0';
		strcpy(full_path[1], full_path[0]);
		strcat(full_path[0], libfdk_aac_name_list[0]);
		strcat(full_path[1], libfdk_aac_name_list[1]);
		libfdk_aac_name_list[2] = full_path[0];
		libfdk_aac_name_list[3] = full_path[1];
	}

#define LOAD_AAC_FUNC(name)                                    \
	if (!(name = (name##_t)os_dlsym(libfdk_aac, #name))) { \
		libfdk_aac = NULL;                             \
		continue;                                      \
	}

	void *libfdk_aac = NULL;
	for (int i = 0; libfdk_aac_name_list[i]; i++) {
		libfdk_aac = os_dlopen(libfdk_aac_name_list[i]);
		if (!libfdk_aac)
			continue;
		LOAD_AAC_FUNC(aacEncOpen)
		LOAD_AAC_FUNC(aacEncClose)
		LOAD_AAC_FUNC(aacEncEncode)
		LOAD_AAC_FUNC(aacEncInfo)
		LOAD_AAC_FUNC(aacEncoder_SetParam)
		break;
	}

#undef LOAD_AAC_FUNC
	if (libfdk_aac)
		return true;
	blog(LOG_INFO, "Failed to load libfdk-aac from shared libraries.");
	return false;
}

#if defined(__cplusplus)
}
#endif
