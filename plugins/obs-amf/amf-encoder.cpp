#include "AMF/include/components/VideoEncoderVCE.h"
#include "AMF/include/components/VideoEncoderHEVC.h"
#include "amf-encoder.hpp"
#include <obs-avc.h>
#include "obs-amf.hpp"
#include "AMF/include/components/VideoConverter.h"

#define INITGUID
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <thread>
#include <chrono>
#define PTS_PROP L"PtsProp"
#define NOPTS_VALUE ((int64_t)UINT64_C(0x8000000000000000))

#include <iostream>
#include <fstream>

static ID3D11Texture2D *create_texture(struct amf_data *enc,
				       const D3D11_TEXTURE2D_DESC &textureDesc)
{
	ATL::CComPtr<ID3D11Device> device = enc->pD3D11Device;
	ID3D11Texture2D *tex;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = textureDesc.Width;
	desc.Height = textureDesc.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = textureDesc.Format;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;

	hr = device->CreateTexture2D(&desc, NULL, &tex);
	if (FAILED(hr)) {
		AMF_LOG_ERROR("Failed to create texture");
		return nullptr;
	}

	return tex;
}

void log_amf_properties(amf::AMFPropertyStorage *component)
{
	AMF_RESULT res;

	size_t count = component->GetPropertyCount();
	AMF_LOG(LOG_INFO, "Encoder properties log start.");
	AMF_LOG(LOG_INFO, "Properties count = %d", count);
	int i;
	amf::AMFVariantStruct var;
	res = AMFVariantInit(&var);

	for (i = 0; i < count; ++i) {
		wchar_t *name = new wchar_t[100];
		amf_size nameSize = 40;

		res = component->GetPropertyAt(i, name, nameSize, &var);

		switch (var.type) {
		case amf::AMF_VARIANT_BOOL:
			AMF_LOG(LOG_INFO, "%S %s", name,
				var.boolValue == true ? "true" : "false");
			break;
		case amf::AMF_VARIANT_INT64:
			AMF_LOG(LOG_INFO, "%S %d", name, var.int64Value);
			break;
		case amf::AMF_VARIANT_DOUBLE:
			AMF_LOG(LOG_INFO, "%S %f", name, var.doubleValue);
			break;
		case amf::AMF_VARIANT_STRING:
			AMF_LOG(LOG_INFO, "%S %s", name, var.stringValue);
			break;
		case amf::AMF_VARIANT_WSTRING:
			AMF_LOG(LOG_INFO, "%S %S", name, var.wstringValue);
			break;
		case amf::AMF_VARIANT_RATE:
			AMF_LOG(LOG_INFO, "%S %d:%d", name, var.rateValue.num,
				var.rateValue.den);
			break;
		case amf::AMF_VARIANT_RATIO:
			AMF_LOG(LOG_INFO, "%S %d:%d", name, var.ratioValue.num,
				var.ratioValue.den);
			break;
		case amf::AMF_VARIANT_SIZE:
			AMF_LOG(LOG_INFO, "%S %d - %d", name,
				var.sizeValue.width, var.sizeValue.height);
			break;
		case amf::AMF_VARIANT_POINT:
			AMF_LOG(LOG_INFO, "%S %d - %d", name, var.pointValue.x,
				var.pointValue.y);
			break;
		case amf::AMF_VARIANT_COLOR:
			AMF_LOG(LOG_INFO, "%S %d.%d.%d.%d", name,
				var.colorValue.r, var.colorValue.g,
				var.colorValue.b, var.colorValue.a);
			break;
		default:
			AMF_LOG(LOG_INFO, "%S %s", name, "type = AMFInterface");
			break;
		}
	}
	AMF_LOG(LOG_INFO, "Encoder properties log end.");
}

