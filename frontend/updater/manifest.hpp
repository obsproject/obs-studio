/*
 * Copyright (c) 2023 Dennis Sädtler <dennis@obsproject.com>
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

#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace updater {
struct File {
	/* 160-bit blake2b hashes of zstd compressed and raw file (hex string) */
	std::string compressed_hash;
	std::string hash;
	/* Relative path */
	std::string name;
	/* Uncompressed size in bytes */
	size_t size = 0;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(File, compressed_hash, hash, name, size)
};

struct Package {
	std::string name;
	std::vector<File> files = {};
	/* Relative paths of files to remove */
	std::vector<std::string> removed_files = {};

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Package, name, files, removed_files)
};

struct Manifest {
	/* Update packages */
	std::vector<Package> packages = {};

	/* Version information */
	uint8_t version_major = 0;
	uint8_t version_minor = 0;
	uint8_t version_patch = 0;
	uint8_t beta = 0;
	uint8_t rc = 0;
	std::string commit;

	/* Hash of VC redist file */
	std::string vc2019_redist_x64;

	/* Release notes in HTML format */
	std::string notes;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Manifest, packages, version_major, version_minor, version_patch, beta, rc,
				       commit, vc2019_redist_x64, notes)
};

struct PatchRequest {
	/* Relative path of file */
	std::string name;
	/* 160-bit blake2b hash of existing file */
	std::string hash;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PatchRequest, name, hash)
};

using PatchesRequest = std::vector<PatchRequest>;

struct PatchResponse {
	/* Relative path of file */
	std::string name;
	/* 160-bit blake2b hash of patch file */
	std::string hash;
	/* Download URL for patch file */
	std::string source;
	/* Size of patch file */
	size_t size = 0;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PatchResponse, name, hash, size, source)
};

using PatchesResponse = std::vector<PatchResponse>;
} // namespace updater
