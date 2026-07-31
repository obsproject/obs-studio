#include <windows.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <stdint.h>
#include <vector>

using namespace Gdiplus;

extern HINSTANCE dll_inst;

namespace {

struct Placeholder {
	std::vector<uint8_t> data;
	int cx = 0;
	int cy = 0;
	bool valid = false;
};

/* XXX: optimize this later.  or don't, it's only called once. */

std::vector<uint8_t> convert_placeholder(const uint8_t *rgb_in, int width, int height)
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

		*(out++) = (uint8_t)(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
		*(out++) = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
		*(out++) = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
	}

	std::vector<uint8_t> nv12_out;
	nv12_out.resize(width * height * 3 / 2);

	in = &yuv_out[0];
	end = in + size;

	out = &nv12_out[0];
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

	return nv12_out;
}

Placeholder load_placeholder_internal()
{
	Placeholder result;
	Status s;

	wchar_t file[MAX_PATH];
	if (!GetModuleFileNameW(dll_inst, file, MAX_PATH)) {
		return result;
	}

	wchar_t *slash = wcsrchr(file, '\\');
	if (!slash) {
		return result;
	}

	slash[1] = 0;

	StringCbCat(file, sizeof(file), L"placeholder.png");

	Bitmap bmp(file);
	if (bmp.GetLastStatus() != Status::Ok) {
		return result;
	}

	const int cx = bmp.GetWidth();
	const int cy = bmp.GetHeight();

	BitmapData bmd = {};
	Rect r(0, 0, cx, cy);

	s = bmp.LockBits(&r, ImageLockModeRead, PixelFormat24bppRGB, &bmd);
	if (s != Status::Ok) {
		return result;
	}

	result.data = convert_placeholder((const uint8_t *)bmd.Scan0, cx, cy);
	result.cx = cx;
	result.cy = cy;
	result.valid = true;

	bmp.UnlockBits(&bmd);
	return result;
}

Placeholder load_placeholder()
{
	GdiplusStartupInput si;
	ULONG_PTR token;

	if (GdiplusStartup(&token, &si, nullptr) != Status::Ok) {
		return Placeholder();
	}

	Placeholder result = load_placeholder_internal();

	GdiplusShutdown(token);
	return result;
}

// A single application can create any number of VCamFilter instances, each of
// which loads the placeholder from its own thread. As this is shared global
// state, ensure only a single copy is initialized with C++11 function-local
// static initialization to prevent multiple initializations stomping on
// each other.
const Placeholder &get_placeholder()
{
	static const Placeholder placeholder = load_placeholder();
	return placeholder;
}

} // namespace

bool initialize_placeholder()
{
	return get_placeholder().valid;
}

const uint8_t *get_placeholder_ptr()
{
	const Placeholder &placeholder = get_placeholder();

	if (placeholder.valid) {
		return placeholder.data.data();
	}

	return nullptr;
}

const bool get_placeholder_size(int *out_cx, int *out_cy)
{
	const Placeholder &placeholder = get_placeholder();

	if (placeholder.valid) {
		*out_cx = placeholder.cx;
		*out_cy = placeholder.cy;
		return true;
	}

	return false;
}
