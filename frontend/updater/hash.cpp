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

#include <util/windows/WinHandle.hpp>
#include <vector>

using namespace std;

void HashToString(const B2Hash &in, string &out)
{
	constexpr char alphabet[] = "0123456789abcdef";
	out.resize(kBlake2StrLength);

	for (size_t i = 0; i != kBlake2HashLength; ++i) {
		out[2 * i] = alphabet[(uint8_t)in[i] / 16];
		out[2 * i + 1] = alphabet[(uint8_t)in[i] % 16];
	}
}

void StringToHash(const string &in, B2Hash &out)
{
	unsigned int temp;
	const char *str = in.c_str();

	for (size_t i = 0; i < kBlake2HashLength; i++) {
		sscanf_s(str + i * 2, "%02x", &temp);
		out[i] = static_cast<std::byte>(temp);
	}
}

bool CalculateFileHash(const wchar_t *path, B2Hash &hash)
{
	static __declspec(thread) vector<BYTE> hashBuffer;
	blake2b_state blake2;
	if (blake2b_init(&blake2, kBlake2HashLength) != 0)
		return false;

	hashBuffer.resize(1048576);

	WinHandle handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return false;

	for (;;) {
		DWORD read = 0;
		if (!ReadFile(handle, hashBuffer.data(), (DWORD)hashBuffer.size(), &read, nullptr))
			return false;

		if (!read)
			break;

		if (blake2b_update(&blake2, hashBuffer.data(), read) != 0)
			return false;
	}

	if (blake2b_final(&blake2, hash.data(), hash.size()) != 0)
		return false;

	return true;
}
