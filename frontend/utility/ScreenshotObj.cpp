/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "ScreenshotObj.hpp"

#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#ifdef _WIN32
#include <wincodec.h>
#include <wincodecsdk.h>
#include <wrl/client.h>
#pragma comment(lib, "windowscodecs.lib")
#endif

#include "moc_ScreenshotObj.cpp"

static void ScreenshotTick(void *param, float);

ScreenshotObj::ScreenshotObj(obs_source_t *source) : weakSource(OBSGetWeakRef(source))
{
	obs_add_tick_callback(ScreenshotTick, this);
}

ScreenshotObj::~ScreenshotObj()
{
	obs_enter_graphics();
	gs_stagesurface_destroy(stagesurf);
	gs_texrender_destroy(texrender);
	obs_leave_graphics();

	obs_remove_tick_callback(ScreenshotTick, this);

	if (th.joinable()) {
		th.join();

		if (cx && cy) {
			OBSBasic *main = OBSBasic::Get();
			main->ShowStatusBarMessage(
				QTStr("Basic.StatusBar.ScreenshotSavedTo").arg(QT_UTF8(path.c_str())));

			main->lastScreenshot = path;

			main->OnEvent(OBS_FRONTEND_EVENT_SCREENSHOT_TAKEN);
		}
	}
}

void ScreenshotObj::Screenshot()
{
	OBSSource source = OBSGetStrongRef(weakSource);

	if (source) {
		cx = obs_source_get_width(source);
		cy = obs_source_get_height(source);
	} else {
		obs_video_info ovi;
		obs_get_video_info(&ovi);
		cx = ovi.base_width;
		cy = ovi.base_height;
	}

	if (!cx || !cy) {
		blog(LOG_WARNING, "Cannot screenshot, invalid target size");
		obs_remove_tick_callback(ScreenshotTick, this);
		deleteLater();
		return;
	}

#ifdef _WIN32
	enum gs_color_space space = obs_source_get_color_space(source, 0, nullptr);
	if (space == GS_CS_709_EXTENDED) {
		/* Convert for JXR */
		space = GS_CS_709_SCRGB;
	}
#else
	/* Tonemap to SDR if HDR */
	const enum gs_color_space space = GS_CS_SRGB;
#endif
	const enum gs_color_format format = gs_get_format_from_space(space);

	texrender = gs_texrender_create(format, GS_ZS_NONE);
	stagesurf = gs_stagesurface_create(cx, cy, format);

	if (gs_texrender_begin_with_color_space(texrender, cx, cy, space)) {
		vec4 zero;
		vec4_zero(&zero);

		gs_clear(GS_CLEAR_COLOR, &zero, 0.0f, 0);
		gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		if (source) {
			obs_source_inc_showing(source);
			obs_source_video_render(source);
			obs_source_dec_showing(source);
		} else {
			obs_render_main_texture();
		}

		gs_blend_state_pop();
		gs_texrender_end(texrender);
	}
}

void ScreenshotObj::Download()
{
	gs_stage_texture(stagesurf, gs_texrender_get_texture(texrender));
}

void ScreenshotObj::Copy()
{
	uint8_t *videoData = nullptr;
	uint32_t videoLinesize = 0;

	if (gs_stagesurface_map(stagesurf, &videoData, &videoLinesize)) {
		if (gs_stagesurface_get_color_format(stagesurf) == GS_RGBA16F) {
			const uint32_t linesize = cx * 8;
			half_bytes.reserve(cx * cy * 8);

			for (uint32_t y = 0; y < cy; y++) {
				const uint8_t *const line = videoData + (y * videoLinesize);
				half_bytes.insert(half_bytes.end(), line, line + linesize);
			}
		} else {
			image = QImage(cx, cy, QImage::Format::Format_RGBX8888);

			int linesize = image.bytesPerLine();
			for (int y = 0; y < (int)cy; y++)
				memcpy(image.scanLine(y), videoData + (y * videoLinesize), linesize);
		}

		gs_stagesurface_unmap(stagesurf);
	}
}

