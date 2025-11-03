#include "mf-camera.hpp"
#include <memory>
#include <mfreadwrite.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
/* clang-format off */

/* settings defines that will cause errors if there are typos */
#define VIDEO_DEVICE_ID   "video_device_id"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define ACTIVATE          "activate"


#define TEXT_INPUT_NAME     obs_module_text("MediaFoundationVideoCaptureDevice")
#define TEXT_DEVICE         obs_module_text("Device")
#define TEXT_ACTIVATE       obs_module_text("Activate")
#define TEXT_DEACTIVATE     obs_module_text("Deactivate")

/* clang-format on */

static std::string VideoForamtToString(video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_NONE:
		return "NONE";
	case VIDEO_FORMAT_I420:
		return "I420";
	case VIDEO_FORMAT_NV12:
		return "NV12";
	case VIDEO_FORMAT_YVYU:
		return "YVYU";
	case VIDEO_FORMAT_YUY2:
		return "YUY2";
	case VIDEO_FORMAT_UYVY:
		return "UYVY";
	case VIDEO_FORMAT_RGBA:
		return "RGBA";
	case VIDEO_FORMAT_BGRA:
		return "BGRA";
	case VIDEO_FORMAT_BGRX:
		return "BGRX";
	case VIDEO_FORMAT_Y800:
		return "Y800";
	case VIDEO_FORMAT_I444:
		return "I444";
	case VIDEO_FORMAT_BGR3:
		return "BGR3";
	case VIDEO_FORMAT_I422:
		return "I422";
	case VIDEO_FORMAT_I40A:
		return "I40A";
	case VIDEO_FORMAT_I42A:
		return "I42A";
	case VIDEO_FORMAT_YUVA:
		return "YUVA";
	case VIDEO_FORMAT_AYUV:
		return "AYUV";
	case VIDEO_FORMAT_I010:
		return "I010";
	case VIDEO_FORMAT_P010:
		return "P010";
	case VIDEO_FORMAT_I210:
		return "I210";
	case VIDEO_FORMAT_I412:
		return "I412";
	case VIDEO_FORMAT_YA2L:
		return "YA2L";
	case VIDEO_FORMAT_P216:
		return "P216";
	case VIDEO_FORMAT_P416:
		return "P416";
	case VIDEO_FORMAT_V210:
		return "V210";
	case VIDEO_FORMAT_R10L:
		return "R10L";
	}

	return "NONE";
}

MediaFoundationCameraSource::MediaFoundationCameraSource(obs_source_t *source, obs_data_t *settings) : m_source(source)
{
	std::ostringstream ss;
	ss << "MediaFoundationCameraSource:" << this;
	m_message_queue = std::make_shared<NormalMessageQueue>(ss.str().c_str());

	if (obs_data_get_bool(settings, "active")) {
		std::string device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
		m_active = true;
		ReStart(device_id);
	}
}

MediaFoundationCameraSource::~MediaFoundationCameraSource()
{
	Close();
	m_message_queue->Close();
}

void MediaFoundationCameraSource::SetActive(bool _active)
{
	obs_data_t *settings = obs_source_get_settings(m_source);
	obs_data_set_bool(settings, "active", _active);
	obs_data_release(settings);

	MessageImplPtr pMessage = MessageImpl::createMessage(
		[this](bool isActive) {
			if (isActive) {
				Open();
			} else {
				Close();
			}
		},
		_active);
	m_message_queue->Invoke(pMessage);
}

void MediaFoundationCameraSource::Update(obs_data_t *settings)
{
	std::string device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	std::string old_device_id = obs_data_get_string(settings, LAST_VIDEO_DEV_ID);

	if (old_device_id != device_id) {
		ReStart(device_id);
	}
}

bool MediaFoundationCameraSource::IsActive()
{
	return m_active;
}

static std::wstring QueryActivationObjectString(DStr &str, IMFActivate *activation, const GUID &pguid)
{
	std::wstring wstrRet = L"";
	LPWSTR wstr = nullptr;
	UINT32 wlen = 0;
	HRESULT ret = activation->GetAllocatedString(pguid, &wstr, &wlen);
	if (FAILED(ret)) {
		return L"";
	}
	if (wstr != nullptr) {
		wstrRet = wstr;
	}

	dstr_from_wcs(str, wstr);
	CoTaskMemFree(wstr);

	return wstrRet;
}

