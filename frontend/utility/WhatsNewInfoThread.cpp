#include "WhatsNewInfoThread.hpp"

#include <OBSApp.hpp>
#include <utility/RemoteTextThread.hpp>
#include <utility/crypto-helpers.hpp>
#include <utility/platform.hpp>
#include <utility/update-helpers.hpp>

#include <QRandomGenerator>
#include <blake2.h>

#include <fstream>

#include "moc_WhatsNewInfoThread.cpp"

#ifndef MAC_WHATSNEW_URL
#define MAC_WHATSNEW_URL "https://obsproject.com/update_studio/whatsnew.json"
#endif

#ifndef WIN_WHATSNEW_URL
#define WIN_WHATSNEW_URL "https://obsproject.com/update_studio/whatsnew.json"
#endif

#ifndef LINUX_WHATSNEW_URL
#define LINUX_WHATSNEW_URL "https://obsproject.com/update_studio/whatsnew.json"
#endif

#ifdef __APPLE__
#define WHATSNEW_URL MAC_WHATSNEW_URL
#elif defined(_WIN32)
#define WHATSNEW_URL WIN_WHATSNEW_URL
#else
#define WHATSNEW_URL LINUX_WHATSNEW_URL
#endif

#define HASH_READ_BUF_SIZE 65536
#define BLAKE2_HASH_LENGTH 20

/* ------------------------------------------------------------------------ */

static bool QuickWriteFile(const char *file, const std::string &data)
try {
	std::ofstream fileStream(std::filesystem::u8path(file), std::ios::binary);
	if (fileStream.fail())
		throw strprintf("Failed to open file '%s': %s", file, strerror(errno));

	fileStream.write(data.data(), data.size());
	if (fileStream.fail())
		throw strprintf("Failed to write file '%s': %s", file, strerror(errno));

	return true;

} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

static bool QuickReadFile(const char *file, std::string &data)
try {
	std::ifstream fileStream(std::filesystem::u8path(file), std::ios::binary);
	if (!fileStream.is_open() || fileStream.fail())
		throw strprintf("Failed to open file '%s': %s", file, strerror(errno));

	fileStream.seekg(0, fileStream.end);
	size_t size = fileStream.tellg();
	fileStream.seekg(0);

	data.resize(size);
	fileStream.read(&data[0], size);

	if (fileStream.fail())
		throw strprintf("Failed to write file '%s': %s", file, strerror(errno));

	return true;

} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

