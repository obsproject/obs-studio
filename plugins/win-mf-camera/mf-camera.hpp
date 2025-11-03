#pragma once

#include <obs-module.h>
#include <obs.h>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/CoTaskMemPtr.hpp>

#include <mfapi.h>
#include <Mfidl.h>
#include <mfcaptureengine.h>
#include <strmif.h>
#include <mfreadwrite.h>
#include <shlwapi.h>

#include <string>
#include <vector>
#include <functional>

#include <media-io/video-io.h>
#include <message-queue.hpp>

struct MFCameraDeviceInfo {
	std::string id;
	std::string name;
};

struct MFCameraResolution {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t fps_num = 0;
	uint32_t fps_den = 0;
	uint32_t aspect_ratio_num = 0;
	uint32_t aspect_ratio_denom = 0;
	MFCameraResolution() : width(0), height(0), fps_num(0), fps_den(0), aspect_ratio_num(0), aspect_ratio_denom(0)
	{
	}

	MFCameraResolution(uint32_t _width, uint32_t _height, uint32_t _fps_num, uint32_t _fps_den,
			   uint32_t _aspect_ratio_num, uint32_t _aspect_ratio_denom)
		: width(_width),
		  height(_height),
		  fps_num(_fps_num),
		  fps_den(_fps_den),
		  aspect_ratio_num(_aspect_ratio_num),
		  aspect_ratio_denom(_aspect_ratio_denom)
	{
	}
};

struct MediaFormatConfiguration {
	bool is_hardware_format;
	GUID mf_source_media_subtype;
	video_format pixel_format;
};

struct CMFMediaType {
	ComPtr<IMFMediaType> media_type;
	CMFMediaType() {}
	CMFMediaType(IMFMediaType *pType) : media_type(pType) {}
	CMFMediaType(ComPtr<IMFMediaType> pType) : media_type(pType) {}
	MFCameraResolution GetMFCameraResolution()
	{
		MFCameraResolution ret;
		MFGetAttributeSize(media_type.Get(), MF_MT_FRAME_SIZE, &ret.width, &ret.height);
		MFGetAttributeRatio(media_type.Get(), MF_MT_FRAME_RATE, &ret.fps_num, &ret.fps_den);
		MFGetAttributeRatio(media_type.Get(), MF_MT_PIXEL_ASPECT_RATIO, &ret.aspect_ratio_num,
				    &ret.aspect_ratio_denom);
		return ret;
	}

	int32_t GetWidth()
	{
		UINT32 width = 0;
		UINT32 height = 0;
		MFGetAttributeSize(media_type.Get(), MF_MT_FRAME_SIZE, &width, &height);
		return width;
	}

	int32_t GetHeight()
	{
		UINT32 width = 0;
		UINT32 height = 0;
		MFGetAttributeSize(media_type.Get(), MF_MT_FRAME_SIZE, &width, &height);
		return height;
	}

	video_format GetVideoFormat()
	{
		GUID subtype;
		media_type->GetGUID(MF_MT_SUBTYPE, &subtype);
		return ConvertMFSubTypeToVideoFormat(subtype);
	}

	static video_format ConvertMFSubTypeToVideoFormat(const GUID &subtype)
	{
		if (subtype == MFVideoFormat_RGB32) {
			return VIDEO_FORMAT_BGRX;
		}
		if (subtype == MFVideoFormat_ARGB32) {
			return VIDEO_FORMAT_BGRA;
		} else if (subtype == MFVideoFormat_I420) {
			return VIDEO_FORMAT_I420;
		} else if (subtype == MFVideoFormat_YV12) {
			return VIDEO_FORMAT_I420;
		} else if (subtype == MFVideoFormat_NV12) {
			return VIDEO_FORMAT_NV12;
		} else if (subtype == MFVideoFormat_YVYU) {
			return VIDEO_FORMAT_YVYU;
		} else if (subtype == MFVideoFormat_YUY2) {
			return VIDEO_FORMAT_YUY2;
		} else if (subtype == MFVideoFormat_UYVY) {
			return VIDEO_FORMAT_UYVY;
		} else if (subtype == MFVideoFormat_P010) {
			return VIDEO_FORMAT_P010;
		}
		return VIDEO_FORMAT_NONE;
	}

