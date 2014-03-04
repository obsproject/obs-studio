/*
 * Copyright (c) 2013-2014 Hugh Bailey <obs.jim@gmail.com>
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

#include <errno.h>
#include <stdlib.h>
#include "c99defs.h"
#include "platform.h"
#include "bmem.h"
#include "utf8.h"
#include "dstr.h"

FILE *os_wfopen(const wchar_t *path, const char *mode)
{
	FILE *file;
#ifdef _MSC_VER
	wchar_t *wcs_mode;

	os_utf8_to_wcs_ptr(mode, 0, &wcs_mode);
	file = _wfopen(path, wcs_mode);
	bfree(wcs_mode);
#else
	char *mbs_path;

	os_wcs_to_utf8_ptr(path, 0, &mbs_path);
	file = fopen(mbs_path, mode);
	bfree(mbs_path);
#endif
	return file;
}

FILE *os_fopen(const char *path, const char *mode)
{
#ifdef _WIN32
	wchar_t *wpath = NULL;
	FILE *file = NULL;
	os_utf8_to_wcs_ptr(path, 0, &wpath);
	file = os_wfopen(wpath, mode);
	bfree(wpath);

	return file;
#else
	return fopen(path, mode);
#endif
}

off_t os_fgetsize(FILE *file)
{
	off_t cur_offset = ftello(file);
	off_t size;
	int errval = 0;

	if (fseeko(file, 0, SEEK_END) == -1)
		return -1;

	size = ftello(file);
	if (size == -1)
		errval = errno;

	if (fseeko(file, cur_offset, SEEK_SET) != 0 && errval != 0)
		errno = errval;

	return size;
}

size_t os_fread_mbs(FILE *file, char **pstr)
{
	size_t size = 0;
	size_t len = 0;

	fseeko(file, 0, SEEK_END);
	size = (size_t)ftello(file);
	*pstr = NULL;

	if (size > 0) {
		char *mbstr = bmalloc(size+1);

		fseeko(file, 0, SEEK_SET);
		size = fread(mbstr, 1, size, file);
		if (size == 0) {
			bfree(mbstr);
			return 0;
		}

		mbstr[size] = 0;
		len = os_mbs_to_utf8_ptr(mbstr, size, pstr);
		bfree(mbstr);
	}

	return len;
}

size_t os_fread_utf8(FILE *file, char **pstr)
{
	size_t size = 0;
	size_t size_read;
	size_t len = 0;

	fseeko(file, 0, SEEK_END);
	size = (size_t)ftello(file);

	if (size > 0) {
		char bom[3];
		char *utf8str;
		off_t offset;

		/* remove the ghastly BOM if present */
		fseeko(file, 0, SEEK_SET);
		size_read = fread(bom, 1, 3, file);
		if (size_read != 3)
			return 0;

		offset = (astrcmp_n(bom, "\xEF\xBB\xBF", 3) == 0) ? 3 : 0;

		size -= offset;
		utf8str = bmalloc(size+1);
		fseeko(file, offset, SEEK_SET);

		size = fread(utf8str, 1, size, file);
		if (size == 0) {
			bfree(utf8str);
			return 0;
		}

		utf8str[size] = 0;

		*pstr = utf8str;
	} else {
		*pstr = NULL;
	}

	return len;
}

char *os_quick_read_mbs_file(const char *path)
{
	FILE *f = os_fopen(path, "rb");
	char *file_string = NULL;

	if (!f)
		return NULL;

	os_fread_mbs(f, &file_string);
	fclose(f);

	return file_string;
}

char *os_quick_read_utf8_file(const char *path)
{
	FILE *f = os_fopen(path, "rb");
	char *file_string = NULL;

	if (!f)
		return NULL;

	os_fread_utf8(f, &file_string);
	fclose(f);

	return file_string;
}

bool os_quick_write_mbs_file(const char *path, const char *str, size_t len)
{
	FILE *f = os_fopen(path, "wb");
	char *mbs = NULL;
	size_t mbs_len = 0;
	if (!f)
		return false;

	mbs_len = os_utf8_to_mbs_ptr(str, len, &mbs);
	if (mbs_len)
		fwrite(mbs, 1, mbs_len, f);
	bfree(mbs);
	fclose(f);

	return true;
}