template<typename F, typename... Fs> void MediaFoundationCameraSource::EnumMFCameraDevices(F &&f, Fs &&...fs)
{
	HRESULT hr;

	ComPtr<IMFAttributes> attrs = nullptr;
	if (FAILED(MFCreateAttributes(&attrs, 1))) {
		return;
	}

	hr = attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr)) {
		return;
	}

	IMFActivate **activations = nullptr;
	UINT32 total = 0;
	hr = MFEnumDeviceSources(attrs, &activations, &total);
	if (FAILED(hr)) {
		return;
	}

	for (UINT32 i = 0; i < total; i++) {
		DStr symbol_link;
		std::wstring device_id = QueryActivationObjectString(
			symbol_link, activations[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
		DStr name;
		QueryActivationObjectString(name, activations[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
		MFCameraDeviceInfo info;
		if (symbol_link->array && name->array) {
			info.id = symbol_link;
			info.name = name;
		}
		if (!std::bind(std::forward<F>(f), activations[i], info, std::forward<Fs>(fs)...)()) {
			activations[i]->Release();
			break;
		}
		activations[i]->Release();
	}
}

template<typename F, typename... Fs>
void MediaFoundationCameraSource::EnumMFCameraDeviceFomatsAndResolutions(const std::string &device_id, F &&f,
									 Fs &&...fs)
{
	HRESULT hr;

	ComPtr<IMFAttributes> attrs = nullptr;
	if (FAILED(MFCreateAttributes(&attrs, 1))) {
		return;
	}

	hr = attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr)) {
		return;
	}

	wchar_t w_device_id[512] = L"";
	os_utf8_to_wcs(device_id.c_str(), device_id.size(), &w_device_id[0], 512);
	attrs->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, w_device_id);

	ComPtr<IMFMediaSource> source;
	hr = MFCreateDeviceSource(attrs.Get(), &source);
	if (!source) {
		return;
	}

	ComPtr<IMFSourceReader> reader;

	hr = MFCreateSourceReaderFromMediaSource(source.Get(), attrs.Get(), &reader);

	if (!reader) {
		return;
	}
	DWORD stream_index = 0;
	ComPtr<IMFMediaType> type;
	while (SUCCEEDED(hr = reader->GetNativeMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
							 stream_index, &type))) {
		UINT32 width, height;
		HRESULT hrResize = MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, &width, &height);

		UINT32 numerator, denominator;
		HRESULT hrFPS = MFGetAttributeRatio(type.Get(), MF_MT_FRAME_RATE, &numerator, &denominator);

		GUID type_guid;
		HRESULT hrMediaType = type->GetGUID(MF_MT_SUBTYPE, &type_guid);
		if (FAILED(hrResize) || FAILED(hrFPS) || FAILED(hrMediaType)) {
			++stream_index;
			continue;
		}

		++stream_index;

		if (!std::bind(std::forward<F>(f), type, std::forward<Fs>(fs)...)()) {
			break;
		}
	}
}

ComPtr<IMFAttributes> MediaFoundationCameraSource::GetDefaultSourceConfig(UINT32 num)
{
	ComPtr<IMFAttributes> res;
	if (FAILED(MFCreateAttributes(&res, num)) ||
	    FAILED(res->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, true)) ||
	    FAILED(res->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, false)) ||
	    FAILED(res->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, false)) ||
	    FAILED(res->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, false))) {
		return nullptr;
	}
	return res;
}

bool MediaFoundationCameraSource::Open()
{
	ComPtr<IMFAttributes> attr = GetDefaultSourceConfig(1);
	ComPtr<IMFSourceReaderCallback> cb = this;
	attr->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, cb.Get());

	ComPtr<IMFMediaSource> source;
	MFCameraDeviceInfo device_info;
	MediaFoundationCameraSource::EnumMFCameraDevices(
		[this, &source, &device_info](IMFActivate *activate, const MFCameraDeviceInfo &info) -> bool {
			if (info.id != "" && info.name != "") {
				activate->ActivateObject(__uuidof(IMFMediaSource), (void **)(&source));
				if (m_current_device == "") {
					device_info = info;
					m_current_device = info.id;
					return false;
				}

				if (m_current_device == info.id) {
					device_info = info;
					return false;
				}
			}
			return true;
		});

	if (!source) {
		return false;
	}

	ComPtr<IMFSourceReader> reader;
	MFCreateSourceReaderFromMediaSource(source.Get(), attr.Get(), &reader);

	if (FAILED(reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, false))) {
		return false;
	}

	if (FAILED(reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, true))) {
		return false;
	}

	ComPtr<IMFMediaType> mediaTypesOut;
	MediaFoundationCameraSource::EnumMFCameraDeviceFomatsAndResolutions(
		device_info.id, [&mediaTypesOut](const ComPtr<IMFMediaType> &pType) -> bool {
			CMFMediaType mt(pType);
			auto format = mt.GetMediaFormatConfiguration();
			if (format.pixel_format != VIDEO_FORMAT_NONE) {
				mediaTypesOut = pType;
				return false;
			}
			return true;
		});
	HRESULT hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, mediaTypesOut.Get());

	m_source_reader = reader;
	m_active = true;
	m_media_type.media_type = mediaTypesOut;
	if (SUCCEEDED(hr)) {
		hr = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
	}
	return true;
}

