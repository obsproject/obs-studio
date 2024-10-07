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
#include "manifest.hpp"

#include <psapi.h>
#include <WinTrust.h>
#include <SoftPub.h>

#include <util/windows/CoTaskMemPtr.hpp>

#include <future>
#include <string>
#include <string_view>
#include <mutex>
#include <unordered_set>
#include <queue>

using namespace std;
using namespace updater;

/* ----------------------------------------------------------------------- */

constexpr const string_view kCDNUrl = "https://cdn-fastly.obsproject.com/";
constexpr const wchar_t *kCDNHostname = L"cdn-fastly.obsproject.com";
constexpr const wchar_t *kCDNUpdateBaseUrl = L"https://cdn-fastly.obsproject.com/update_studio";
constexpr const wchar_t *kPatchManifestURL = L"https://obsproject.com/update_studio/getpatchmanifest";
constexpr const wchar_t *kVSRedistURL = L"https://aka.ms/vs/17/release/vc_redist.x64.exe";
constexpr const wchar_t *kMSHostname = L"aka.ms";

/* ----------------------------------------------------------------------- */

HANDLE cancelRequested = nullptr;
HANDLE updateThread = nullptr;
HINSTANCE hinstMain = nullptr;
HWND hwndMain = nullptr;
HCRYPTPROV hProvider = 0;

static bool bExiting = false;
static bool updateFailed = false;

static bool downloadThreadFailure = false;

size_t totalFileSize = 0;
size_t completedFileSize = 0;
static int completedUpdates = 0;

static wchar_t tempPath[MAX_PATH];
static wchar_t obs_base_directory[MAX_PATH];

struct LastError {
	DWORD code;
	inline LastError() { code = GetLastError(); }
};

void FreeWinHttpHandle(HINTERNET handle)
{
	WinHttpCloseHandle(handle);
}

/* ----------------------------------------------------------------------- */

static bool IsVSRedistOutdated()
{
	VS_FIXEDFILEINFO *info = nullptr;
	UINT len = 0;
	vector<std::byte> buf;

	const wchar_t vc_dll[] = L"msvcp140";

	auto size = GetFileVersionInfoSize(vc_dll, nullptr);
	if (!size)
		return true;

	buf.resize(size);
	if (!GetFileVersionInfo(vc_dll, 0, size, buf.data()))
		return true;

	bool success = VerQueryValue(buf.data(), L"\\", reinterpret_cast<LPVOID *>(&info), &len);
	if (!success || !info || !len)
		return true;

	return LOWORD(info->dwFileVersionMS) < 40;
}

static void Status(const wchar_t *fmt, ...)
{
	wchar_t str[512];

	va_list argptr;
	va_start(argptr, fmt);

	StringCbVPrintf(str, sizeof(str), fmt, argptr);

	SetDlgItemText(hwndMain, IDC_STATUS, str);

	va_end(argptr);
}

