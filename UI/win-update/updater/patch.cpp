/*
 * Copyright (c) 2017-2018 Hugh Bailey <obs.jim@gmail.com>
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

#include <stdint.h>
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

int ApplyPatch(ZSTD_DCtx *zstdCtx, const wchar_t *patchFile,
	       const wchar_t *targetFile)
try {
	uint8_t header[24];
	int64_t newsize;
	bool success;

	WinHandle hPatch;
	WinHandle hTarget;

	/* --------------------------------- *
	 * open patch and file to patch      */

	hPatch = CreateFile(patchFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING,
			    0, nullptr);
	if (!hPatch.Valid())
		throw int(GetLastError());

	hTarget = CreateFile(targetFile, GENERIC_READ, 0, nullptr,
			     OPEN_EXISTING, 0, nullptr);
	if (!hTarget.Valid())
		throw int(GetLastError());

	/* --------------------------------- *
	 * read patch header                 */

	DWORD read;
	DWORD patchFileSize;

	patchFileSize = GetFileSize(hPatch, nullptr);
	if (patchFileSize == INVALID_FILE_SIZE)
		throw int(GetLastError());

	success = !!ReadFile(hPatch, header, sizeof(header), &read, nullptr);
	if (success && read == sizeof(header)) {
		if (memcmp(header, "BOUF//ZSTD//DICT", 16))
			throw int(-4);
	} else {
		throw int(GetLastError());
	}

	/* --------------------------------- *
	 * allocate new file size data       */

	newsize = offtin(header + 16);
	if (newsize < 0 || newsize >= 0x7ffffffff)
		throw int(-5);

	vector<uint8_t> newData;
	try {
		newData.resize((size_t)newsize);
	} catch (...) {
		throw int(-1);
	}

	/* --------------------------------- *
	 * read remainder of patch file     */

	vector<uint8_t> patchData;
	try {
		patchData.resize(patchFileSize - sizeof(header));
	} catch (...) {
		throw int(-1);
	}

	if (!ReadFile(hPatch, &patchData[0], patchFileSize - sizeof(header),
		      &read, nullptr))
		throw int(GetLastError());

	if (read != (patchFileSize - sizeof(header)))
		throw int(-1);

	/* --------------------------------- *
	 * read old file                     */

	DWORD targetFileSize;

	targetFileSize = GetFileSize(hTarget, nullptr);
	if (targetFileSize == INVALID_FILE_SIZE)
		throw int(GetLastError());

	vector<uint8_t> oldData;
	try {
		oldData.resize(targetFileSize);
	} catch (...) {
		throw int(-1);
	}

	if (!ReadFile(hTarget, &oldData[0], targetFileSize, &read, nullptr))
		throw int(GetLastError());
	if (read != targetFileSize)
		throw int(-1);

	/* --------------------------------- *
	 * patch to new file data            */

	size_t result = ZSTD_decompress_usingDict(
		zstdCtx, &newData[0], newData.size(), patchData.data(),
		patchData.size(), oldData.data(), oldData.size());

	if (result != newsize || ZSTD_isError(result))
		throw int(-9);

	/* --------------------------------- *
	 * write new file                    */

	hTarget = nullptr;
	hTarget = CreateFile(targetFile, GENERIC_WRITE, 0, nullptr,
			     CREATE_ALWAYS, 0, nullptr);
	if (!hTarget.Valid())
		throw int(GetLastError());

	DWORD written;

	success = !!WriteFile(hTarget, newData.data(), (DWORD)newsize, &written,
			      nullptr);
	if (!success || written != newsize)
		throw int(GetLastError());

	return 0;

} catch (int code) {
	return code;
}

int DecompressFile(ZSTD_DCtx *ctx, const wchar_t *tempFile, size_t newSize)
try {
	WinHandle hTemp;

	hTemp = CreateFile(tempFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0,
			   nullptr);
	if (!hTemp.Valid())
		throw int(GetLastError());

	/* --------------------------------- *
	 * read compressed data              */

	DWORD read;
	DWORD compressedFileSize;

	compressedFileSize = GetFileSize(hTemp, nullptr);
	if (compressedFileSize == INVALID_FILE_SIZE)
		throw int(GetLastError());

	vector<uint8_t> oldData;
	try {
		oldData.resize(compressedFileSize);
	} catch (...) {
		throw int(-1);
	}

	if (!ReadFile(hTemp, &oldData[0], compressedFileSize, &read, nullptr))
		throw int(GetLastError());
	if (read != compressedFileSize)
		throw int(-1);

	/* --------------------------------- *
	 * decompress data                   */

	vector<uint8_t> newData;
	try {
		newData.resize((size_t)newSize);
	} catch (...) {
		throw int(-1);
	}

	size_t result = ZSTD_decompressDCtx(ctx, &newData[0], newData.size(),
					    oldData.data(), oldData.size());

	if (result != newSize)
		throw int(-9);
	if (ZSTD_isError(result))
		throw int(-10);

	/* --------------------------------- *
	 * overwrite temp file with new data */

	hTemp = nullptr;
	hTemp = CreateFile(tempFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
			   0, nullptr);
	if (!hTemp.Valid())
		throw int(GetLastError());

	bool success;
	DWORD written;

	success = !!WriteFile(hTemp, newData.data(), (DWORD)newSize, &written,
			      nullptr);
	if (!success || written != newSize)
		throw int(GetLastError());

	return 0;
} catch (int code) {
	return code;
}
