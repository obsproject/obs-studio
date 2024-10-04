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

#include "Updater.hpp"

#include <algorithm>

using namespace std;

#define MAX_BUF_SIZE 262144
#define READ_BUF_SIZE 32768

/* ------------------------------------------------------------------------ */

static bool ReadHTTPData(string &responseBuf, const uint8_t *buffer, DWORD outSize)
{
	try {
		responseBuf.append((const char *)buffer, outSize);
	} catch (...) {
		return false;
	}
	return true;
}

bool HTTPPostData(const wchar_t *url, const BYTE *data, int dataLen, const wchar_t *extraHeaders, int *responseCode,
		  string &responseBuf)
{
	HttpHandle hSession;
	HttpHandle hConnect;
	HttpHandle hRequest;
	URL_COMPONENTS urlComponents = {};
	bool secure = false;

	wchar_t hostName[256];
	wchar_t path[1024];

	const wchar_t *acceptTypes[] = {L"*/*", nullptr};

	const DWORD tlsProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	const DWORD compressionFlags = WINHTTP_DECOMPRESSION_FLAG_ALL;

	responseBuf.clear();

	/* -------------------------------------- *
	 * get URL components                     */

	urlComponents.dwStructSize = sizeof(urlComponents);

	urlComponents.lpszHostName = hostName;
	urlComponents.dwHostNameLength = _countof(hostName);

	urlComponents.lpszUrlPath = path;
	urlComponents.dwUrlPathLength = _countof(path);

	WinHttpCrackUrl(url, 0, 0, &urlComponents);

	if (urlComponents.nPort == 443)
		secure = true;

	/* -------------------------------------- *
	 * connect to server                      */

	hSession = WinHttpOpen(L"OBS Studio Updater/3.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
			       WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		*responseCode = -1;
		return false;
	}

	WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, (LPVOID)&tlsProtocols, sizeof(tlsProtocols));
	WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, (LPVOID)&compressionFlags, sizeof(compressionFlags));

	hConnect = WinHttpConnect(hSession, hostName, secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
				  0);
	if (!hConnect) {
		*responseCode = -2;
		return false;
	}

	/* -------------------------------------- *
	 * request data                           */

	hRequest = WinHttpOpenRequest(hConnect, L"POST", path, nullptr, WINHTTP_NO_REFERER, acceptTypes,
				      secure ? WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH : WINHTTP_FLAG_REFRESH);
	if (!hRequest) {
		*responseCode = -3;
		return false;
	}

	bool bResults =
		!!WinHttpSendRequest(hRequest, extraHeaders, extraHeaders ? -1 : 0, (void *)data, dataLen, dataLen, 0);

	/* -------------------------------------- *
	 * end request                            */

	if (bResults) {
		bResults = !!WinHttpReceiveResponse(hRequest, nullptr);
	} else {
		*responseCode = GetLastError();
		return false;
	}

	/* -------------------------------------- *
	 * get headers                            */

	wchar_t statusCode[8];
	DWORD statusCodeLen;

	statusCodeLen = sizeof(statusCode);
	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode,
				 &statusCodeLen, WINHTTP_NO_HEADER_INDEX)) {
		*responseCode = -4;
		return false;
	} else {
		statusCode[_countof(statusCode) - 1] = 0;
	}

	/* -------------------------------------- *
	 * allocate response data                 */

	DWORD responseBufSize = MAX_BUF_SIZE;

	try {
		responseBuf.reserve(responseBufSize);
	} catch (...) {
		*responseCode = -6;
		return false;
	}

	/* -------------------------------------- *
	 * read data                              */

	*responseCode = wcstoul(statusCode, nullptr, 10);

	/* are we supposed to return true here? */
	if (!bResults || *responseCode != 200)
		return true;

	BYTE buffer[READ_BUF_SIZE];
	DWORD dwSize, outSize;

	do {
		/* Check for available data. */
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			*responseCode = -8;
			return false;
		}

		dwSize = std::min(dwSize, (DWORD)sizeof(buffer));

		if (!WinHttpReadData(hRequest, (void *)buffer, dwSize, &outSize)) {
			*responseCode = -9;
			return false;
		}

		if (!outSize)
			break;

		if (!ReadHTTPData(responseBuf, buffer, outSize)) {
			*responseCode = -6;
			return false;
		}

		if (WaitForSingleObject(cancelRequested, 0) == WAIT_OBJECT_0) {
			*responseCode = -14;
			return false;
		}
	} while (dwSize > 0);

	return true;
}

/* ------------------------------------------------------------------------ */

static bool ReadHTTPFile(HANDLE updateFile, const uint8_t *buffer, DWORD outSize, int *responseCode)
{
	DWORD written;
	if (!WriteFile(updateFile, buffer, outSize, &written, nullptr)) {
		*responseCode = -12;
		return false;
	}

	if (written != outSize) {
		*responseCode = -13;
		return false;
	}

	completedFileSize += outSize;
	return true;
}