static bool MyCopyFile(const wchar_t *src, const wchar_t *dest)
try {
	WinHandle hSrc;
	WinHandle hDest;

	hSrc = CreateFile(src, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
			  nullptr);
	if (!hSrc.Valid())
		throw LastError();

	hDest = CreateFile(dest, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (!hDest.Valid())
		throw LastError();

	BYTE buf[65536];
	DWORD read, wrote;

	for (;;) {
		if (!ReadFile(hSrc, buf, sizeof(buf), &read, nullptr))
			throw LastError();

		if (read == 0)
			break;

		if (!WriteFile(hDest, buf, read, &wrote, nullptr))
			throw LastError();

		if (wrote != read)
			return false;
	}

	return true;

} catch (LastError &error) {
	SetLastError(error.code);
	return false;
}

static void MyDeleteFile(const wstring &filename)
{
	/* Try straightforward delete first */
	if (DeleteFile(filename.c_str()))
		return;

	DWORD err = GetLastError();
	if (err == ERROR_FILE_NOT_FOUND)
		return;

	/* If all else fails, schedule the file to be deleted on reboot */
	MoveFileEx(filename.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
}

static bool IsSafeFilename(const wchar_t *path)
{
	const wchar_t *p = path;

	if (!*p)
		return false;

	if (wcsstr(path, L".."))
		return false;

	if (*p == '/')
		return false;

	while (*p) {
		if (!isalnum(*p) && *p != '.' && *p != '/' && *p != '_' && *p != '-')
			return false;
		p++;
	}

	return true;
}

static string QuickReadFile(const wchar_t *path)
{
	string data;

	WinHandle handle = CreateFileW(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (!handle.Valid()) {
		return {};
	}

	LARGE_INTEGER size;

	if (!GetFileSizeEx(handle, &size)) {
		return {};
	}

	data.resize((size_t)size.QuadPart);

	DWORD read;
	if (!ReadFile(handle, data.data(), (DWORD)data.size(), &read, nullptr)) {
		return {};
	}
	if (read != size.QuadPart) {
		return {};
	}

	return data;
}

static bool QuickWriteFile(const wchar_t *file, const void *data, size_t size)
try {
	WinHandle handle = CreateFile(file, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);

	if (handle == INVALID_HANDLE_VALUE)
		throw GetLastError();

	DWORD written;
	if (!WriteFile(handle, data, (DWORD)size, &written, nullptr))
		throw GetLastError();

	return true;

} catch (LastError &error) {
	SetLastError(error.code);
	return false;
}

/* ----------------------------------------------------------------------- */

/* Extend std::hash for B2Hash */
template<> struct std::hash<B2Hash> {
	size_t operator()(const B2Hash &value) const noexcept
	{
		return hash<string_view>{}(string_view(reinterpret_cast<const char *>(value.data()), value.size()));
	}
};

enum state_t {
	STATE_INVALID,
	STATE_PENDING_DOWNLOAD,
	STATE_DOWNLOADING,
	STATE_DOWNLOADED,
	STATE_ALREADY_DOWNLOADED,
	STATE_INSTALL_FAILED,
	STATE_INSTALLED,
};

struct update_t {
	wstring sourceURL;
	wstring outputPath;
	wstring previousFile;
	string packageName;

	B2Hash hash;
	B2Hash my_hash;
	B2Hash downloadHash;

	size_t fileSize = 0;
	state_t state = STATE_INVALID;
	bool has_hash = false;
	bool patchable = false;
	bool compressed = false;

	update_t() = default;

	update_t(const update_t &from) = default;

	update_t(update_t &&from) noexcept
		: sourceURL(std::move(from.sourceURL)),
		  outputPath(std::move(from.outputPath)),
		  previousFile(std::move(from.previousFile)),
		  packageName(std::move(from.packageName)),
		  hash(from.hash),
		  my_hash(from.my_hash),
		  downloadHash(from.downloadHash),
		  fileSize(from.fileSize),
		  state(from.state),
		  has_hash(from.has_hash),
		  patchable(from.patchable),
		  compressed(from.compressed)
	{
		from.state = STATE_INVALID;
	}

	void CleanPartialUpdate() const
	{
		if (state == STATE_INSTALL_FAILED || state == STATE_INSTALLED) {
			if (!previousFile.empty()) {
				DeleteFile(outputPath.c_str());
				MyCopyFile(previousFile.c_str(), outputPath.c_str());
				DeleteFile(previousFile.c_str());
			} else {
				DeleteFile(outputPath.c_str());
			}
		}
	}

	update_t &operator=(const update_t &from) = default;
};

struct deletion_t {
	wstring originalFilename;
	wstring deleteMeFilename;

	void UndoRename() const
	{
		if (!deleteMeFilename.empty())
			MoveFile(deleteMeFilename.c_str(), originalFilename.c_str());
	}
};

static unordered_map<B2Hash, vector<std::byte>> download_data;
static unordered_map<string, B2Hash> hashes;
static vector<update_t> updates;
static vector<deletion_t> deletions;
static mutex updateMutex;

static inline void CleanupPartialUpdates()
{
	for (update_t &update : updates)
		update.CleanPartialUpdate();

	for (deletion_t &deletion : deletions)
		deletion.UndoRename();
}

/* ----------------------------------------------------------------------- */

static int Decompress(ZSTD_DCtx *ctx, std::vector<std::byte> &buf, size_t size)
{
	// Copy compressed data
	vector<std::byte> comp(buf.begin(), buf.end());

	try {
		buf.resize(size);
	} catch (...) {
		return -1;
	}

	// Overwrite buffer with decompressed data
	size_t result = ZSTD_decompressDCtx(ctx, buf.data(), buf.size(), comp.data(), comp.size());

	if (result != size)
		return -9;
	if (ZSTD_isError(result))
		return -10;

	return 0;
}

bool DownloadWorkerThread()
{
	const DWORD tlsProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;

	const DWORD enableHTTP2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;

	const DWORD compressionFlags = WINHTTP_DECOMPRESSION_FLAG_ALL;

	HttpHandle hSession = WinHttpOpen(L"OBS Studio Updater/3.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
					  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		downloadThreadFailure = true;
		Status(L"Update failed: Couldn't open obsproject.com");
		return false;
	}

	WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, (LPVOID)&tlsProtocols, sizeof(tlsProtocols));

	WinHttpSetOption(hSession, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, (LPVOID)&enableHTTP2Flag,
			 sizeof(enableHTTP2Flag));

	WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, (LPVOID)&compressionFlags, sizeof(compressionFlags));

	HttpHandle hConnect = WinHttpConnect(hSession, kCDNHostname, INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) {
		downloadThreadFailure = true;
		Status(L"Update failed: Couldn't connect to %S", kCDNHostname);
		return false;
	}

	ZSTDDCtx zCtx;

	for (;;) {
		bool foundWork = false;

		unique_lock<mutex> ulock(updateMutex);

		for (update_t &update : updates) {
			int responseCode;

			DWORD waitResult = WaitForSingleObject(cancelRequested, 0);
			if (waitResult == WAIT_OBJECT_0) {
				return false;
			}

			if (update.state != STATE_PENDING_DOWNLOAD)
				continue;

			update.state = STATE_DOWNLOADING;

			ulock.unlock();

			foundWork = true;

			if (downloadThreadFailure) {
				return false;
			}

			auto &buf = download_data[update.downloadHash];
			/* Reserve required memory */
			buf.reserve(update.fileSize);

			if (!HTTPGetBuffer(hConnect, update.sourceURL.c_str(), L"Accept-Encoding: gzip", buf,
					   &responseCode)) {
				downloadThreadFailure = true;
				Status(L"Update failed: Could not download "
				       L"%s (error code %d)",
				       update.outputPath.c_str(), responseCode);
				return true;
			}

			if (responseCode != 200) {
				downloadThreadFailure = true;
				Status(L"Update failed: Could not download "
				       L"%s (error code %d)",
				       update.outputPath.c_str(), responseCode);
				return true;
			}

			/* Validate hash of downloaded data. */
			B2Hash dataHash;
			blake2b(dataHash.data(), dataHash.size(), buf.data(), buf.size(), nullptr, 0);

			if (dataHash != update.downloadHash) {
				downloadThreadFailure = true;
				Status(L"Update failed: Integrity check "
				       L"failed on %s",
				       update.outputPath.c_str());
				return true;
			}

			/* Decompress data in compressed buffer. */
			if (update.compressed && !update.patchable) {
				int res = Decompress(zCtx, buf, update.fileSize);
				if (res) {
					downloadThreadFailure = true;
					Status(L"Update failed: Decompression "
					       L"failed on %s (error code %d)",
					       update.outputPath.c_str(), res);
					return true;
				}
			}

			ulock.lock();

			update.state = STATE_DOWNLOADED;
			completedUpdates++;
		}

		if (!foundWork) {
			break;
		}
		if (downloadThreadFailure) {
			return false;
		}
	}

	return true;
}

static bool RunDownloadWorkers(int num)
try {
	vector<future<bool>> thread_success_results;
	thread_success_results.resize(num);

	for (future<bool> &result : thread_success_results) {
		result = async(DownloadWorkerThread);
	}
	for (future<bool> &result : thread_success_results) {
		if (!result.get()) {
			return false;
		}
	}

	return true;

} catch (...) {
	return false;
}

/* ----------------------------------------------------------------------- */

enum { WAITIFOBS_SUCCESS, WAITIFOBS_WRONG_PROCESS, WAITIFOBS_CANCELLED };

static inline DWORD WaitIfOBS(DWORD id, const wchar_t *expected)
{
	wchar_t path[MAX_PATH];
	wchar_t *name;
	DWORD path_len = _countof(path);

	*path = 0;

	WinHandle proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, false, id);
	if (!proc.Valid())
		return WAITIFOBS_WRONG_PROCESS;

	if (!QueryFullProcessImageNameW(proc, 0, path, &path_len))
		return WAITIFOBS_WRONG_PROCESS;

	// check it's actually our exe that's running
	size_t len = wcslen(obs_base_directory);
	if (wcsncmp(path, obs_base_directory, len) != 0)
		return WAITIFOBS_WRONG_PROCESS;

	name = wcsrchr(path, L'\\');
	if (name)
		name += 1;
	else
		name = path;

	if (_wcsnicmp(name, expected, 5) == 0) {
		HANDLE hWait[2];
		hWait[0] = proc;
		hWait[1] = cancelRequested;

		int i = WaitForMultipleObjects(2, hWait, false, INFINITE);
		if (i == WAIT_OBJECT_0 + 1)
			return WAITIFOBS_CANCELLED;

		return WAITIFOBS_SUCCESS;
	}

	return WAITIFOBS_WRONG_PROCESS;
}