AMF_RESULT init_d3d11(obs_data_t *settings, struct amf_data *enc)
{
	ATL::CComPtr<IDXGIFactory> pFactory;
	ATL::CComPtr<ID3D11Device> pD3D11Device;
	ATL::CComPtr<ID3D11DeviceContext> pD3D11Context;
	ATL::CComPtr<IDXGIAdapter> pAdapter;
	HRESULT hr;
	uint32_t adapterIndex = 0;
	DXGI_ADAPTER_DESC desc;

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void **)(&pFactory));
	if (FAILED(hr)) {
		AMF_LOG_WARNING("CreateDXGIFactory1 failed");
		return AMF_FAIL;
	}
	obs_video_info info;
	if (obs_get_video_info(&info)) {
		adapterIndex = info.adapter;
	}
	hr = pFactory->EnumAdapters(adapterIndex, &pAdapter);
	if (FAILED(hr)) {
		AMF_LOG_WARNING("EnumAdapters failed");
		return AMF_FAIL;
	}

	pAdapter->GetDesc(&desc);

	if (desc.VendorId != 0x1002) {
		AMF_LOG_WARNING("D3D11CreateDevice failed. Invalid vendor.");
		return AMF_FAIL;
	}

	hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL,
			       0, D3D11_SDK_VERSION, &pD3D11Device, NULL,
			       &pD3D11Context);
	if (FAILED(hr)) {
		AMF_LOG_WARNING("D3D11CreateDevice failed");
		return AMF_FAIL;
	}

	enc->pD3D11Device = pD3D11Device;
	enc->pD3D11Context = pD3D11Context;
	return AMF_OK;
}

ID3D11Texture2D *get_tex_from_handle(amf_data *enc, uint32_t handle,
				     IDXGIKeyedMutex **km_out)
{
	ATL::CComPtr<ID3D11Device> device = enc->pD3D11Device;
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

	hr = device->OpenSharedResource((HANDLE)(uintptr_t)handle,
					__uuidof(ID3D11Resource),
					(void **)(&input_tex));
	if (FAILED(hr)) {
		AMF_LOG_ERROR("OpenSharedResource failed");
		return NULL;
	}

	hr = input_tex->QueryInterface(_uuidof(IDXGIKeyedMutex), (void **)&km);
	if (FAILED(hr)) {
		AMF_LOG_ERROR("QueryInterface(IDXGIKeyedMutex) failed");
		input_tex->Release();
		return NULL;
	}

	input_tex->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);
	*km_out = km;
	struct handle_tex new_ht = {handle, input_tex, km};
	da_push_back(enc->input_textures, &new_ht);
	return input_tex;
}

bool amf_data_to_encoder_packet(void *encData, amf::AMFDataPtr &data,
				struct encoder_packet *packet)
{
	if (data == nullptr)
		return true;
	struct amf_data *enc = (amf_data *)encData;

	enc->packet_data = amf::AMFBufferPtr(data);
	auto clk_start = std::chrono::high_resolution_clock::now();

	// Timestamps
	packet->type = OBS_ENCODER_VIDEO;
	/// Present Timestamp
	data->GetProperty(AMF_PRESENT_TIMESTAMP, &packet->pts);
	/// Decode Timestamp
	packet->dts =
		(int64_t)round((double_t)data->GetPts() / enc->timestamp_step);
	/// Data
	if (enc->codec == amf_data::CODEC_ID_HEVC) {
		uint64_t pktType;
		data->GetProperty(AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE,
				  &pktType);
		switch ((AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_ENUM)pktType) {
		case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_IDR:
			packet->keyframe = true;
			break;
		case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_I:
			packet->keyframe = true;
			packet->priority = 1;
			break;
		case AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE_P:
			packet->priority = 0;
			break;
		}
	} else {
		uint64_t pktType;
		data->GetProperty(AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE, &pktType);
		switch ((AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_ENUM)pktType) {
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_IDR:
			packet->keyframe = true;
			break;
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_I:
			packet->priority = 3;
			break;
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_P:
			packet->priority = 2;
			break;
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_B:
			packet->priority = 0;
			break;
		}
	}

	packet->data = (uint8_t *)enc->packet_data->GetNative();
	packet->size = enc->packet_data->GetSize();
	packet->type = OBS_ENCODER_VIDEO;

	return true;
}

