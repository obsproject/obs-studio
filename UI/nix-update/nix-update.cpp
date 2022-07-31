#include "nix-update.hpp"
#include "crypto-helpers.hpp"
#include "nix-update-helpers.hpp"
#include "obs-app.hpp"
#include "remote-text.hpp"
#include "platform.hpp"

#include <util/util.hpp>
#include <blake2.h>

#include <iostream>
#include <fstream>

#include <QRandomGenerator>
#include <QByteArray>
#include <QString>

#include <browser-panel.hpp>

struct QCef;
extern QCef *cef;

#ifndef MAC_WHATSNEW_URL
#define MAC_WHATSNEW_URL "https://obsproject.com/update_studio/whatsnew.json"
#endif

#ifndef LINUX_WHATSNEW_URL
#define LINUX_WHATSNEW_URL "https://obsproject.com/update_studio/whatsnew.json"
#endif

#ifdef __APPLE__
#define WHATSNEW_URL MAC_WHATSNEW_URL
#else
#define WHATSNEW_URL LINUX_WHATSNEW_URL
#endif

#define HASH_READ_BUF_SIZE 65536
#define BLAKE2_HASH_LENGTH 20

/* ------------------------------------------------------------------------ */

static bool QuickWriteFile(const char *file, std::string &data)
try {
	std::ofstream fileStream(file, std::ios::binary);
	if (fileStream.fail())
		throw strprintf("Failed to open file '%s': %s", file,
				strerror(errno));

	fileStream.write(data.data(), data.size());
	if (fileStream.fail())
		throw strprintf("Failed to write file '%s': %s", file,
				strerror(errno));

	return true;

} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

static bool QuickReadFile(const char *file, std::string &data)
try {
	std::ifstream fileStream(file);
	if (!fileStream.is_open() || fileStream.fail())
		throw strprintf("Failed to open file '%s': %s", file,
				strerror(errno));

	fileStream.seekg(0, fileStream.end);
	size_t size = fileStream.tellg();
	fileStream.seekg(0);

	data.resize(size);
	fileStream.read(&data[0], size);

	if (fileStream.fail())
		throw strprintf("Failed to write file '%s': %s", file,
				strerror(errno));

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

	std::ifstream file(path, std::ios::binary);
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
	const char *pguid =
		config_get_string(GetGlobalConfig(), "General", "InstallGUID");
	std::string guid;
	if (pguid)
		guid = pguid;

	if (guid.empty()) {
		GenerateGUID(guid);

		if (!guid.empty())
			config_set_string(GetGlobalConfig(), "General",
					  "InstallGUID", guid.c_str());
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

static bool CheckDataSignature(const char *name, const std::string &data,
			       const std::string &hexSig)
try {
	if (hexSig.empty() || hexSig.length() > 0xFFFF ||
	    (hexSig.length() & 1) != 0)
		throw strprintf("Missing or invalid signature for %s: %s", name,
				hexSig.c_str());

	static std::string obsPubKey;
	if (obsPubKey.empty())
		LoadPublicKey(obsPubKey);

	// Convert hex string to bytes
	auto signature = QByteArray::fromHex(hexSig.data());

	if (!VerifySignature((uint8_t *)obsPubKey.data(), obsPubKey.size(),
			     (uint8_t *)data.data(), data.size(),
			     (uint8_t *)signature.data(), signature.size()))
		throw strprintf("Signature check failed for %s", name);

	return true;

} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

/* ------------------------------------------------------------------------ */

void WhatsNewInfoThread::run()
try {
	long responseCode;
	std::vector<std::string> extraHeaders;
	std::string text;
	std::string error;
	std::string signature;
	uint8_t whatsnewHash[BLAKE2_HASH_LENGTH];
	bool success;

	BPtr<char> whatsnewPath =
		GetConfigPathPtr("obs-studio/updates/whatsnew.json");

	/* ----------------------------------- *
	 * avoid downloading json again        */

	if (CalculateFileHash(whatsnewPath, whatsnewHash)) {
		auto hash = QByteArray::fromRawData((const char *)whatsnewHash,
						    BLAKE2_HASH_LENGTH);

		QString header = "If-None-Match: " + hash.toHex();
		extraHeaders.push_back(move(header.toStdString()));
	}

	/* ----------------------------------- *
	 * get current install GUID            */

	std::string guid = GetProgramGUID();

	if (!guid.empty()) {
		std::string header = "X-OBS2-GUID: " + guid;
		extraHeaders.push_back(move(header));
	}

	/* ----------------------------------- *
	 * get json from server                */

	success = GetRemoteFile(WHATSNEW_URL, text, error, &responseCode,
				nullptr, "", nullptr, extraHeaders, &signature);

	if (!success || (responseCode != 200 && responseCode != 304)) {
		if (responseCode == 404)
			return;

		throw strprintf("Failed to fetch whatsnew file: %s",
				error.c_str());
	}

	/* ----------------------------------- *
	 * verify file signature               */

	if (responseCode == 200) {
		success = CheckDataSignature("whatsnew", text, signature);
		if (!success)
			throw std::string("Invalid whatsnew signature");
	}

	/* ----------------------------------- *
	 * write or load json                  */

	if (responseCode == 200) {
		if (!QuickWriteFile(whatsnewPath, text))
			throw strprintf("Could not write file '%s'",
					whatsnewPath.Get());
	} else {
		if (!QuickReadFile(whatsnewPath, text))
			throw strprintf("Could not read file '%s'",
					whatsnewPath.Get());
	}

	/* ----------------------------------- *
	 * success                             */

	emit Result(QString::fromStdString(text));

} catch (std::string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
}

/* ------------------------------------------------------------------------ */

void WhatsNewBrowserInitThread::run()
{
	cef->wait_for_browser_init();
	emit Result(url);
}