void ScreenshotObj::Save()
{
	OBSBasic *main = OBSBasic::Get();
	config_t *config = main->Config();

	const char *mode = config_get_string(config, "Output", "Mode");
	const char *type = config_get_string(config, "AdvOut", "RecType");
	const char *adv_path = strcmp(type, "Standard") ? config_get_string(config, "AdvOut", "FFFilePath")
							: config_get_string(config, "AdvOut", "RecFilePath");
	const char *rec_path = strcmp(mode, "Advanced") ? config_get_string(config, "SimpleOutput", "FilePath")
							: adv_path;

	bool noSpace = config_get_bool(config, "SimpleOutput", "FileNameWithoutSpace");
	const char *filenameFormat = config_get_string(config, "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(config, "Output", "OverwriteIfExists");

	const char *ext = half_bytes.empty() ? "png" : "jxr";
	path = GetOutputFilename(rec_path, ext, noSpace, overwriteIfExists,
				 GetFormatString(filenameFormat, "Screenshot", nullptr).c_str());

	th = std::thread([this] { MuxAndFinish(); });
}

#ifdef _WIN32
static HRESULT SaveJxrImage(LPCWSTR path, uint8_t *pixels, uint32_t cx, uint32_t cy, IWICBitmapFrameEncode *frameEncode,
			    IPropertyBag2 *options)
{
	wchar_t lossless[] = L"Lossless";
	PROPBAG2 bag = {};
	bag.pstrName = lossless;
	VARIANT value = {};
	value.vt = VT_BOOL;
	value.bVal = TRUE;
	HRESULT hr = options->Write(1, &bag, &value);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->Initialize(options);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->SetSize(cx, cy);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->SetResolution(72, 72);
	if (FAILED(hr))
		return hr;

	WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat64bppRGBAHalf;
	hr = frameEncode->SetPixelFormat(&pixelFormat);
	if (FAILED(hr))
		return hr;

	if (memcmp(&pixelFormat, &GUID_WICPixelFormat64bppRGBAHalf, sizeof(WICPixelFormatGUID)) != 0)
		return E_FAIL;

	hr = frameEncode->WritePixels(cy, cx * 8, cx * cy * 8, pixels);
	if (FAILED(hr))
		return hr;

	hr = frameEncode->Commit();
	if (FAILED(hr))
		return hr;

	return S_OK;
}

static HRESULT SaveJxr(LPCWSTR path, uint8_t *pixels, uint32_t cx, uint32_t cy)
{
	Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
				      IID_PPV_ARGS(factory.GetAddressOf()));
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IWICStream> stream;
	hr = factory->CreateStream(stream.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder = NULL;
	hr = factory->CreateEncoder(GUID_ContainerFormatWmp, NULL, encoder.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
	if (FAILED(hr))
		return hr;

	Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frameEncode;
	Microsoft::WRL::ComPtr<IPropertyBag2> options;
	hr = encoder->CreateNewFrame(frameEncode.GetAddressOf(), options.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = SaveJxrImage(path, pixels, cx, cy, frameEncode.Get(), options.Get());
	if (FAILED(hr))
		return hr;

	encoder->Commit();
	return S_OK;
}
#endif // #ifdef _WIN32

void ScreenshotObj::MuxAndFinish()
{
	if (half_bytes.empty()) {
		image.save(QT_UTF8(path.c_str()));
		blog(LOG_INFO, "Saved screenshot to '%s'", path.c_str());
	} else {
#ifdef _WIN32
		wchar_t *path_w = nullptr;
		os_utf8_to_wcs_ptr(path.c_str(), 0, &path_w);
		if (path_w) {
			SaveJxr(path_w, half_bytes.data(), cx, cy);
			bfree(path_w);
		}
#endif // #ifdef _WIN32
	}

	deleteLater();
}

#define STAGE_SCREENSHOT 0
#define STAGE_DOWNLOAD 1
#define STAGE_COPY_AND_SAVE 2
#define STAGE_FINISH 3

static void ScreenshotTick(void *param, float)
{
	ScreenshotObj *data = reinterpret_cast<ScreenshotObj *>(param);

	if (data->stage == STAGE_FINISH) {
		return;
	}

	obs_enter_graphics();

	switch (data->stage) {
	case STAGE_SCREENSHOT:
		data->Screenshot();
		break;
	case STAGE_DOWNLOAD:
		data->Download();
		break;
	case STAGE_COPY_AND_SAVE:
		data->Copy();
		QMetaObject::invokeMethod(data, "Save");
		obs_remove_tick_callback(ScreenshotTick, data);
		break;
	}

	obs_leave_graphics();

	data->stage++;
}