bool amf_encode_tex(void *data, uint32_t handle, int64_t pts, uint64_t lock_key,
		    uint64_t *next_key, encoder_packet *packet,
		    bool *received_packet)
{
	AMF_RESULT res = AMF_FAIL;
	AMFSurfacePtr surface;
	AMFDataPtr pData = NULL;
	AMFDataPtr pConvData = NULL;
	AMFDataPtr pOutData = NULL;
	struct amf_data *enc = (amf_data *)data;
	CComPtr<ID3D11Texture2D> input_tex;
	CComPtr<ID3D11Texture2D> output_tex;
	IDXGIKeyedMutex *km;
	ATL::CComPtr<ID3D11DeviceContext> context = enc->pD3D11Context;

	if (handle == GS_INVALID_HANDLE) {
		AMF_LOG_WARNING("Encode failed: bad texture handle");
		*next_key = lock_key;
		return false;
	}

	static const GUID AMFTextureArrayIndexGUID = {0x28115527,
						      0xe7c3,
						      0x4b66,
						      {0x99, 0xd3, 0x4f, 0x2a,
						       0xe6, 0xb4, 0x7f, 0xaf}};
	input_tex = get_tex_from_handle(enc, handle, &km);
	if (!enc->texture) {
		D3D11_TEXTURE2D_DESC textureDesc;
		input_tex->GetDesc(&textureDesc);
		enc->texture = create_texture(enc, textureDesc);
	}
	output_tex = enc->texture;
	km->AcquireSync(lock_key, INFINITE);
	context->CopyResource((ID3D11Resource *)output_tex.p,
			      (ID3D11Resource *)input_tex.p);
	context->Flush();
	km->ReleaseSync(*next_key);

	res = enc->context->CreateSurfaceFromDX11Native(output_tex.p, &surface,
							NULL);

	int64_t tsLast = (int64_t)round((pts - 1) * enc->timestamp_step);
	int64_t tsNow = (int64_t)round(pts * enc->timestamp_step);

	surface->SetPts(tsNow);
	surface->SetProperty(AMF_PRESENT_TIMESTAMP, pts);
	surface->SetDuration(tsNow - tsLast);

	if (res != AMF_OK) {
		AMF_LOG_ERROR(
			"CreateSurfaceFromDX11Native() failed  with error %d\n",
			res);
		return false;
	}

	bool scaling = obs_encoder_scaling_enabled(enc->encoder);

	if (scaling) {
		res = enc->converter_amf->SubmitInput(surface);
		if (res != AMF_OK) {
			AMF_LOG_ERROR(
				"Failed to SubmitInput(converter) with error  %ls\n",
				AMF::Instance()->GetTrace()->GetResultText(
					res));
		}
		while (true) {
			res = enc->converter_amf->QueryOutput(&pConvData);
			if (res == AMF_INPUT_FULL)
				std::this_thread::sleep_for(
					enc->query_wait_time);
			else
				break;
		}
		res = enc->encoder_amf->SubmitInput(pConvData);
		if (res != AMF_OK) {
			AMF_LOG_ERROR(
				"Failed to SubmitInput with error  %ls\n",
				AMF::Instance()->GetTrace()->GetResultText(
					res));
			*received_packet = false;
			return false;
		}
	} else { // !scaling
		res = enc->encoder_amf->SubmitInput(surface);
		if (res != AMF_OK) {
			AMF_LOG_ERROR(
				"Failed to SubmitInput with error %ls\n",
				AMF::Instance()->GetTrace()->GetResultText(
					res));
			*received_packet = false;
			return false;
		}
	}

	while (true) {
		res = enc->encoder_amf->QueryOutput(&pOutData);
		if (res == AMF_OK)
			break;
		switch (res) {
		case AMF_NEED_MORE_INPUT: {
			*received_packet = false;
			return true;
		}
		case AMF_REPEAT: {
			break;
		}
		default: {
			AMF_LOG_WARNING(
				"Failed to QueryOutput  with code: %ls\n",
				AMF::Instance()->GetTrace()->GetResultText(
					res));
			break;
		}
		}

		std::this_thread::sleep_for(enc->query_wait_time);
	}
	*received_packet = true;
	return amf_data_to_encoder_packet(data, pOutData, packet);
}