static bool WaitForOBS()
{
	DWORD proc_ids[1024], needed, count;

	if (!EnumProcesses(proc_ids, sizeof(proc_ids), &needed)) {
		return true;
	}

	count = needed / sizeof(DWORD);

	for (DWORD i = 0; i < count; i++) {
		DWORD id = proc_ids[i];
		if (id != 0) {
			switch (WaitIfOBS(id, L"obs64")) {
			case WAITIFOBS_SUCCESS:
				return true;
			case WAITIFOBS_WRONG_PROCESS:
				break;
			case WAITIFOBS_CANCELLED:
				return false;
			}
		}
	}

	return true;
}

/* ----------------------------------------------------------------------- */

static inline bool UTF8ToWide(wchar_t *wide, int wideSize, const char *utf8)
{
	return !!MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, wideSize);
}

static inline bool WideToUTF8(char *utf8, int utf8Size, const wchar_t *wide)
{
	return !!WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8Size, nullptr, nullptr);
}

#define UTF8ToWideBuf(wide, utf8) UTF8ToWide(wide, _countof(wide), utf8)
#define WideToUTF8Buf(utf8, wide) WideToUTF8(utf8, _countof(utf8), wide)

/* ----------------------------------------------------------------------- */

queue<string> hashQueue;

void HasherThread()
{
	unique_lock ulock(updateMutex, defer_lock);

	while (true) {
		ulock.lock();
		if (hashQueue.empty())
			return;

		auto fileName = hashQueue.front();
		hashQueue.pop();

		ulock.unlock();

		wchar_t updateFileName[MAX_PATH];

		if (!UTF8ToWideBuf(updateFileName, fileName.c_str()))
			continue;
		if (!IsSafeFilename(updateFileName))
			continue;

		B2Hash existingHash;
		if (CalculateFileHash(updateFileName, existingHash)) {
			ulock.lock();
			hashes.emplace(fileName, existingHash);
			ulock.unlock();
		}
	}
}

static void RunHasherWorkers(int num, const vector<Package> &packages)
try {

	for (const Package &package : packages) {
		for (const File &file : package.files) {
			hashQueue.push(file.name);
		}
	}

	vector<future<void>> futures;
	futures.resize(num);

	for (auto &result : futures) {
		result = async(launch::async, HasherThread);
	}
	for (auto &result : futures) {
		result.wait();
	}
} catch (...) {
}

/* ----------------------------------------------------------------------- */

