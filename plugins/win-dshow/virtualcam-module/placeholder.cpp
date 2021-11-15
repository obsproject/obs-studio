#include <windows.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <stdint.h>
#include <vector>

using namespace Gdiplus;

extern HINSTANCE dll_inst;

static std::vector<uint8_t> placeholder;
static bool initialized = false;
int cx, cy;

/* XXX: optimize this later.  or don't, it's only called once. */

static void convert_placeholder(const uint8_t *rgb_in, int width, int height)
{
	size_t size = width * height * 3;
	size_t linesize = width * 3;

	std::vector<uint8_t> yuv_out;
	yuv_out.resize(size);

	const uint8_t *in = rgb_in;
	const uint8_t *end = in + size;
	uint8_t *out = &yuv_out[0];

	while (in < end) {
		const int16_t b = *(in++);
		const int16_t g = *(in++);
		const int16_t r = *(in++);

		*(out++) = (uint8_t)(((66 * r + 129 * g + 25 * b + 128) >> 8) +
				     16);
		*(out++) = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) +
				     128);
		*(out++) = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) +
				     128);
	}

	placeholder.resize(width * height * 3 / 2);

	in = &yuv_out[0];
	end = in + size;

	out = &placeholder[0];
	uint8_t *chroma = out + width * height;

	while (in < end) {
		const uint8_t *in2 = in + linesize;
		const uint8_t *end2 = in2;
		uint8_t *out2 = out + width;

		while (in < end2) {
			int16_t u;
			int16_t v;

			*(out++) = *(in++);
			u = *(in++);
			v = *(in++);

			*(out++) = *(in++);
			u += *(in++);
			v += *(in++);

			*(out2++) = *(in2++);
			u += *(in2++);
			v += *(in2++);

			*(out2++) = *(in2++);
			u += *(in2++);
			v += *(in2++);

			*(chroma++) = (uint8_t)(u / 4);
			*(chroma++) = (uint8_t)(v / 4);
		}

		in = in2;
		out = out2;
	}
}

static bool load_placeholder_internal(int dest_cx, int dest_cy)
{
	Status s;

	wchar_t file[MAX_PATH];
	if (!GetModuleFileNameW(dll_inst, file, MAX_PATH)) {
		return false;
	}

	wchar_t *slash = wcsrchr(file, '\\');
	if (!slash) {
		return false;
	}

	slash[1] = 0;

	StringCbCat(file, sizeof(file), L"placeholder.png");

	Bitmap bmp(file);
	if (bmp.GetLastStatus() != Status::Ok) {
		return false;
	}

	cx = bmp.GetWidth();
	cy = bmp.GetHeight();

	if (cx == dest_cx && cy == dest_cy) {
		BitmapData bmd = {};
		Rect r(0, 0, cx, cy);

		s = bmp.LockBits(&r, ImageLockModeRead, PixelFormat24bppRGB,
				 &bmd);
		if (s != Status::Ok) {
			return false;
		}

		convert_placeholder((const uint8_t *)bmd.Scan0, cx, cy);

		bmp.UnlockBits(&bmd);
	} else {
		int rect_x, rect_y, rect_w, rect_h;
		float ratio_x = (float)cx / (float)dest_cx;
		float ratio_y = (float)cy / (float)dest_cy;
		if (ratio_x < ratio_y) {
			rect_y = 0;
			rect_h = dest_cy;
			rect_w = rect_h * ((float)cx / (float)cy);
			rect_x = (dest_cx - rect_w) / 2;
		} else {
			rect_x = 0;
			rect_w = dest_cx;
			rect_h = rect_w * ((float)cy / (float)cx);
			rect_y = (dest_cy - rect_h) / 2;
		}

		Bitmap target(dest_cx, dest_cy, bmp.GetPixelFormat());
		Gdiplus::Graphics graphics(&target);
		graphics.Clear(Color(0, 0, 0));

		Rect dest_r(rect_x, rect_y, rect_w, rect_h);
		graphics.SetInterpolationMode(
			Gdiplus::InterpolationModeHighQualityBicubic);
		graphics.DrawImage(&bmp, dest_r, 0, 0, cx, cy,
				   Gdiplus::UnitPixel);

		BitmapData bmd = {};
		Rect r(0, 0, dest_cx, dest_cy);

		s = target.LockBits(&r, ImageLockModeRead, PixelFormat24bppRGB,
				    &bmd);
		if (s != Status::Ok) {
			return false;
		}

		convert_placeholder((const uint8_t *)bmd.Scan0, dest_cx,
				    dest_cy);

		target.UnlockBits(&bmd);

		cx = dest_cx;
		cy = dest_cy;
	}

	return true;
}

bool initialize_placeholder(int dest_cx, int dest_cy)
{
	GdiplusStartupInput si;
	ULONG_PTR token;
	GdiplusStartup(&token, &si, nullptr);

	initialized = load_placeholder_internal(dest_cx, dest_cy);

	GdiplusShutdown(token);
	return initialized;
}

const uint8_t *get_placeholder_ptr()
{
	if (initialized)
		return placeholder.data();

	return nullptr;
}

const bool get_placeholder_size(int *out_cx, int *out_cy)
{
	if (initialized) {
		*out_cx = cx;
		*out_cy = cy;
		return true;
	}

	return false;
}