bool os_quick_write_utf8_file(const char *path, const char *str, size_t len,
		bool marker)
{
	FILE *f = os_fopen(path, "wb");
	if (!f)
		return false;

	if (marker)
		fwrite("\xEF\xBB\xBF", 1, 3, f);
	if (len)
		fwrite(str, 1, len, f);
	fclose(f);

	return true;
}

size_t os_mbs_to_wcs(const char *str, size_t len, wchar_t *dst)
{
	size_t out_len = dst ? len : mbstowcs(NULL, str, len);

	if (len && dst) {
		mbstowcs(dst, str, out_len+1);
		dst[out_len] = 0;
	}

	return out_len;
}

size_t os_utf8_to_wcs(const char *str, size_t len, wchar_t *dst)
{
	size_t in_len = len ? len : strlen(str);
	size_t out_len = dst ? len : utf8_to_wchar(str, in_len, NULL, 0, 0);

	if (out_len && dst) {
		utf8_to_wchar(str, in_len, dst, out_len+1, 0);
		dst[out_len] = 0;
	}

	return out_len;
}

size_t os_wcs_to_mbs(const wchar_t *str, size_t len, char *dst)
{
	size_t out_len = dst ? len : wcstombs(NULL, str, len);

	if (len && dst) {
		wcstombs(dst, str, out_len+1);
		dst[out_len] = 0;
	}

	return out_len;
}

size_t os_wcs_to_utf8(const wchar_t *str, size_t len, char *dst)
{
	size_t in_len = (len != 0) ? len : wcslen(str);
	size_t out_len = dst ? len : wchar_to_utf8(str, in_len, NULL, 0, 0);

	if (out_len && dst) {
		wchar_to_utf8(str, in_len, dst, out_len+1, 0);
		dst[out_len] = 0;
	}

	return out_len;
}

size_t os_mbs_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr)
{
	size_t  out_len = os_mbs_to_wcs(str, len, NULL);

	*pstr = bmalloc((out_len+1) * sizeof(wchar_t));
	return os_mbs_to_wcs(str, out_len, *pstr);
}

size_t os_utf8_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr)
{
	size_t  out_len = os_utf8_to_wcs(str, len, NULL);
	*pstr = bmalloc((out_len+1) * sizeof(wchar_t));
	return os_utf8_to_wcs(str, out_len, *pstr);
}

size_t os_wcs_to_mbs_ptr(const wchar_t *str, size_t len, char **pstr)
{
	size_t out_len = os_wcs_to_mbs(str, len, NULL);

	*pstr = bmalloc((out_len+1) * sizeof(char));
	return os_wcs_to_mbs(str, out_len, *pstr);
}

size_t os_wcs_to_utf8_ptr(const wchar_t *str, size_t len, char **pstr)
{
	size_t out_len = os_wcs_to_utf8(str, len, NULL);

	*pstr = bmalloc((out_len+1) * sizeof(char));
	return os_wcs_to_utf8(str, out_len, *pstr);
}

size_t os_utf8_to_mbs_ptr(const char *str, size_t len, char **pstr)
{
	wchar_t *wstr = NULL;
	char *dst = NULL;
	size_t wlen = os_utf8_to_wcs_ptr(str, len, &wstr);
	size_t out_len = os_wcs_to_mbs_ptr(wstr, wlen, &dst);

	bfree(wstr);
	*pstr = dst;

	return out_len;
}

size_t os_mbs_to_utf8_ptr(const char *str, size_t len, char **pstr)
{
	wchar_t *wstr = NULL;
	char *dst = NULL;
	size_t wlen = os_mbs_to_wcs_ptr(str, len, &wstr);
	size_t out_len = os_wcs_to_utf8_ptr(wstr, wlen, &dst);

	bfree(wstr);
	*pstr = dst;

	return out_len;
}

#ifdef _MSC_VER
int fseeko(FILE *stream, off_t offset, int whence)
{
#if _FILE_OFFSET_BITS == 64
	return _fseeki64(stream, offset, whence);
#else
	return fseek(stream, offset, whence);
#endif /* _FILE_OFFSET_BITS == 64 */
}

off_t ftello(FILE *stream)
{
#if _FILE_OFFSET_BITS == 64
	return _ftelli64(stream);
#else
	return ftell(stream);
#endif /* _FILE_OFFSET_BITS == 64 */
}
#endif /* _MSC_VER */