static inline bool FileExists(const wchar_t *path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;

	hFind = FindFirstFileW(path, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	return hFind != INVALID_HANDLE_VALUE;
}

static bool NonCorePackageInstalled(const char *name)
{
	if (strcmp(name, "obs-browser") == 0)
		return FileExists(L"obs-plugins\\64bit\\obs-browser.dll");

	return false;
}

static bool AddPackageUpdateFiles(const Package &package, const wchar_t *branch)
{
	wchar_t wPackageName[512];
	if (!UTF8ToWideBuf(wPackageName, package.name.c_str()))
		return false;

	if (package.name != "core" && !NonCorePackageInstalled(package.name.c_str()))
		return true;

	for (const File &file : package.files) {
		if (file.hash.size() != kBlake2StrLength)
			continue;

		/* The download hash may not exist if a file is uncompressed */

		bool compressed = false;
		if (file.compressed_hash.size() == kBlake2StrLength)
			compressed = true;

		/* convert strings to wide */

		wchar_t sourceURL[1024];
		wchar_t updateFileName[MAX_PATH];

		if (!UTF8ToWideBuf(updateFileName, file.name.c_str()))
			continue;

		/* make sure paths are safe */

		if (!IsSafeFilename(updateFileName)) {
			Status(L"Update failed: Unsafe path '%s' found in "
			       L"manifest",
			       updateFileName);
			return false;
		}

		StringCbPrintf(sourceURL, sizeof(sourceURL), L"%s/%s/%s/%s", kCDNUpdateBaseUrl, branch, wPackageName,
			       updateFileName);

		/* Convert hashes */
		B2Hash updateHash;
		StringToHash(file.hash, updateHash);

		/* We don't really care if this fails, it's just to avoid
		 * wasting bandwidth by downloading unmodified files */
		B2Hash localFileHash;
		bool has_hash = false;

		if (hashes.count(file.name)) {
			localFileHash = hashes[file.name];
			if (localFileHash == updateHash)
				continue;

			has_hash = true;
		}

		/* Add update file */
		update_t update;
		update.fileSize = file.size;
		update.outputPath = updateFileName;
		update.sourceURL = sourceURL;
		update.packageName = package.name;
		update.state = STATE_PENDING_DOWNLOAD;
		update.patchable = false;
		update.compressed = compressed;
		update.hash = updateHash;

		if (compressed) {
			update.sourceURL += L".zst";
			StringToHash(file.compressed_hash, update.downloadHash);
		} else {
			update.downloadHash = updateHash;
		}

		update.has_hash = has_hash;
		if (has_hash)
			update.my_hash = localFileHash;

		updates.push_back(std::move(update));

		totalFileSize += file.size;
	}

	return true;
}

static void AddPackageRemovedFiles(const Package &package)
{
	for (const string &filename : package.removed_files) {
		wchar_t removedFileName[MAX_PATH];
		if (!UTF8ToWideBuf(removedFileName, filename.c_str()))
			continue;

		/* Ensure paths are safe, also check if file exists */
		if (!IsSafeFilename(removedFileName))
			continue;
		/* Technically GetFileAttributes can fail for other reasons,
		 * so double-check by also checking the last error */
		if (GetFileAttributesW(removedFileName) == INVALID_FILE_ATTRIBUTES) {
			int err = GetLastError();
			if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
				continue;
		}

		deletion_t deletion;
		deletion.originalFilename = removedFileName;

		deletions.push_back(deletion);
	}
}

static bool RenameRemovedFile(deletion_t &deletion)
{
	_TCHAR deleteMeName[MAX_PATH];
	_TCHAR randomStr[MAX_PATH];

	BYTE junk[40];
	B2Hash hash;
	string temp;

	CryptGenRandom(hProvider, sizeof(junk), junk);
	blake2b(hash.data(), hash.size(), junk, sizeof(junk), nullptr, 0);
	HashToString(hash, temp);

	if (!UTF8ToWideBuf(randomStr, temp.c_str()))
		return false;

	randomStr[8] = 0;

	StringCbCopy(deleteMeName, sizeof(deleteMeName), deletion.originalFilename.c_str());

	StringCbCat(deleteMeName, sizeof(deleteMeName), L".");
	StringCbCat(deleteMeName, sizeof(deleteMeName), randomStr);
	StringCbCat(deleteMeName, sizeof(deleteMeName), L".deleteme");

	if (MoveFile(deletion.originalFilename.c_str(), deleteMeName)) {
		/* Only set this if the file was successfully renamed */
		deletion.deleteMeFilename = deleteMeName;
		return true;
	}

	return false;
}

static void UpdateWithPatchIfAvailable(const PatchResponse &patch)
{
	wchar_t widePatchableFilename[MAX_PATH];
	wchar_t sourceURL[1024];

	if (patch.source.compare(0, kCDNUrl.size(), kCDNUrl) != 0)
		return;

	if (patch.name.find('/') == string::npos)
		return;

	string patchPackageName(patch.name, 0, patch.name.find('/'));
	string fileName(patch.name, patch.name.find('/') + 1);

	if (!UTF8ToWideBuf(widePatchableFilename, fileName.c_str()))
		return;
	if (!UTF8ToWideBuf(sourceURL, patch.source.c_str()))
		return;

	for (update_t &update : updates) {
		if (update.packageName != patchPackageName)
			continue;
		if (update.outputPath != widePatchableFilename)
			continue;

		update.patchable = true;

		/* Replace the source URL with the patch file, update
	         * the download hash, and re-calculate download size */
		StringToHash(patch.hash, update.downloadHash);
		update.sourceURL = sourceURL;
		totalFileSize -= (update.fileSize - patch.size);
		update.fileSize = patch.size;

		break;
	}
}

static bool MoveInUseFileAway(const update_t &file)
{
	_TCHAR deleteMeName[MAX_PATH];
	_TCHAR randomStr[MAX_PATH];

	BYTE junk[40];
	B2Hash hash;
	string temp;

	CryptGenRandom(hProvider, sizeof(junk), junk);
	blake2b(hash.data(), hash.size(), junk, sizeof(junk), nullptr, 0);
	HashToString(hash, temp);

	if (!UTF8ToWideBuf(randomStr, temp.c_str()))
		return false;

	randomStr[8] = 0;

	StringCbCopy(deleteMeName, sizeof(deleteMeName), file.outputPath.c_str());

	StringCbCat(deleteMeName, sizeof(deleteMeName), L".");
	StringCbCat(deleteMeName, sizeof(deleteMeName), randomStr);
	StringCbCat(deleteMeName, sizeof(deleteMeName), L".deleteme");

	if (MoveFile(file.outputPath.c_str(), deleteMeName)) {

		if (MyCopyFile(deleteMeName, file.outputPath.c_str())) {
			MoveFileEx(deleteMeName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

			return true;
		} else {
			MoveFile(deleteMeName, file.outputPath.c_str());
		}
	}

	return false;
}

static bool UpdateFile(ZSTD_DCtx *ctx, update_t &file)
{
	wchar_t oldFileRenamedPath[MAX_PATH];

	/* Grab the patch/file data from the global cache. */
	vector<std::byte> &patch_data = download_data[file.downloadHash];

	/* Check if we're replacing an existing file or just installing a new
	 * one */
	DWORD attribs = GetFileAttributes(file.outputPath.c_str());

	if (attribs != INVALID_FILE_ATTRIBUTES) {
		wchar_t baseName[MAX_PATH];

		StringCbCopy(baseName, sizeof(baseName), file.outputPath.c_str());

		wchar_t *curFileName = wcsrchr(baseName, '/');
		if (curFileName) {
			curFileName[0] = '\0';
			curFileName++;
		} else
			curFileName = baseName;

		/* Backup the existing file in case a rollback is needed */
		StringCbCopy(oldFileRenamedPath, sizeof(oldFileRenamedPath), file.outputPath.c_str());
		StringCbCat(oldFileRenamedPath, sizeof(oldFileRenamedPath), L".old");

		if (!MyCopyFile(file.outputPath.c_str(), oldFileRenamedPath)) {
			DWORD err = GetLastError();
			int is_sharing_violation = (err == ERROR_SHARING_VIOLATION || err == ERROR_USER_MAPPED_FILE);

			if (is_sharing_violation)
				Status(L"Update failed: %s is still in use.  "
				       L"Close all programs and try again.",
				       curFileName);
			else
				Status(L"Update failed: Couldn't backup %s "
				       L"(error %d)",
				       curFileName, GetLastError());
			return false;
		}

		file.previousFile = oldFileRenamedPath;

		int error_code;
		bool installed_ok;
		bool already_tried_to_move = false;

	retryAfterMovingFile:

		if (file.patchable) {
			error_code = ApplyPatch(ctx, patch_data.data(), file.fileSize, file.outputPath.c_str());

			installed_ok = (error_code == 0);

			if (installed_ok) {
				B2Hash patchedFileHash;
				if (!CalculateFileHash(file.outputPath.c_str(), patchedFileHash)) {
					Status(L"Update failed: Couldn't "
					       L"verify integrity of patched %s",
					       curFileName);

					file.state = STATE_INSTALL_FAILED;
					return false;
				}

				if (file.hash != patchedFileHash) {
					Status(L"Update failed: Integrity "
					       L"check of patched "
					       L"%s failed",
					       curFileName);

					file.state = STATE_INSTALL_FAILED;
					return false;
				}
			}
		} else {
			installed_ok = QuickWriteFile(file.outputPath.c_str(), patch_data.data(), patch_data.size());
			error_code = GetLastError();
		}

		if (!installed_ok) {
			int is_sharing_violation =
				(error_code == ERROR_SHARING_VIOLATION || error_code == ERROR_USER_MAPPED_FILE);

			if (is_sharing_violation) {
				if (!already_tried_to_move) {
					already_tried_to_move = true;

					if (MoveInUseFileAway(file))
						goto retryAfterMovingFile;
				}

				Status(L"Update failed: %s is still in use.  "
				       L"Close all "
				       L"programs and try again.",
				       curFileName);
			} else {
				DWORD err = GetLastError();
				Status(L"Update failed: Couldn't update %s "
				       L"(error %d)",
				       curFileName, err ? err : error_code);
			}

			file.state = STATE_INSTALL_FAILED;
			return false;
		}

		file.state = STATE_INSTALLED;
	} else {
		if (file.patchable) {
			/* Uh oh, we thought we could patch something but it's
			 * no longer there! */
			Status(L"Update failed: Source file %s not found", file.outputPath.c_str());
			return false;
		}

		/* We may be installing into new folders,
		 * make sure they exist */
		filesystem::path filePath(file.outputPath.c_str());
		create_directories(filePath.parent_path());

		file.previousFile = L"";

		bool success = !!QuickWriteFile(file.outputPath.c_str(), patch_data.data(), patch_data.size());
		if (!success) {
			Status(L"Update failed: Couldn't install %s (error %d)", file.outputPath.c_str(),
			       GetLastError());
			file.state = STATE_INSTALL_FAILED;
			return false;
		}

		file.state = STATE_INSTALLED;
	}

	return true;
}

queue<reference_wrapper<update_t>> updateQueue;
static int lastPosition = 0;
static int installed = 0;
static bool updateThreadFailed = false;

static bool UpdateWorker()
{
	unique_lock<mutex> ulock(updateMutex, defer_lock);
	ZSTDDCtx zCtx;

	while (true) {
		ulock.lock();

		if (updateThreadFailed)
			return false;
		if (updateQueue.empty())
			break;

		auto update = updateQueue.front();
		updateQueue.pop();
		ulock.unlock();

		if (!UpdateFile(zCtx, update)) {
			updateThreadFailed = true;
			return false;
		} else {
			int position = (int)(((float)++installed / (float)completedUpdates) * 100.0f);
			if (position > lastPosition) {
				lastPosition = position;
				SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, position, 0);
			}
		}
	}

	return true;
}

static bool RunUpdateWorkers(int num)
try {
	for (update_t &update : updates)
		updateQueue.emplace(update);

	vector<future<bool>> thread_success_results;
	thread_success_results.resize(num);

	for (future<bool> &result : thread_success_results) {
		result = async(launch::async, UpdateWorker);
	}
	for (future<bool> &result : thread_success_results) {
		if (!result.get()) {
			return false;
		}
	}

	return true;

} catch (...) {
	return false;
}

static bool UpdateVSRedists()
{
	/* ------------------------------------------ *
	 * Initialize session                         */

	const DWORD tlsProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;

	const DWORD compressionFlags = WINHTTP_DECOMPRESSION_FLAG_ALL;

	HttpHandle hSession = WinHttpOpen(L"OBS Studio Updater/3.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
					  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		Status(L"VC Redist Update failed: Couldn't create session");
		return false;
	}

	WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, (LPVOID)&tlsProtocols, sizeof(tlsProtocols));

	WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, (LPVOID)&compressionFlags, sizeof(compressionFlags));

	HttpHandle hConnect = WinHttpConnect(hSession, kMSHostname, INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) {
		Status(L"Update failed: Couldn't connect to %S", kMSHostname);
		return false;
	}

	int responseCode;

	DWORD waitResult = WaitForSingleObject(cancelRequested, 0);
	if (waitResult == WAIT_OBJECT_0) {
		return false;
	}

	/* ------------------------------------------ *
	 * Download redist                            */

	Status(L"Downloading Visual C++ Redistributable");

	wstring destPath;
	destPath += tempPath;
	destPath += L"\\VC_redist.x64.exe";

	if (!HTTPGetFile(hConnect, kVSRedistURL, destPath.c_str(), L"Accept-Encoding: gzip", &responseCode)) {

		DeleteFile(destPath.c_str());
		Status(L"Update failed: Could not download "
		       L"%s (error code %d)",
		       L"Visual C++ Redistributable", responseCode);
		return false;
	}

	/* ------------------------------------------ *
	 * Verify file signature                      */

	GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	WINTRUST_FILE_INFO fileInfo = {};
	fileInfo.cbStruct = sizeof(fileInfo);
	fileInfo.pcwszFilePath = destPath.c_str();

	WINTRUST_DATA data = {};
	data.cbStruct = sizeof(data);
	data.dwUIChoice = WTD_UI_NONE;
	data.dwUnionChoice = WTD_CHOICE_FILE;
	data.dwStateAction = WTD_STATEACTION_VERIFY;
	data.pFile = &fileInfo;

	LONG result = WinVerifyTrust(nullptr, &action, &data);

	if (result != ERROR_SUCCESS) {
		Status(L"Update failed: Signature verification failed for "
		       L"%s (error code %d / %d)",
		       L"Visual C++ Redistributable", result, GetLastError());
		DeleteFile(destPath.c_str());
		return false;
	}

	/* ------------------------------------------ *
	 * If verification succeeded, install redist  */

	wchar_t commandline[MAX_PATH + MAX_PATH];
	StringCbPrintf(commandline, sizeof(commandline), L"%s /install /quiet /norestart", destPath.c_str());

	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {};
	si.cb = sizeof(si);

	bool success = !!CreateProcessW(destPath.c_str(), commandline, nullptr, nullptr, false, CREATE_NO_WINDOW,
					nullptr, nullptr, &si, &pi);
	if (success) {
		Status(L"Installing %s...", L"Visual C++ Redistributable");

		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
	} else {
		Status(L"Update failed: Could not execute "
		       L"%s (error code %d)",
		       L"Visual C++ Redistributable", (int)GetLastError());
	}

	DeleteFile(destPath.c_str());

	waitResult = WaitForSingleObject(cancelRequested, 0);
	if (waitResult == WAIT_OBJECT_0) {
		return false;
	}

	return success;
}