void MediaFoundationCameraSource::SourceTick()
{
	MessageImplPtr pMessage = MessageImpl::createMessage(
		[this](bool isActive) {
			if (isActive) {
				if (m_source_reader) {
					m_source_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL,
								    NULL, NULL);
				}
			} else {
			}
		},
		m_active);
	m_message_queue->Invoke(pMessage);
}

bool MediaFoundationCameraSource::Close()
{
	if (m_source_reader) {
		m_source_reader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
		m_source_reader.Release();
	}

	m_active = false;
	return true;
}

bool MediaFoundationCameraSource::ReStart(const std::string &device_id)
{
	MessageImplPtr pMessage = MessageImpl::createMessage(
		[this, device_id](bool isActive) {
			m_current_device = device_id;
			if (isActive) {
				Close();
				Open();
			} else {
				Close();
			}
		},
		m_active);
	m_message_queue->Invoke(pMessage);
	return true;
}

HRESULT STDMETHODCALLTYPE MediaFoundationCameraSource::OnReadSample(_In_ HRESULT hrStatus, _In_ DWORD dwStreamIndex,
								    _In_ DWORD dwStreamFlags, _In_ LONGLONG llTimestamp,
								    _In_opt_ IMFSample *pSample)
{
	if (SUCCEEDED(hrStatus)) {
		if (pSample) {
			ComPtr<IMFSample> temp(pSample);
			MessageImplPtr pMessage = MessageImpl::createMessage(
				[this](ComPtr<IMFSample> temp) { ReadSampleInMessage(temp); }, temp);

			m_message_queue->Invoke(pMessage);
		}
	}
	return S_OK;
}

void MediaFoundationCameraSource::ReadSampleInMessage(ComPtr<IMFSample> pSample)
{
	if (!pSample) {
		return;
	}

	ComPtr<IMFMediaBuffer> media_buffer = nullptr;
	pSample->ConvertToContiguousBuffer(&media_buffer);

	if (!media_buffer) {
		return;
	}

	video_colorspace cs = m_media_type.GetColorSpace();
	const enum video_range_type range = m_media_type.GetColorRange();

	enum video_trc trc = VIDEO_TRC_DEFAULT;
	switch (cs) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_601:
	case VIDEO_CS_709:
	case VIDEO_CS_SRGB:
		trc = VIDEO_TRC_SRGB;
		break;
	case VIDEO_CS_2100_PQ:
		trc = VIDEO_TRC_PQ;
		break;
	case VIDEO_CS_2100_HLG:
		trc = VIDEO_TRC_HLG;
	}

	obs_source_frame2 frame;
	frame.format = m_media_type.GetVideoFormat();

	bool success = video_format_get_parameters_for_format(cs, range, frame.format, frame.color_matrix,
							      frame.color_range_min, frame.color_range_max);

	frame.range = range;
	frame.trc = trc;

	frame.timestamp = os_gettime_ns();
	frame.width = m_media_type.GetWidth();
	frame.height = m_media_type.GetHeight();

	frame.flags = 0;
	frame.flip = false;

	ComPtr<IMF2DBuffer2> buffer2d2 = NULL;
	BYTE *data = NULL;
	LONG pitch = 0;
	DWORD buflen = 0;
	media_buffer->QueryInterface(IID_IMF2DBuffer2, (void **)&buffer2d2);

	if (!buffer2d2) {
		return;
	}

	BYTE *bufstart = NULL;
	buffer2d2->Lock2DSize(MF2DBuffer_LockFlags_Read, &data, &pitch, &bufstart, &buflen);

	int32_t cy_abs = std::abs((int32_t)frame.height);
	if (frame.format == VIDEO_FORMAT_BGRX || frame.format == VIDEO_FORMAT_BGRA) {
		frame.data[0] = data;
		frame.linesize[0] = pitch * 4;

	} else if (frame.format == VIDEO_FORMAT_YVYU || frame.format == VIDEO_FORMAT_YUY2 ||
		   frame.format == VIDEO_FORMAT_UYVY) {
		frame.data[0] = data;
		frame.linesize[0] = pitch * 2;

	} else if (frame.format == VIDEO_FORMAT_I420) {
		frame.data[0] = data;
		frame.data[1] = frame.data[0] + (pitch * cy_abs);
		frame.data[2] = frame.data[1] + (pitch * cy_abs / 4);
		frame.linesize[0] = pitch;
		frame.linesize[1] = pitch / 2;
		frame.linesize[2] = pitch / 2;

	} else if (frame.format == VIDEO_FORMAT_NV12) {
		frame.data[0] = data;
		frame.data[1] = frame.data[0] + (pitch * cy_abs);
		frame.linesize[0] = pitch;
		frame.linesize[1] = pitch;

	} else if (frame.format == VIDEO_FORMAT_Y800) {
		frame.data[0] = data;
		frame.linesize[0] = pitch;

	} else if (frame.format == VIDEO_FORMAT_P010) {
		frame.data[0] = data;
		frame.data[1] = frame.data[0] + (pitch * cy_abs) * 2;
		frame.linesize[0] = pitch * 2;
		frame.linesize[1] = pitch * 2;

	} else {
	}

	obs_source_output_video2(m_source, &frame);

	buffer2d2->Unlock2D();
}