	MediaFormatConfiguration GetMediaFormatConfiguration() const
	{
		MediaFormatConfiguration ret;
		GUID subtype;
		media_type->GetGUID(MF_MT_SUBTYPE, &subtype);
		ret.is_hardware_format = false;
		ret.pixel_format = ConvertMFSubTypeToVideoFormat(subtype);
		ret.mf_source_media_subtype = subtype;
		return ret;
	}

	UINT32 GetDefaultStride()
	{
		UINT32 stride = 0;
		media_type->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32 *)&stride);
		return stride;
	}

	enum video_range_type GetColorRange()
	{
		UINT32 range = MFNominalRange_16_235;
		media_type->GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, &range);
		if (range == MFNominalRange_0_255) {
			return video_range_type::VIDEO_RANGE_FULL;
		}

		if (range == MFNominalRange_16_235) {
			return video_range_type::VIDEO_RANGE_PARTIAL;
		}

		return video_range_type::VIDEO_RANGE_DEFAULT;
	}

	video_colorspace GetColorSpace()
	{
		UINT32 primaries = MFVideoPrimaries_SMPTE170M;
		media_type->GetUINT32(MF_MT_VIDEO_PRIMARIES, &primaries);

		if (primaries == MFVideoPrimaries_SMPTE170M) {
			return video_colorspace::VIDEO_CS_601;
		}

		if (primaries == MFVideoPrimaries_BT709) {
			return video_colorspace::VIDEO_CS_709;
		}

		return video_colorspace::VIDEO_CS_DEFAULT;
	}

	bool IsFixedSizeSamples()
	{
		UINT32 isFixedSize = 0;
		media_type->GetUINT32(MF_MT_FIXED_SIZE_SAMPLES, &isFixedSize);
		return !!isFixedSize;
	}

	UINT32 GetSampleSize()
	{
		UINT32 sampleSize = 0;
		media_type->GetUINT32(MF_MT_SAMPLE_SIZE, &sampleSize);
		return sampleSize;
	}

	MFVideoInterlaceMode GetInterlaceMode()
	{
		UINT32 interlaceMode = 0;
		media_type->GetUINT32(MF_MT_INTERLACE_MODE, &interlaceMode);
		return (MFVideoInterlaceMode)interlaceMode;
	}
};

class MediaFoundationCameraSource : public IMFSourceReaderCallback {
public:
	MediaFoundationCameraSource(obs_source_t *source, obs_data_t *setting);
	~MediaFoundationCameraSource();

	void SetActive(bool active);
	void Update(obs_data_t *setting);
	bool IsActive();

	bool Open();
	bool Close();

	bool ReStart(const std::string &device_id);

	void SourceTick();

	template<typename F, typename... Fs> static void EnumMFCameraDevices(F &&f, Fs &&...fs);
	template<typename F, typename... Fs>
	static void EnumMFCameraDeviceFomatsAndResolutions(const std::string &device_id, F &&f, Fs &&...fs);
	static ComPtr<IMFAttributes> GetDefaultSourceConfig(UINT32 num);

	// IUnknown methods
	// deec8d99-fa1d-4d82-84c2-2c8969944867
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if (riid == IID_IMFSourceReaderCallback) {
			*ppvObject = this;
			return S_OK;
		} else if (riid == IID_IMFSourceReaderCallback2) {
			return S_FALSE;
		} else {
			return S_FALSE;
		}
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void) { return 1; }
	virtual ULONG STDMETHODCALLTYPE Release(void) { return 1; }

	// IMFSourceReaderCallback methods
	virtual HRESULT STDMETHODCALLTYPE OnReadSample(_In_ HRESULT hrStatus, _In_ DWORD dwStreamIndex,
						       _In_ DWORD dwStreamFlags, _In_ LONGLONG llTimestamp,
						       _In_opt_ IMFSample *pSample);

	virtual HRESULT STDMETHODCALLTYPE OnFlush(_In_ DWORD dwStreamIndex);

	virtual HRESULT STDMETHODCALLTYPE OnEvent(_In_ DWORD dwStreamIndex, _In_ IMFMediaEvent *pEvent);

	void ReadSampleInMessage(ComPtr<IMFSample> pSample);

	bool m_active = false;
	obs_source_t *m_source;
	MessageQueuePtr m_message_queue;
	ComPtr<IMFSourceReader> m_source_reader;
	CMFMediaType m_media_type;
	std::string m_current_device;
};