static void UpdateRegistryVersion(const Manifest &manifest)
{
	const char *regKey = R"(Software\Microsoft\Windows\CurrentVersion\Uninstall\OBS Studio)";
	LSTATUS res;
	HKEY key;
	char version[32];
	int formattedLen;

	/* The manifest does not store a version string, so we gotta make one ourselves. */
	if (manifest.beta || manifest.rc) {
		formattedLen = sprintf_s(version, sizeof(version), "%d.%d.%d-%s%d", manifest.version_major,
					 manifest.version_minor, manifest.version_patch, manifest.beta ? "beta" : "rc",
					 manifest.beta ? manifest.beta : manifest.rc);
	} else {
		formattedLen = sprintf_s(version, sizeof(version), "%d.%d.%d", manifest.version_major,
					 manifest.version_minor, manifest.version_patch);
	}

	if (formattedLen <= 0)
		return;

	res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regKey, 0, KEY_WRITE | KEY_WOW64_32KEY, &key);
	if (res != ERROR_SUCCESS)
		return;

	RegSetValueExA(key, "DisplayVersion", 0, REG_SZ, (const BYTE *)version, formattedLen + 1);
	RegCloseKey(key);
}

static void ClearShaderCache()
{
	wchar_t shader_path[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, shader_path);
	StringCbCatW(shader_path, sizeof(shader_path), L"\\obs-studio\\shader-cache");
	filesystem::remove_all(shader_path);
}

extern "C" void UpdateHookFiles(void);

