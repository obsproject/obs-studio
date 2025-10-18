#include "mf-common.hpp"

#include <util/platform.h>
#include <unordered_map>
#include <mfapi.h>
#include <Mferror.h>
#include <strsafe.h>
#include <wrl/client.h>

#include <string>

using namespace std;

namespace std {
template<> struct hash<GUID> {
	std::size_t operator()(const GUID &guid) const noexcept
	{
		const uint64_t *p = reinterpret_cast<const uint64_t *>(&guid);
		return std::hash<uint64_t>()(p[0]) ^ std::hash<uint64_t>()(p[1]);
	}
};
} // namespace std

namespace {
void DBGMSG(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	WCHAR msg[MAX_PATH];

	if (SUCCEEDED(StringCbVPrintf(msg, sizeof(msg), format, args))) {
		char *cmsg;
		os_wcs_to_utf8_ptr(msg, 0, &cmsg);
		MF::MF_LOG(LOG_INFO, "%s", cmsg);
		bfree(cmsg);
	}
}

LPCWSTR GetGUIDNameConst(const GUID &guid)
{
	static const std::unordered_map<GUID, LPCWSTR, std::hash<GUID>> guidMap = {
		{MF_MT_MAJOR_TYPE, L"MF_MT_MAJOR_TYPE"},
		{MF_MT_SUBTYPE, L"MF_MT_SUBTYPE"},
		{MF_MT_ALL_SAMPLES_INDEPENDENT, L"MF_MT_ALL_SAMPLES_INDEPENDENT"},
		{MF_MT_FIXED_SIZE_SAMPLES, L"MF_MT_FIXED_SIZE_SAMPLES"},
		{MF_MT_COMPRESSED, L"MF_MT_COMPRESSED"},
		{MF_MT_SAMPLE_SIZE, L"MF_MT_SAMPLE_SIZE"},
		{MF_MT_WRAPPED_TYPE, L"MF_MT_WRAPPED_TYPE"},
		{MF_MT_AUDIO_NUM_CHANNELS, L"MF_MT_AUDIO_NUM_CHANNELS"},
		{MF_MT_AUDIO_SAMPLES_PER_SECOND, L"MF_MT_AUDIO_SAMPLES_PER_SECOND"},
		{MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, L"MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND"},
		{MF_MT_AUDIO_AVG_BYTES_PER_SECOND, L"MF_MT_AUDIO_AVG_BYTES_PER_SECOND"},
		{MF_MT_AUDIO_BLOCK_ALIGNMENT, L"MF_MT_AUDIO_BLOCK_ALIGNMENT"},
		{MF_MT_AUDIO_BITS_PER_SAMPLE, L"MF_MT_AUDIO_BITS_PER_SAMPLE"},
		{MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, L"MF_MT_AUDIO_VALID_BITS_PER_SAMPLE"},
		{MF_MT_AUDIO_SAMPLES_PER_BLOCK, L"MF_MT_AUDIO_SAMPLES_PER_BLOCK"},
		{MF_MT_AUDIO_CHANNEL_MASK, L"MF_MT_AUDIO_CHANNEL_MASK"},
		{MF_MT_AUDIO_FOLDDOWN_MATRIX, L"MF_MT_AUDIO_FOLDDOWN_MATRIX"},
		{MF_MT_AUDIO_WMADRC_PEAKREF, L"MF_MT_AUDIO_WMADRC_PEAKREF"},
		{MF_MT_AUDIO_WMADRC_PEAKTARGET, L"MF_MT_AUDIO_WMADRC_PEAKTARGET"},
		{MF_MT_AUDIO_WMADRC_AVGREF, L"MF_MT_AUDIO_WMADRC_AVGREF"},
		{MF_MT_AUDIO_WMADRC_AVGTARGET, L"MF_MT_AUDIO_WMADRC_AVGTARGET"},
		{MF_MT_AUDIO_PREFER_WAVEFORMATEX, L"MF_MT_AUDIO_PREFER_WAVEFORMATEX"},
		{MF_MT_FRAME_SIZE, L"MF_MT_FRAME_SIZE"},
		{MF_MT_FRAME_RATE, L"MF_MT_FRAME_RATE"},
		{MF_MT_FRAME_RATE_RANGE_MAX, L"MF_MT_FRAME_RATE_RANGE_MAX"},
		{MF_MT_FRAME_RATE_RANGE_MIN, L"MF_MT_FRAME_RATE_RANGE_MIN"},
		{MF_MT_PIXEL_ASPECT_RATIO, L"MF_MT_PIXEL_ASPECT_RATIO"},
		{MF_MT_DRM_FLAGS, L"MF_MT_DRM_FLAGS"},
		{MF_MT_PAD_CONTROL_FLAGS, L"MF_MT_PAD_CONTROL_FLAGS"},
		{MF_MT_SOURCE_CONTENT_HINT, L"MF_MT_SOURCE_CONTENT_HINT"},
		{MF_MT_VIDEO_CHROMA_SITING, L"MF_MT_VIDEO_CHROMA_SITING"},
		{MF_MT_INTERLACE_MODE, L"MF_MT_INTERLACE_MODE"},
		{MF_MT_TRANSFER_FUNCTION, L"MF_MT_TRANSFER_FUNCTION"},
		{MF_MT_VIDEO_PRIMARIES, L"MF_MT_VIDEO_PRIMARIES"},
		{MF_MT_CUSTOM_VIDEO_PRIMARIES, L"MF_MT_CUSTOM_VIDEO_PRIMARIES"},
		{MF_MT_YUV_MATRIX, L"MF_MT_YUV_MATRIX"},
		{MF_MT_VIDEO_LIGHTING, L"MF_MT_VIDEO_LIGHTING"},
		{MF_MT_VIDEO_NOMINAL_RANGE, L"MF_MT_VIDEO_NOMINAL_RANGE"},
		{MF_MT_GEOMETRIC_APERTURE, L"MF_MT_GEOMETRIC_APERTURE"},
		{MF_MT_MINIMUM_DISPLAY_APERTURE, L"MF_MT_MINIMUM_DISPLAY_APERTURE"},
		{MF_MT_PAN_SCAN_APERTURE, L"MF_MT_PAN_SCAN_APERTURE"},
		{MF_MT_PAN_SCAN_ENABLED, L"MF_MT_PAN_SCAN_ENABLED"},
		{MF_MT_AVG_BITRATE, L"MF_MT_AVG_BITRATE"},
		{MF_MT_AVG_BIT_ERROR_RATE, L"MF_MT_AVG_BIT_ERROR_RATE"},
		{MF_MT_MAX_KEYFRAME_SPACING, L"MF_MT_MAX_KEYFRAME_SPACING"},
		{MF_MT_DEFAULT_STRIDE, L"MF_MT_DEFAULT_STRIDE"},
		{MF_MT_PALETTE, L"MF_MT_PALETTE"},
		{MF_MT_USER_DATA, L"MF_MT_USER_DATA"},
		{MF_MT_AM_FORMAT_TYPE, L"MF_MT_AM_FORMAT_TYPE"},
		{MF_MT_MPEG_START_TIME_CODE, L"MF_MT_MPEG_START_TIME_CODE"},
		{MF_MT_VIDEO_LEVEL, L"MF_MT_VIDEO_LEVEL"},
		{MF_MT_VIDEO_PROFILE, L"MF_MT_VIDEO_PROFILE"},
		{MF_MT_MPEG2_FLAGS, L"MF_MT_MPEG2_FLAGS"},
		{MF_MT_MPEG_SEQUENCE_HEADER, L"MF_MT_MPEG_SEQUENCE_HEADER"},
		{MF_MT_DV_AAUX_SRC_PACK_0, L"MF_MT_DV_AAUX_SRC_PACK_0"},
		{MF_MT_DV_AAUX_CTRL_PACK_0, L"MF_MT_DV_AAUX_CTRL_PACK_0"},
		{MF_MT_DV_AAUX_SRC_PACK_1, L"MF_MT_DV_AAUX_SRC_PACK_1"},
		{MF_MT_DV_AAUX_CTRL_PACK_1, L"MF_MT_DV_AAUX_CTRL_PACK_1"},
		{MF_MT_DV_VAUX_SRC_PACK, L"MF_MT_DV_VAUX_SRC_PACK"},
		{MF_MT_DV_VAUX_CTRL_PACK, L"MF_MT_DV_VAUX_CTRL_PACK"},
		{MF_MT_ARBITRARY_HEADER, L"MF_MT_ARBITRARY_HEADER"},
		{MF_MT_ARBITRARY_FORMAT, L"MF_MT_ARBITRARY_FORMAT"},
		{MF_MT_IMAGE_LOSS_TOLERANT, L"MF_MT_IMAGE_LOSS_TOLERANT"},
		{MF_MT_MPEG4_SAMPLE_DESCRIPTION, L"MF_MT_MPEG4_SAMPLE_DESCRIPTION"},
		{MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY, L"MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY"},
		{MF_MT_ORIGINAL_4CC, L"MF_MT_ORIGINAL_4CC"},
		{MF_MT_ORIGINAL_WAVE_FORMAT_TAG, L"MF_MT_ORIGINAL_WAVE_FORMAT_TAG"},
		{MFMediaType_Audio, L"MFMediaType_Audio"},
		{MFMediaType_Video, L"MFMediaType_Video"},
		{MFMediaType_Protected, L"MFMediaType_Protected"},
		{MFMediaType_SAMI, L"MFMediaType_SAMI"},
		{MFMediaType_Script, L"MFMediaType_Script"},
		{MFMediaType_Image, L"MFMediaType_Image"},
		{MFMediaType_HTML, L"MFMediaType_HTML"},
		{MFMediaType_Binary, L"MFMediaType_Binary"},
		{MFMediaType_FileTransfer, L"MFMediaType_FileTransfer"},
		{MFVideoFormat_AI44, L"MFVideoFormat_AI44"},
		{MFVideoFormat_ARGB32, L"MFVideoFormat_ARGB32"},
		{MFVideoFormat_AYUV, L"MFVideoFormat_AYUV"},
		{MFVideoFormat_DV25, L"MFVideoFormat_DV25"},
		{MFVideoFormat_DV50, L"MFVideoFormat_DV50"},
		{MFVideoFormat_DVH1, L"MFVideoFormat_DVH1"},
		{MFVideoFormat_DVSD, L"MFVideoFormat_DVSD"},
		{MFVideoFormat_DVSL, L"MFVideoFormat_DVSL"},
		{MFVideoFormat_H264, L"MFVideoFormat_H264"},
		{MFVideoFormat_HEVC, L"MFVideoFormat_HEVC"},
		{MFVideoFormat_H265, L"MFVideoFormat_H265"},
		{MFVideoFormat_I420, L"MFVideoFormat_I420"},
		{MFVideoFormat_IYUV, L"MFVideoFormat_IYUV"},
		{MFVideoFormat_M4S2, L"MFVideoFormat_M4S2"},
		{MFVideoFormat_MJPG, L"MFVideoFormat_MJPG"},
		{MFVideoFormat_MP43, L"MFVideoFormat_MP43"},
		{MFVideoFormat_MP4S, L"MFVideoFormat_MP4S"},
		{MFVideoFormat_MP4V, L"MFVideoFormat_MP4V"},
		{MFVideoFormat_MPG1, L"MFVideoFormat_MPG1"},
		{MFVideoFormat_MSS1, L"MFVideoFormat_MSS1"},
		{MFVideoFormat_MSS2, L"MFVideoFormat_MSS2"},
		{MFVideoFormat_NV11, L"MFVideoFormat_NV11"},
		{MFVideoFormat_NV12, L"MFVideoFormat_NV12"},
		{MFVideoFormat_P010, L"MFVideoFormat_P010"},
		{MFVideoFormat_P016, L"MFVideoFormat_P016"},
		{MFVideoFormat_P210, L"MFVideoFormat_P210"},
		{MFVideoFormat_P216, L"MFVideoFormat_P216"},
		{MFVideoFormat_RGB24, L"MFVideoFormat_RGB24"},
		{MFVideoFormat_RGB32, L"MFVideoFormat_RGB32"},
		{MFVideoFormat_RGB555, L"MFVideoFormat_RGB555"},
		{MFVideoFormat_RGB565, L"MFVideoFormat_RGB565"},
		{MFVideoFormat_RGB8, L"MFVideoFormat_RGB8"},
		{MFVideoFormat_UYVY, L"MFVideoFormat_UYVY"},
		{MFVideoFormat_v210, L"MFVideoFormat_v210"},
		{MFVideoFormat_v410, L"MFVideoFormat_v410"},
		{MFVideoFormat_WMV1, L"MFVideoFormat_WMV1"},
		{MFVideoFormat_WMV2, L"MFVideoFormat_WMV2"},
		{MFVideoFormat_WMV3, L"MFVideoFormat_WMV3"},
		{MFVideoFormat_WVC1, L"MFVideoFormat_WVC1"},
		{MFVideoFormat_Y210, L"MFVideoFormat_Y210"},
		{MFVideoFormat_Y216, L"MFVideoFormat_Y216"},
		{MFVideoFormat_Y410, L"MFVideoFormat_Y410"},
		{MFVideoFormat_Y416, L"MFVideoFormat_Y416"},
		{MFVideoFormat_Y41P, L"MFVideoFormat_Y41P"},
		{MFVideoFormat_Y41T, L"MFVideoFormat_Y41T"},
		{MFVideoFormat_YUY2, L"MFVideoFormat_YUY2"},
		{MFVideoFormat_YV12, L"MFVideoFormat_YV12"},
		{MFVideoFormat_YVYU, L"MFVideoFormat_YVYU"},
		{MFAudioFormat_PCM, L"MFAudioFormat_PCM"},
		{MFAudioFormat_Float, L"MFAudioFormat_Float"},
		{MFAudioFormat_DTS, L"MFAudioFormat_DTS"},
		{MFAudioFormat_Dolby_AC3_SPDIF, L"MFAudioFormat_Dolby_AC3_SPDIF"},
		{MFAudioFormat_DRM, L"MFAudioFormat_DRM"},
		{MFAudioFormat_WMAudioV8, L"MFAudioFormat_WMAudioV8"},
		{MFAudioFormat_WMAudioV9, L"MFAudioFormat_WMAudioV9"},
		{MFAudioFormat_WMAudio_Lossless, L"MFAudioFormat_WMAudio_Lossless"},
		{MFAudioFormat_WMASPDIF, L"MFAudioFormat_WMASPDIF"},
		{MFAudioFormat_MSP1, L"MFAudioFormat_MSP1"},
		{MFAudioFormat_MP3, L"MFAudioFormat_MP3"},
		{MFAudioFormat_MPEG, L"MFAudioFormat_MPEG"},
		{MFAudioFormat_ADTS, L"MFAudioFormat_ADTS"}};

	auto it = guidMap.find(guid);
	return (it != guidMap.end()) ? it->second : nullptr;
}

float OffsetToFloat(const MFOffset &offset)
{
	return offset.value + (static_cast<float>(offset.fract) / 65536.0f);
}

HRESULT LogVideoArea(wstring &str, const PROPVARIANT &var)
{
	if (var.caub.cElems < sizeof(MFVideoArea)) {
		return MF_E_BUFFERTOOSMALL;
	}

	MFVideoArea *pArea = (MFVideoArea *)var.caub.pElems;

	str += L"(";
	str += to_wstring(OffsetToFloat(pArea->OffsetX));
	str += L",";
	str += to_wstring(OffsetToFloat(pArea->OffsetY));
	str += L") (";
	str += to_wstring(pArea->Area.cx);
	str += L",";
	str += to_wstring(pArea->Area.cy);
	str += L")";
	return S_OK;
}

HRESULT GetGUIDName(const GUID &guid, WCHAR **outGuidName)
{
	HRESULT hr = S_OK;
	WCHAR *pName = NULL;

	LPCWSTR guidNameConst = GetGUIDNameConst(guid);
	if (guidNameConst) {
		size_t cchLength = 0;

		hr = StringCchLength(guidNameConst, STRSAFE_MAX_CCH, &cchLength);
		if (FAILED(hr)) {
			goto done;
		}

		pName = (WCHAR *)CoTaskMemAlloc((cchLength + 1) * sizeof(WCHAR));

		if (pName == NULL) {
			hr = E_OUTOFMEMORY;
			goto done;
		}

		hr = StringCchCopy(pName, cchLength + 1, guidNameConst);
		if (FAILED(hr)) {
			goto done;
		}
	} else {
		hr = StringFromCLSID(guid, &pName);
	}

done:
	if (FAILED(hr)) {
		*outGuidName = NULL;
		CoTaskMemFree(pName);
	} else {
		*outGuidName = pName;
	}
	return hr;
}

void LogUINT32AsUINT64(wstring &str, const PROPVARIANT &var)
{
	UINT32 uHigh = 0, uLow = 0;
	Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);
	str += to_wstring(uHigh);
	str += L" x ";
	str += to_wstring(uLow);
}