bool amf_encode(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet)
{
	AMF_RESULT res;

	AMFSurfacePtr surface;
	AMFDataPtr pData = NULL;
	AMFDataPtr pConvData = NULL;
	AMFDataPtr pOutData = NULL;
	struct amf_data *enc = (amf_data *)data;

	// Encoding Steps
	res = enc->context->AllocSurface(amf::AMF_MEMORY_HOST,
					 enc->in_surface_format,
					 enc->convertor_frame_w,
					 enc->convertor_frame_h, &surface);

	size_t planeCount = surface->GetPlanesCount();
	for (uint8_t i = 0; i < planeCount; i++) {
		amf::AMFPlanePtr plane = surface->GetPlaneAt(i);
		int32_t width = plane->GetWidth();
		int32_t height = plane->GetHeight();
		int32_t hpitch = plane->GetHPitch();

		void *plane_nat = plane->GetNative();
		for (int32_t py = 0; py < height; py++) {
			int32_t plane_off = py * hpitch;
			int32_t frame_off = py * frame->linesize[i];
			std::memcpy(
				static_cast<void *>(
					static_cast<uint8_t *>(plane_nat) +
					plane_off),
				static_cast<void *>(frame->data[i] + frame_off),
				frame->linesize[i]);
		}
	}

	int64_t tsLast = (int64_t)round((frame->pts - 1) * enc->timestamp_step);
	int64_t tsNow = (int64_t)round(frame->pts * enc->timestamp_step);

	surface->SetPts(tsNow);
	surface->SetProperty(AMF_PRESENT_TIMESTAMP, frame->pts);
	surface->SetDuration(tsNow - tsLast);

	//convert
	res = enc->converter_amf->SubmitInput(surface);
	if (res != AMF_OK) {
		AMF_LOG_ERROR(
			"Failed to SubmitInput(converter) with error lsd\n",
			AMF::Instance()->GetTrace()->GetResultText(res));
	}
	while (true) {
		res = enc->converter_amf->QueryOutput(&pConvData);
		if (res == AMF_INPUT_FULL)
			std::this_thread::sleep_for(enc->query_wait_time);
		else
			break;
	}

	AMF_RESULT tmp = res;
	res = enc->encoder_amf->SubmitInput(pConvData);
	if (res != AMF_OK) {
		AMF_LOG_ERROR("Failed to SubmitInput(encoder) with error %ls\n",
			      AMF::Instance()->GetTrace()->GetResultText(res));
		*received_packet = false;
		return false;
	}
	while (true) {
		res = enc->encoder_amf->QueryOutput(&pOutData);
		if (res == AMF_OK)
			break;
		switch (res) {
		case AMF_NEED_MORE_INPUT: {
			*received_packet = false;
			return true;
		}
		case AMF_REPEAT: {
			break;
		}
		default: {
			AMF_LOG_WARNING(
				"Failed to QueryOutput  with code: %ls\n",
				AMF::Instance()->GetTrace()->GetResultText(
					res));
			break;
		}
		}

		std::this_thread::sleep_for(enc->query_wait_time);
	}
	*received_packet = true;
	return amf_data_to_encoder_packet(data, pOutData, packet);
}

void amf_destroy(void *data)
{
	struct amf_data *enc = (amf_data *)data;

	for (size_t i = 0; i < enc->input_textures.num; i++) {
		ID3D11Texture2D *tex = enc->input_textures.array[i].tex;
		IDXGIKeyedMutex *km = enc->input_textures.array[i].km;
		tex->Release();
		km->Release();
	}

	da_free(enc->input_textures);

	bfree(enc);
}

bool amf_extra_data(void *data, uint8_t **header, size_t *size)
{
	struct amf_data *enc = (amf_data *)data;

	if (!enc->header) {
		return false;
	}

	*header = (uint8_t *)enc->header->GetNative();
	*size = enc->header->GetSize();
	return true;
}