static bool Update(wchar_t *cmdLine)
{
	/* ------------------------------------- *
	 * Check to make sure OBS isn't running  */

	HANDLE hObsUpdateMutex = OpenMutexW(SYNCHRONIZE, false, L"OBSStudioUpdateMutex");
	if (hObsUpdateMutex) {
		HANDLE hWait[2];
		hWait[0] = hObsUpdateMutex;
		hWait[1] = cancelRequested;

		int i = WaitForMultipleObjects(2, hWait, false, INFINITE);

		if (i == WAIT_OBJECT_0)
			ReleaseMutex(hObsUpdateMutex);

		CloseHandle(hObsUpdateMutex);

		if (i == WAIT_OBJECT_0 + 1)
			return false;
	}

	if (!WaitForOBS())
		return false;

	/* ------------------------------------- *
	 * Init crypt stuff                      */

	CryptProvider hProvider;
	if (!CryptAcquireContext(&hProvider, nullptr, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		SetDlgItemTextW(hwndMain, IDC_STATUS, L"Update failed: CryptAcquireContext failure");
		return false;
	}

	::hProvider = hProvider;

	/* ------------------------------------- */

	SetDlgItemTextW(hwndMain, IDC_STATUS, L"Searching for available updates...");

	HWND hProgress = GetDlgItem(hwndMain, IDC_PROGRESS);
	LONG_PTR style = GetWindowLongPtr(hProgress, GWL_STYLE);
	SetWindowLongPtr(hProgress, GWL_STYLE, style | PBS_MARQUEE);

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETMARQUEE, 1, 0);

	/* ------------------------------------- *
	 * Check if updating portable build      */

	bool bIsPortable = false;
	wstring branch = L"stable";
	wstring appdata;

	if (cmdLine[0]) {
		int argc;
		LPWSTR *argv = CommandLineToArgvW(cmdLine, &argc);

		if (argv) {
			for (int i = 0; i < argc; i++) {
				if (wcscmp(argv[i], L"Portable") == 0) {
					// Legacy OBS
					bIsPortable = true;
					break;
				} else if (wcsncmp(argv[i], L"--branch=", 9) == 0) {
					branch = argv[i] + 9;
				} else if (wcsncmp(argv[i], L"--appdata=", 10) == 0) {
					appdata = argv[i] + 10;
				} else if (wcscmp(argv[i], L"--portable") == 0) {
					bIsPortable = true;
				} else if (wcsncmp(argv[i], L"--portable--branch=", 19) == 0) {
					/* Versions pre-29.1 beta 2 produce broken parameters :( */
					bIsPortable = true;
					branch = argv[i] + 19;
				}
			}
			LocalFree((HLOCAL)argv);
		}
	}

	/* ------------------------------------- *
	 * Get config path                       */

	wchar_t lpAppDataPath[MAX_PATH];
	lpAppDataPath[0] = 0;

	if (bIsPortable) {
		StringCbCopy(lpAppDataPath, sizeof(lpAppDataPath), obs_base_directory);
		StringCbCat(lpAppDataPath, sizeof(lpAppDataPath), L"\\config");
	} else {
		if (!appdata.empty()) {
			HRESULT hr = StringCbCopy(lpAppDataPath, sizeof(lpAppDataPath), appdata.c_str());
			if (hr != S_OK) {
				Status(L"Update failed: Could not determine AppData "
				       L"location");
				return false;
			}
		} else {
			CoTaskMemPtr<wchar_t> pOut;
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pOut);
			if (hr != S_OK) {
				Status(L"Update failed: Could not determine AppData "
				       L"location");
				return false;
			}

			StringCbCopy(lpAppDataPath, sizeof(lpAppDataPath), pOut);
		}
	}

	StringCbCat(lpAppDataPath, sizeof(lpAppDataPath), L"\\obs-studio");

	/* ------------------------------------- *
	 * Get download path                     */

	wchar_t manifestPath[MAX_PATH];
	wchar_t tempDirName[MAX_PATH];

	manifestPath[0] = 0;
	tempDirName[0] = 0;

	StringCbPrintf(manifestPath, sizeof(manifestPath), L"%s\\updates\\manifest.json", lpAppDataPath);
	if (!GetTempPathW(_countof(tempDirName), tempDirName)) {
		Status(L"Update failed: Failed to get temp path: %ld", GetLastError());
		return false;
	}
	if (!GetTempFileNameW(tempDirName, L"obs-studio", 0, tempPath)) {
		Status(L"Update failed: Failed to create temp dir name: %ld", GetLastError());
		return false;
	}

	DeleteFile(tempPath);
	CreateDirectory(tempPath, nullptr);

	/* ------------------------------------- *
	 * Load manifest file                    */

	Manifest manifest;
	{
		string manifestFile = QuickReadFile(manifestPath);
		if (manifestFile.empty()) {
			Status(L"Update failed: Couldn't load manifest file");
			return false;
		}

		try {
			json manifestContents = json::parse(manifestFile);
			manifest = manifestContents.get<Manifest>();
		} catch (json::exception &e) {
			Status(L"Update failed: Couldn't parse update "
			       L"manifest: %S",
			       e.what());
			return false;
		}
	}

	/* ------------------------------------- *
	 * Hash local files listed in manifest   */

	RunHasherWorkers(4, manifest.packages);

	/* ------------------------------------- *
	 * Parse current manifest update files   */

	for (const Package &package : manifest.packages) {
		if (!AddPackageUpdateFiles(package, branch.c_str())) {
			Status(L"Update failed: Failed to process update packages");
			return false;
		}

		/* Add removed files to deletion queue (if any) */
		AddPackageRemovedFiles(package);
	}

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETMARQUEE, 0, 0);
	SetWindowLongPtr(hProgress, GWL_STYLE, style);

	/* ------------------------------------- *
	 * Exit if updates already installed     */

	if (updates.empty()) {
		Status(L"All available updates are already installed.");
		SetDlgItemText(hwndMain, IDC_BUTTON, L"Launch OBS");
		return true;
	}

	/* ------------------------------------- *
	 * Check VS redistributables version     */

	if (IsVSRedistOutdated()) {
		if (!UpdateVSRedists()) {
			return false;
		}
	}

	/* ------------------------------------- *
	 * Generate file hash json               */

	PatchesRequest files;
	for (update_t &update : updates) {
		if (!update.has_hash)
			continue;

		char outputPath[MAX_PATH];
		if (!WideToUTF8Buf(outputPath, update.outputPath.c_str()))
			continue;

		string hash_string;
		HashToString(update.my_hash, hash_string);

		string package_path;
		package_path = update.packageName;
		package_path += "/";
		package_path += outputPath;

		files.push_back({package_path, hash_string});
	}

	/* ------------------------------------- *
	 * Send file hashes                      */

	string newManifest;
	if (!files.empty()) {
		json request = files;
		string post_body = request.dump();

		int len = (int)post_body.size();
		size_t compressSize = ZSTD_compressBound(len);
		string compressedJson;

		compressedJson.resize(compressSize);

		size_t result = ZSTD_compress(compressedJson.data(), compressedJson.size(), post_body.data(),
					      post_body.size(), ZSTD_CLEVEL_DEFAULT);

		if (ZSTD_isError(result))
			return false;

		compressedJson.resize(result);

		wstring manifestUrl(kPatchManifestURL);
		if (branch != L"stable")
			manifestUrl += L"?branch=" + branch;

		int responseCode;
		bool success = !!HTTPPostData(manifestUrl.c_str(), (BYTE *)compressedJson.data(),
					      (int)compressedJson.size(), L"Accept-Encoding: gzip", &responseCode,
					      newManifest);

		if (!success)
			return false;

		if (responseCode != 200) {
			Status(L"Update failed: HTTP/%d while trying to "
			       L"download patch manifest",
			       responseCode);
			return false;
		}
	} else {
		newManifest = "[]";
	}

	/* ------------------------------------- *
	 * Parse new manifest                    */

	PatchesResponse patches;
	try {
		json patchManifest = json::parse(newManifest);
		patches = patchManifest.get<PatchesResponse>();
	} catch (json::exception &e) {
		Status(L"Update failed: Couldn't parse patch manifest: %S", e.what());
		return false;
	}

	/* Update updates with patch information. */
	for (const PatchResponse &patch : patches) {
		UpdateWithPatchIfAvailable(patch);
	}

	/* ------------------------------------- *
	 * Deduplicate Downloads                 */

	unordered_set<B2Hash> downloadHashes;
	for (update_t &update : updates) {
		if (downloadHashes.count(update.downloadHash)) {
			update.state = STATE_ALREADY_DOWNLOADED;
			totalFileSize -= update.fileSize;
			completedUpdates++;
		} else {
			downloadHashes.insert(update.downloadHash);
		}
	}

	/* ------------------------------------- *
	 * Download Updates                      */

	Status(L"Downloading updates...");
	if (!RunDownloadWorkers(4))
		return false;

	if ((size_t)completedUpdates != updates.size()) {
		Status(L"Update failed to download all files.");
		return false;
	}

	/* ------------------------------------- *
	 * Install updates                       */

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, 0, 0);

	Status(L"Installing updates...");
	if (!RunUpdateWorkers(4))
		return false;

	for (deletion_t &deletion : deletions) {
		if (!RenameRemovedFile(deletion)) {
			Status(L"Update failed: Couldn't remove "
			       L"obsolete files");
			return false;
		}
	}

	/* ------------------------------------- *
	 * Install virtual camera                */

	auto runcommand = [](wchar_t *cmd) {
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;
		bool success = !!CreateProcessW(nullptr, cmd, nullptr, nullptr, false, CREATE_NEW_CONSOLE, nullptr,
						nullptr, &si, &pi);
		if (success) {
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	};

	if (!bIsPortable) {
		Status(L"Installing Virtual Camera...");
		wchar_t regsvr[MAX_PATH];
		wchar_t src[MAX_PATH];
		wchar_t tmp[MAX_PATH];
		wchar_t tmp2[MAX_PATH];

		SHGetFolderPathW(nullptr, CSIDL_SYSTEM, nullptr, SHGFP_TYPE_CURRENT, regsvr);
		StringCbCat(regsvr, sizeof(regsvr), L"\\regsvr32.exe");

		StringCbCopy(src, sizeof(src), obs_base_directory);
		StringCbCat(src, sizeof(src), L"\\data\\obs-plugins\\win-dshow\\");

		StringCbCopy(tmp, sizeof(tmp), L"\"");
		StringCbCat(tmp, sizeof(tmp), regsvr);
		StringCbCat(tmp, sizeof(tmp), L"\" /s \"");
		StringCbCat(tmp, sizeof(tmp), src);
		StringCbCat(tmp, sizeof(tmp), L"obs-virtualcam-module");

		StringCbCopy(tmp2, sizeof(tmp2), tmp);
		StringCbCat(tmp2, sizeof(tmp2), L"32.dll\"");
		runcommand(tmp2);

		StringCbCopy(tmp2, sizeof(tmp2), tmp);
		StringCbCat(tmp2, sizeof(tmp2), L"64.dll\"");
		runcommand(tmp2);
	}

	/* ------------------------------------- *
	 * Update hook files and vulkan registry */

	Status(L"Updating Game Capture hooks...");
	UpdateHookFiles();

	/* ------------------------------------- *
	 * Clear shader cache                    */

	Status(L"Clearing shader cache...");
	ClearShaderCache();

	/* ------------------------------------- *
	 * Update installed version in registry  */

	if (!bIsPortable) {
		Status(L"Updating version information...");
		UpdateRegistryVersion(manifest);
	}

	/* ------------------------------------- *
	 * Finish                                */

	Status(L"Cleaning up...");
	/* If we get here, all updates installed successfully so we can purge
	 * the old versions */
	for (update_t &update : updates) {
		if (!update.previousFile.empty())
			DeleteFile(update.previousFile.c_str());
	}

	/* Delete all removed files mentioned in the manifest */
	for (deletion_t &deletion : deletions)
		MyDeleteFile(deletion.deleteMeFilename);

	SendDlgItemMessage(hwndMain, IDC_PROGRESS, PBM_SETPOS, 100, 0);

	Status(L"Update complete.");
	SetDlgItemText(hwndMain, IDC_BUTTON, L"Launch OBS");
	return true;
}