// Handle certain known special cases.
HRESULT SpecialCaseAttributeValue(wstring &str, GUID guid, const PROPVARIANT &var)
{
	if ((guid == MF_MT_FRAME_RATE) || (guid == MF_MT_FRAME_RATE_RANGE_MAX) ||
	    (guid == MF_MT_FRAME_RATE_RANGE_MIN) || (guid == MF_MT_FRAME_SIZE) || (guid == MF_MT_PIXEL_ASPECT_RATIO)) {

		// Attributes that contain two packed 32-bit values.
		LogUINT32AsUINT64(str, var);

	} else if ((guid == MF_MT_GEOMETRIC_APERTURE) || (guid == MF_MT_MINIMUM_DISPLAY_APERTURE) ||
		   (guid == MF_MT_PAN_SCAN_APERTURE)) {

		// Attributes that an MFVideoArea structure.
		return LogVideoArea(str, var);

	} else {
		return S_FALSE;
	}
	return S_OK;
}

HRESULT LogAttributeValueByIndex(IMFAttributes *pAttr, DWORD index)
{
	wstring str;

	WCHAR *pGuidName = NULL;
	WCHAR *pGuidValName = NULL;

	GUID guid = {0};

	PROPVARIANT var;
	PropVariantInit(&var);

	HRESULT hr = pAttr->GetItemByIndex(index, &guid, &var);
	if (FAILED(hr)) {
		goto done;
	}

	hr = GetGUIDName(guid, &pGuidName);
	if (FAILED(hr)) {
		goto done;
	}

	str += L"    ";
	str += pGuidName;
	str += L": ";

	hr = SpecialCaseAttributeValue(str, guid, var);
	if (FAILED(hr)) {
		goto done;
	}

	if (hr == S_FALSE) {
		switch (var.vt) {
		case VT_UI4:
			str += to_wstring(var.ulVal);
			break;

		case VT_UI8:
			str += to_wstring(var.uhVal.QuadPart);
			break;

		case VT_R8:
			str += to_wstring(var.dblVal);
			break;

		case VT_CLSID:
			hr = GetGUIDName(*var.puuid, &pGuidValName);
			if (SUCCEEDED(hr)) {
				str += pGuidValName;
			}
			break;

		case VT_LPWSTR:
			str += var.pwszVal;
			break;

		case VT_VECTOR | VT_UI1:
			str += L"<<byte array>>";
			break;

		case VT_UNKNOWN:
			str += L"IUnknown";
			break;

		default:
			str += L"Unexpected attribute type (vt = ";
			str += to_wstring(var.vt);
			str += L")";
			break;
		}
	}

	DBGMSG(L"%s", str.c_str());

done:
	CoTaskMemFree(pGuidName);
	CoTaskMemFree(pGuidValName);
	PropVariantClear(&var);
	return hr;
}
} // namespace

void MF::MF_LOG(int level, const char *format, ...)
{
	constexpr size_t buf_size = 1024;
	char buffer[buf_size];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, buf_size, format, args);
	va_end(args);

	blog(level, "[Media Foundation encoder]: %s", buffer);
}

void MF::MF_LOG_COM(int level, const char *msg, HRESULT hr)
{
	MF_LOG(level, "%s failed, %S (0x%08lx)", msg, _com_error(hr).ErrorMessage(), hr);
}

bool MF::LogMediaType(IMFMediaType *pType)
{
	UINT32 count = 0;

	HRESULT hr = pType->GetCount(&count);
	if (FAILED(hr)) {
		return false;
	}

	if (count == 0) {
		DBGMSG(L"Empty media type.");
	}

	for (UINT32 i = 0; i < count; i++) {
		hr = LogAttributeValueByIndex(pType, i);
		if (FAILED(hr)) {
			return false;
		}
	}
	return true;
}