HRESULT STDMETHODCALLTYPE MediaFoundationCameraSource::OnFlush(_In_ DWORD dwStreamIndex)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE MediaFoundationCameraSource::OnEvent(_In_ DWORD dwStreamIndex, _In_ IMFMediaEvent *pEvent)
{
	return S_OK;
}

static const char *GetMediaFoundationInputName(void *)
{
	return TEXT_INPUT_NAME;
}

static void *CreateMediaFoundationDeviceInput(obs_data_t *settings, obs_source_t *source)
{
	MFStartup(MF_VERSION, 0);
	return new MediaFoundationCameraSource(source, settings);
}

static void DestroyMediaFoundationSource(void *obj)
{
	delete static_cast<MediaFoundationCameraSource *>(obj);
	MFShutdown();
}

static void GetMediaFoundationDefaultsInput(obs_data_t *settings) {}

static void UpdateMediaFoundationSource(void *obj, obs_data_t *settings)
{
	static_cast<MediaFoundationCameraSource *>(obj)->Update(settings);
}

static bool ActivateClicked(obs_properties_t *, obs_property_t *p, void *data)
{
	MediaFoundationCameraSource *input = reinterpret_cast<MediaFoundationCameraSource *>(data);

	if (input->m_active) {
		input->SetActive(false);
		obs_property_set_description(p, TEXT_ACTIVATE);
	} else {
		input->SetActive(true);
		obs_property_set_description(p, TEXT_DEACTIVATE);
	}

	return true;
}

void VideoTick(void *data, float seconds)
{
	MediaFoundationCameraSource *input = reinterpret_cast<MediaFoundationCameraSource *>(data);
	if (input->m_active) {
		input->SourceTick();
	}
}

static obs_properties_t *GetMediaFoundationPropertiesInput(void *obj)
{
	MediaFoundationCameraSource *input = static_cast<MediaFoundationCameraSource *>(obj);
	obs_properties_t *ppts = obs_properties_create();
	std::vector<MFCameraDeviceInfo> devices;

	obs_property_t *p = obs_properties_add_list(ppts, VIDEO_DEVICE_ID, TEXT_DEVICE, OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_STRING);

	MediaFoundationCameraSource::EnumMFCameraDevices(
		[&devices](IMFActivate *activate, const MFCameraDeviceInfo &info) -> bool {
			if (info.id != "" && info.name != "") {
				devices.emplace_back(info);
			}
			return true;
		});

	for (size_t i = 0; i < devices.size(); i++) {
		MFCameraDeviceInfo &device = devices[i];
		obs_property_list_add_string(p, device.name.c_str(), device.id.c_str());
	}

	bool isActive = false;
	auto message = MessageImpl::createMessage(
		std::bind([&isActive, input]() { input->IsActive() ? isActive = true : isActive = false; }));

	message->setSyncMessage(true);
	input->m_message_queue->Invoke(message);

	const char *activateText = TEXT_ACTIVATE;
	if (isActive)
		activateText = TEXT_DEACTIVATE;

	p = obs_properties_add_button(ppts, ACTIVATE, activateText, ActivateClicked);
	return ppts;
}

void RegisterMediaFoundationInput()
{
	obs_source_info info = {};
	info.id = "media_foundation_camea_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.video_tick = VideoTick;
	info.get_name = GetMediaFoundationInputName;
	info.create = CreateMediaFoundationDeviceInput;
	info.destroy = DestroyMediaFoundationSource;
	info.update = UpdateMediaFoundationSource;
	info.get_defaults = GetMediaFoundationDefaultsInput;
	info.get_properties = GetMediaFoundationPropertiesInput;
	info.icon_type = OBS_ICON_TYPE_CAMERA;
	obs_register_source(&info);
}