static DWORD WINAPI UpdateThread(void *arg)
{
	wchar_t *cmdLine = (wchar_t *)arg;

	bool success = Update(cmdLine);

	if (!success) {
		/* This handles deleting temp files and rolling back and
		 * partially installed updates */
		CleanupPartialUpdates();

		if (tempPath[0])
			RemoveDirectory(tempPath);

		if (WaitForSingleObject(cancelRequested, 0) == WAIT_OBJECT_0)
			Status(L"Update aborted.");

		HWND hProgress = GetDlgItem(hwndMain, IDC_PROGRESS);
		LONG_PTR style = GetWindowLongPtr(hProgress, GWL_STYLE);
		SetWindowLongPtr(hProgress, GWL_STYLE, style & ~PBS_MARQUEE);
		SendMessage(hProgress, PBM_SETSTATE, PBST_ERROR, 0);

		SetDlgItemText(hwndMain, IDC_BUTTON, L"Exit");
		EnableWindow(GetDlgItem(hwndMain, IDC_BUTTON), true);

		updateFailed = true;
	} else {
		if (tempPath[0])
			RemoveDirectory(tempPath);
	}

	if (bExiting)
		ExitProcess(success);
	return 0;
}

static void CancelUpdate(bool quit)
{
	if (WaitForSingleObject(updateThread, 0) != WAIT_OBJECT_0) {
		bExiting = quit;
		SetEvent(cancelRequested);
	} else {
		PostQuitMessage(0);
	}
}

