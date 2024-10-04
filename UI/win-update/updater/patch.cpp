/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "updater.hpp"

#include <vector>

using namespace std;

/* ------------------------------------------------------------------------ */

static int64_t offtin(const uint8_t *buf)
{
	int64_t y;

	y = buf[7] & 0x7F;
	y = y * 256;
	y += buf[6];
	y = y * 256;
	y += buf[5];
	y = y * 256;
	y += buf[4];
	y = y * 256;
	y += buf[3];
	y = y * 256;
	y += buf[2];
	y = y * 256;
	y += buf[1];
	y = y * 256;
	y += buf[0];

	if (buf[7] & 0x80)
		y = -y;

	return y;
}

/* ------------------------------------------------------------------------ */

constexpr const char *kDeltaMagic = "BOUF//ZSTD//DICT";
constexpr int kMagicSize = 16;
constexpr int kHeaderSize = kMagicSize + 8; // magic + int64_t delta size

int ApplyPatch(ZSTD_DCtx *zstdCtx, const std::byte *patch_data, const size_t patch_size, const wchar_t *targetFile)
try {
	int64_t newsize;
	bool success;

	WinHandle hTarget;

	/* --------------------------------- *
	 * open patch and file to patch      */

	hTarget = CreateFile(targetFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (!hTarget.Valid())
		throw int(GetLastError());

	/* --------------------------------- *
	 * read patch header                 */

	if (memcmp(patch_data, kDeltaMagic, kMagicSize) != 0)
		throw int(-4);

	/* --------------------------------- *
	 * allocate new file size data       */

	newsize = offtin((const uint8_t *)patch_data + kMagicSize);
	if (newsize < 0 || newsize >= 0x7ffffffff)
		throw int(-5);

	vector<std::byte> newData;
	try {
		newData.resize((size_t)newsize);
	} catch (...) {
		throw int(-1);
	}

	/* --------------------------------- *
	 * read old file                     */

	DWORD read;
	DWORD oldFileSize;

	oldFileSize = GetFileSize(hTarget, nullptr);
	if (oldFileSize == INVALID_FILE_SIZE)
		throw int(GetLastError());

	vector<std::byte> oldData;
	try {
		oldData.resize(oldFileSize);
	} catch (...) {
		throw int(-1);
	}

	if (!ReadFile(hTarget, oldData.data(), oldFileSize, &read, nullptr))
		throw int(GetLastError());
	if (read != oldFileSize)
		throw int(-1);

	/* --------------------------------- *
	 * patch to new file data            */

	size_t result = ZSTD_decompress_usingDict(zstdCtx, newData.data(), newData.size(), patch_data + kHeaderSize,
						  patch_size - kHeaderSize, oldData.data(), oldData.size());

	if (result != newsize || ZSTD_isError(result))
		throw int(-9);

	/* --------------------------------- *
	 * write new file                    */

	hTarget = nullptr;
	hTarget = CreateFile(targetFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (!hTarget.Valid())
		throw int(GetLastError());

	DWORD written;

	success = !!WriteFile(hTarget, newData.data(), (DWORD)newsize, &written, nullptr);
	if (!success || written != newsize)
		throw int(GetLastError());

	return 0;

} catch (int code) {
	return code;
}