AMF_RESULT amf_create_encoder(obs_data_t *settings, amf_data *enc)
{
	AMF_RESULT result = AMF_FAIL;

	// OBS Settings
	video_t *obsVideoInfo = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi =
		video_output_get_info(obsVideoInfo);
	uint32_t obsFPSnum = voi->fps_num;
	uint32_t obsFPSden = voi->fps_den;

	enc->frame_w = obs_encoder_get_width(enc->encoder);
	enc->frame_h = obs_encoder_get_height(enc->encoder);

	enc->frame_rate = AMFConstructRate(obsFPSnum, obsFPSden);
	enc->query_wait_time = std::chrono::milliseconds(1);
	double_t frameRateFraction =
		((double_t)obsFPSden / (double_t)obsFPSnum);
	enc->timestamp_step = AMF_SECOND * frameRateFraction;
	enc->texture = nullptr;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		enc->in_color_profile = AMF_VIDEO_CONVERTER_COLOR_PROFILE_601;
		enc->in_characteristic =
			AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		enc->in_color_profile = AMF_VIDEO_CONVERTER_COLOR_PROFILE_709;
		enc->in_characteristic =
			AMF_COLOR_TRANSFER_CHARACTERISTIC_BT709;
		break;
	case VIDEO_CS_SRGB:
		enc->in_color_profile = AMF_VIDEO_CONVERTER_COLOR_PROFILE_709;
		enc->in_characteristic =
			AMF_COLOR_TRANSFER_CHARACTERISTIC_IEC61966_2_1;
		break;
	}

	switch (voi->format) {
	case VIDEO_FORMAT_NV12:
		enc->in_surface_format = amf::AMF_SURFACE_NV12;
		break;
	case VIDEO_FORMAT_I420:
		enc->in_surface_format = amf::AMF_SURFACE_YUV420P;
		break;
	case VIDEO_FORMAT_YUY2:
		enc->in_surface_format = amf::AMF_SURFACE_YUY2;
		break;
	case VIDEO_FORMAT_RGBA:
		enc->in_surface_format = amf::AMF_SURFACE_RGBA;
		break;
	case VIDEO_FORMAT_BGRA:
		enc->in_surface_format = amf::AMF_SURFACE_BGRA;
		break;
	case VIDEO_FORMAT_Y800:
		enc->in_surface_format = amf::AMF_SURFACE_GRAY8;
		break;
	default: {
		AMF_LOG_ERROR("Unsupported surface format.");
		return AMF_FAIL;
	}
	}

	//////////////////////////////////////////////////////////////////////////

	result = AMF::Instance()->GetFactory()->CreateContext(&enc->context);
	if (result != AMF_OK) {
		AMF_LOG_WARNING("Context Failed");
		return result;
	}

	result = enc->context->InitDX11(enc->pD3D11Device, AMF_DX11_1);
	if (result != AMF_OK) {
		AMF_LOG_WARNING("Failed to init from dx11.");
		return result;
	}

	/// Create Converter
	bool scaling = obs_encoder_scaling_enabled(enc->encoder);
	if (!obs_nv12_tex_active() || scaling) {
		if (!obs_nv12_tex_active()) {
			enc->convertor_frame_w = enc->frame_w;
			enc->convertor_frame_h = enc->frame_h;
		} else {
			enc->convertor_frame_w = voi->width;
			enc->convertor_frame_h = voi->height;
		}

		result = AMF::Instance()->GetFactory()->CreateComponent(
			enc->context, AMFVideoConverter, &enc->converter_amf);
		if (result != AMF_OK) {
			AMF_LOG_WARNING(
				"CreateComponent AMFVideoConverter Failed");
			return result;
		}
		SET_AMF_VALUE_OR_FAIL(enc->converter_amf,
				      AMF_VIDEO_CONVERTER_MEMORY_TYPE,
				      amf::AMF_MEMORY_UNKNOWN);
		SET_AMF_VALUE_OR_FAIL(enc->converter_amf,
				      AMF_VIDEO_CONVERTER_OUTPUT_FORMAT,
				      amf::AMF_SURFACE_NV12);
		SET_AMF_VALUE_OR_FAIL(enc->converter_amf,
				      AMF_VIDEO_CONVERTER_COLOR_PROFILE,
				      enc->in_color_profile);
		SET_AMF_VALUE_OR_FAIL(
			enc->converter_amf,
			AMF_VIDEO_CONVERTER_INPUT_TRANSFER_CHARACTERISTIC,
			enc->in_characteristic);
		SET_AMF_VALUE_OR_FAIL(
			enc->converter_amf, AMF_VIDEO_CONVERTER_OUTPUT_SIZE,
			AMFConstructSize(enc->frame_w, enc->frame_h));
		enc->converter_amf->Init(enc->in_surface_format,
					 enc->convertor_frame_w,
					 enc->convertor_frame_h);
	}

	// Create Encoder
	if (enc->codec == amf_data::CODEC_ID_H264) {
		result = AMF::Instance()->GetFactory()->CreateComponent(
			enc->context, AMFVideoEncoderVCE_AVC,
			&enc->encoder_amf);
	} else if (enc->codec == amf_data::CODEC_ID_HEVC) {
		result = AMF::Instance()->GetFactory()->CreateComponent(
			enc->context, AMFVideoEncoder_HEVC, &enc->encoder_amf);
	}
	return result;
fail:
	return AMF_FAIL;
}