static void LaunchOBS(LPWSTR lpCmdLine)
{
	wchar_t newCwd[MAX_PATH];
	wchar_t obsPath[MAX_PATH];

	StringCbCopy(obsPath, sizeof(obsPath), obs_base_directory);
	StringCbCat(obsPath, sizeof(obsPath), L"\\bin\\64bit");
	SetCurrentDirectory(obsPath);
	StringCbCopy(newCwd, sizeof(newCwd), obsPath);

	StringCbCat(obsPath, sizeof(obsPath), L"\\obs64.exe");

	if (!FileExists(obsPath)) {
		/* TODO: give user a message maybe? */
		return;
	}

	SHELLEXECUTEINFO execInfo;

	ZeroMemory(&execInfo, sizeof(execInfo));

	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = obsPath;
	execInfo.lpDirectory = newCwd;
	execInfo.nShow = SW_SHOWNORMAL;

	if (lpCmdLine[0])
		execInfo.lpParameters = lpCmdLine;

	ShellExecuteEx(&execInfo);
}

static INT_PTR CALLBACK UpdateDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG: {
		static HICON hMainIcon = LoadIcon(hinstMain, MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hMainIcon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hMainIcon);
		return true;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON) {
			if (HIWORD(wParam) == BN_CLICKED) {
				DWORD result = WaitForSingleObject(updateThread, 0);
				if (result == WAIT_OBJECT_0) {
					if (updateFailed)
						PostQuitMessage(0);
					else
						PostQuitMessage(1);
				} else {
					EnableWindow((HWND)lParam, false);
					CancelUpdate(false);
				}
			}
		}
		return true;

	case WM_CLOSE:
		CancelUpdate(true);
		return true;
	}

	return false;
}

static int RestartAsAdmin(LPCWSTR lpCmdLine, LPCWSTR cwd)
{
	wchar_t myPath[MAX_PATH];
	if (!GetModuleFileNameW(nullptr, myPath, _countof(myPath) - 1)) {
		return 0;
	}

	/* If the admin is a different user, add the path to the user's
	 * AppData to the command line so we can load the correct manifest. */
	wstring elevatedCmdLine(lpCmdLine);
	CoTaskMemPtr<wchar_t> pOut;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pOut);
	if (hr == S_OK) {
		elevatedCmdLine += L" \"--appdata=";
		elevatedCmdLine += pOut;
		elevatedCmdLine += L"\"";
	}

	SHELLEXECUTEINFO shExInfo = {0};
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = nullptr;
	shExInfo.lpVerb = L"runas";                      /* Operation to perform */
	shExInfo.lpFile = myPath;                        /* Application to start */
	shExInfo.lpParameters = elevatedCmdLine.c_str(); /* Additional parameters */
	shExInfo.lpDirectory = cwd;
	shExInfo.nShow = SW_NORMAL;
	shExInfo.hInstApp = nullptr;

	/* annoyingly the actual elevated updater will disappear behind other
	 * windows :( */
	AllowSetForegroundWindow(ASFW_ANY);

	if (ShellExecuteEx(&shExInfo)) {
		DWORD exitCode;

		WaitForSingleObject(shExInfo.hProcess, INFINITE);

		if (GetExitCodeProcess(shExInfo.hProcess, &exitCode)) {
			if (exitCode == 1) {
				return exitCode;
			}
		}
		CloseHandle(shExInfo.hProcess);
	}
	return 0;
}

static bool HasElevation()
{
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID sid = nullptr;
	BOOL elevated = false;
	BOOL success;

	success = AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
					   0, &sid);
	if (success && sid) {
		CheckTokenMembership(nullptr, sid, &elevated);
		FreeSid(sid);
	}

	return elevated;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int)
{
	INITCOMMONCONTROLSEX icce;

	wchar_t cwd[MAX_PATH];
	GetCurrentDirectoryW(_countof(cwd) - 1, cwd);

	if (!IsWindows10OrGreater()) {
		MessageBox(nullptr,
			   L"OBS Studio 28 and newer no longer support Windows 7,"
			   L" Windows 8, or Windows 8.1. You can disable the"
			   L" following setting to opt out of future updates:"
			   L" Settings → General → General → Automatically check"
			   L" for updates on startup",
			   L"Unsupported Operating System", MB_ICONWARNING);
		return 0;
	}

	if (!HasElevation()) {

		WinHandle hMutex = OpenMutex(SYNCHRONIZE, false, L"OBSUpdaterRunningAsNonAdminUser");
		if (hMutex) {
			MessageBox(nullptr, L"OBS Studio Updater must be run as an administrator.", L"Updater Error",
				   MB_ICONWARNING);
			return 2;
		}

		HANDLE hLowMutex = CreateMutexW(nullptr, true, L"OBSUpdaterRunningAsNonAdminUser");

		/* return code 1 =  user wanted to launch OBS */
		if (RestartAsAdmin(lpCmdLine, cwd) == 1) {
			StringCbCat(cwd, sizeof(cwd), L"\\..\\..");
			GetFullPathName(cwd, _countof(obs_base_directory), obs_base_directory, nullptr);
			SetCurrentDirectory(obs_base_directory);

			LaunchOBS(lpCmdLine);
		}

		if (hLowMutex) {
			ReleaseMutex(hLowMutex);
			CloseHandle(hLowMutex);
		}

		return 0;
	} else {
		StringCbCat(cwd, sizeof(cwd), L"\\..\\..");
		GetFullPathName(cwd, _countof(obs_base_directory), obs_base_directory, nullptr);
		SetCurrentDirectory(obs_base_directory);

		hinstMain = hInstance;

		icce.dwSize = sizeof(icce);
		icce.dwICC = ICC_PROGRESS_CLASS;

		InitCommonControlsEx(&icce);

		hwndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_UPDATEDIALOG), nullptr, UpdateDialogProc);
		if (!hwndMain) {
			return -1;
		}

		ShowWindow(hwndMain, SW_SHOWNORMAL);
		SetForegroundWindow(hwndMain);

		cancelRequested = CreateEvent(nullptr, true, false, nullptr);
		updateThread = CreateThread(nullptr, 0, UpdateThread, lpCmdLine, 0, nullptr);

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			if (!IsDialogMessage(hwndMain, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		/* there is no non-elevated process waiting for us if UAC is
		 * disabled */
		WinHandle hMutex = OpenMutex(SYNCHRONIZE, false, L"OBSUpdaterRunningAsNonAdminUser");
		if (msg.wParam == 1 && !hMutex) {
			LaunchOBS(lpCmdLine);
		}

		return (int)msg.wParam;
	}
}