static bool CalculateFileHash(const char *path, uint8_t *hash)
try {
	blake2b_state blake2;
	if (blake2b_init(&blake2, BLAKE2_HASH_LENGTH) != 0)
		return false;

	std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
	if (!file.is_open() || file.fail())
		return false;

	char buf[HASH_READ_BUF_SIZE];

	for (;;) {
		file.read(buf, HASH_READ_BUF_SIZE);
		size_t read = file.gcount();
		if (blake2b_update(&blake2, &buf, read) != 0)
			return false;
		if (file.eof())
			break;
	}

	if (blake2b_final(&blake2, hash, BLAKE2_HASH_LENGTH) != 0)
		return false;

	return true;

} catch (std::string &text) {
	blog(LOG_DEBUG, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

/* ------------------------------------------------------------------------ */

void GenerateGUID(std::string &guid)
{
	const char alphabet[] = "0123456789abcdef";
	QRandomGenerator *rng = QRandomGenerator::system();

	guid.resize(40);

	for (size_t i = 0; i < 40; i++) {
		guid[i] = alphabet[rng->bounded(0, 16)];
	}
}

std::string GetProgramGUID()
{
	static std::mutex m;
	std::lock_guard<std::mutex> lock(m);

	/* NOTE: this is an arbitrary random number that we use to count the
	 * number of unique OBS installations and is not associated with any
	 * kind of identifiable information */
	const char *pguid = config_get_string(App()->GetAppConfig(), "General", "InstallGUID");
	std::string guid;
	if (pguid)
		guid = pguid;

	if (guid.empty()) {
		GenerateGUID(guid);

		if (!guid.empty())
			config_set_string(App()->GetAppConfig(), "General", "InstallGUID", guid.c_str());
	}

	return guid;
}

/* ------------------------------------------------------------------------ */

static void LoadPublicKey(std::string &pubkey)
{
	std::string pemFilePath;

	if (!GetDataFilePath("OBSPublicRSAKey.pem", pemFilePath))
		throw std::string("Could not find OBS public key file!");
	if (!QuickReadFile(pemFilePath.c_str(), pubkey))
		throw std::string("Could not read OBS public key file!");
}

static bool CheckDataSignature(const char *name, const std::string &data, const std::string &hexSig)
try {
	static std::mutex pubkey_mutex;
	static std::string obsPubKey;

	if (hexSig.empty() || hexSig.length() > 0xFFFF || (hexSig.length() & 1) != 0)
		throw strprintf("Missing or invalid signature for %s: %s", name, hexSig.c_str());

	std::scoped_lock lock(pubkey_mutex);
	if (obsPubKey.empty())
		LoadPublicKey(obsPubKey);

	// Convert hex string to bytes
	auto signature = QByteArray::fromHex(hexSig.data());

	if (!VerifySignature((uint8_t *)obsPubKey.data(), obsPubKey.size(), (uint8_t *)data.data(), data.size(),
			     (uint8_t *)signature.data(), signature.size()))
		throw strprintf("Signature check failed for %s", name);

	return true;

} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

/* ------------------------------------------------------------------------ */

bool FetchAndVerifyFile(const char *name, const char *file, const char *url, std::string *out,
			const std::vector<std::string> &extraHeaders)
{
	long responseCode;
	std::vector<std::string> headers;
	std::string error;
	std::string signature;
	std::string data;
	uint8_t fileHash[BLAKE2_HASH_LENGTH];
	bool success;

	BPtr<char> filePath = GetAppConfigPathPtr(file);

	if (!extraHeaders.empty()) {
		headers.insert(headers.end(), extraHeaders.begin(), extraHeaders.end());
	}

	/* ----------------------------------- *
	 * avoid downloading file again        */

	if (CalculateFileHash(filePath, fileHash)) {
		auto hash = QByteArray::fromRawData((const char *)fileHash, BLAKE2_HASH_LENGTH);

		QString header = "If-None-Match: " + hash.toHex();
		headers.push_back(header.toStdString());
	}

	/* ----------------------------------- *
	 * get current install GUID            */

	std::string guid = GetProgramGUID();

	if (!guid.empty()) {
		std::string header = "X-OBS2-GUID: " + guid;
		headers.push_back(std::move(header));
	}

	/* ----------------------------------- *
	 * get file from server                */

	success = GetRemoteFile(url, data, error, &responseCode, nullptr, "", nullptr, headers, &signature);

	if (!success || (responseCode != 200 && responseCode != 304)) {
		if (responseCode == 404)
			return false;

		throw strprintf("Failed to fetch %s file: %s", name, error.c_str());
	}

	/* ----------------------------------- *
	 * verify file signature               */

	if (responseCode == 200) {
		success = CheckDataSignature(name, data, signature);
		if (!success)
			throw strprintf("Invalid %s signature", name);
	}

	/* ----------------------------------- *
	 * write or load file                  */

	if (responseCode == 200) {
		if (!QuickWriteFile(filePath, data))
			throw strprintf("Could not write file '%s'", filePath.Get());
	} else if (out) { /* Only read file if caller wants data */
		if (!QuickReadFile(filePath, data))
			throw strprintf("Could not read file '%s'", filePath.Get());
	}

	if (out)
		*out = data;

	/* ----------------------------------- *
	 * success                             */
	return true;
}

void WhatsNewInfoThread::run()
try {
	std::string text;

	if (FetchAndVerifyFile("whatsnew", "obs-studio/updates/whatsnew.json", WHATSNEW_URL, &text)) {
		emit Result(QString::fromStdString(text));
	}
} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
}