bool HTTPGetFile(HINTERNET hConnect, const wchar_t *url, const wchar_t *outputPath, const wchar_t *extraHeaders,
		 int *responseCode)
{
	HttpHandle hRequest;

	const wchar_t *acceptTypes[] = {L"*/*", nullptr};

	URL_COMPONENTS urlComponents = {};
	bool secure = false;

	wchar_t hostName[256];
	wchar_t path[1024];

	/* -------------------------------------- *
	 * get URL components                     */

	urlComponents.dwStructSize = sizeof(urlComponents);

	urlComponents.lpszHostName = hostName;
	urlComponents.dwHostNameLength = _countof(hostName);

	urlComponents.lpszUrlPath = path;
	urlComponents.dwUrlPathLength = _countof(path);

	WinHttpCrackUrl(url, 0, 0, &urlComponents);

	if (urlComponents.nPort == 443)
		secure = true;

	/* -------------------------------------- *
	 * request data                           */

	hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, acceptTypes,
				      secure ? WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH : WINHTTP_FLAG_REFRESH);
	if (!hRequest) {
		*responseCode = -3;
		return false;
	}

	bool bResults =
		!!WinHttpSendRequest(hRequest, extraHeaders, extraHeaders ? -1 : 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

	/* -------------------------------------- *
	 * end request                            */

	if (bResults) {
		bResults = !!WinHttpReceiveResponse(hRequest, nullptr);
	} else {
		*responseCode = GetLastError();
		return false;
	}

	/* -------------------------------------- *
	 * get headers                            */

	wchar_t statusCode[8];
	DWORD statusCodeLen;

	statusCodeLen = sizeof(statusCode);
	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode,
				 &statusCodeLen, WINHTTP_NO_HEADER_INDEX)) {
		*responseCode = -4;
		return false;
	} else {
		statusCode[_countof(statusCode) - 1] = 0;
	}

	/* -------------------------------------- *
	 * read data                              */

	*responseCode = wcstoul(statusCode, nullptr, 10);

	/* are we supposed to return true here? */
	if (!bResults || *responseCode != 200)
		return true;

	BYTE buffer[READ_BUF_SIZE];
	DWORD dwSize, outSize;
	int lastPosition = 0;

	WinHandle updateFile = CreateFile(outputPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (!updateFile.Valid()) {
		*responseCode = -7;
		return false;
	}

	do {
		/* Check for available data. */
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			*responseCode = -8;
			return false;
		}

		dwSize = std::min(dwSize, (DWORD)sizeof(buffer));

		if (!WinHttpReadData(hRequest, (void *)buffer, dwSize, &outSize)) {
			*responseCode = -9;
			return false;
		} else {
			if (!outSize)
				break;

			if (!ReadHTTPFile(updateFile, buffer, outSize, responseCode))
				return false;

			int position = (int)(((float)completedFileSize / (float)totalFileSize) * 100.0f);
			if (position > lastPosition) {
				lastPosition = position;
				SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, position, 0);
			}
		}

		if (WaitForSingleObject(cancelRequested, 0) == WAIT_OBJECT_0) {
			*responseCode = -14;
			return false;
		}

	} while (dwSize > 0);

	return true;
}

bool HTTPGetBuffer(HINTERNET hConnect, const wchar_t *url, const wchar_t *extraHeaders, vector<std::byte> &out,
		   int *responseCode)
{
	HttpHandle hRequest;

	const wchar_t *acceptTypes[] = {L"*/*", nullptr};

	URL_COMPONENTS urlComponents = {};
	bool secure = false;

	wchar_t hostName[256];
	wchar_t path[1024];

	/* -------------------------------------- *
	 * get URL components                     */

	urlComponents.dwStructSize = sizeof(urlComponents);

	urlComponents.lpszHostName = hostName;
	urlComponents.dwHostNameLength = _countof(hostName);

	urlComponents.lpszUrlPath = path;
	urlComponents.dwUrlPathLength = _countof(path);

	WinHttpCrackUrl(url, 0, 0, &urlComponents);

	if (urlComponents.nPort == 443)
		secure = true;

	/* -------------------------------------- *
	 * request data                           */

	hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, acceptTypes,
				      secure ? WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH : WINHTTP_FLAG_REFRESH);
	if (!hRequest) {
		*responseCode = -3;
		return false;
	}

	bool bResults =
		!!WinHttpSendRequest(hRequest, extraHeaders, extraHeaders ? -1 : 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

	/* -------------------------------------- *
	 * end request                            */

	if (bResults) {
		bResults = !!WinHttpReceiveResponse(hRequest, nullptr);
	} else {
		*responseCode = GetLastError();
		return false;
	}

	/* -------------------------------------- *
	 * get headers                            */

	wchar_t statusCode[8];
	DWORD statusCodeLen;

	statusCodeLen = sizeof(statusCode);
	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode,
				 &statusCodeLen, WINHTTP_NO_HEADER_INDEX)) {
		*responseCode = -4;
		return false;
	} else {
		statusCode[_countof(statusCode) - 1] = 0;
	}

	/* -------------------------------------- *
	 * read data                              */

	*responseCode = wcstoul(statusCode, nullptr, 10);

	/* are we supposed to return true here? */
	if (!bResults || *responseCode != 200)
		return true;

	BYTE buffer[READ_BUF_SIZE];
	DWORD dwSize, outSize;
	int lastPosition = 0;

	do {
		/* Check for available data. */
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			*responseCode = -8;
			return false;
		}

		dwSize = std::min(dwSize, (DWORD)sizeof(buffer));

		if (!WinHttpReadData(hRequest, (void *)buffer, dwSize, &outSize)) {
			*responseCode = -9;
			return false;
		} else {
			if (!outSize)
				break;

			out.insert(out.end(), (std::byte *)buffer, (std::byte *)buffer + outSize);

			completedFileSize += outSize;
			int position = (int)(((float)completedFileSize / (float)totalFileSize) * 100.0f);
			if (position > lastPosition) {
				lastPosition = position;
				SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, position, 0);
			}
		}

		if (WaitForSingleObject(cancelRequested, 0) == WAIT_OBJECT_0) {
			*responseCode = -14;
			return false;
		}

	} while (dwSize > 0);

	return true;
}
