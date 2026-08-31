/******************************************************************************
    Copyright (C) 2026 by nicolaeser <nico@laeser.software>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "RuntimeStatus.hpp"

#include <util/base.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <unistd.h>
#endif

namespace {

constexpr const char *kPidFileName = "obs.pid";
constexpr const char *kStreamingFileName = "streaming";
constexpr const char *kRecordingFileName = "recording";

int currentProcessId()
{
#ifdef _WIN32
	return static_cast<int>(GetCurrentProcessId());
#else
	return static_cast<int>(getpid());
#endif
}

bool processIsAlive(int pid)
{
	if (pid <= 0) {
		return false;
	}

#ifdef _WIN32
	HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
	if (!handle) {
		return false;
	}

	DWORD exitCode = 0;
	const bool isAlive = GetExitCodeProcess(handle, &exitCode) && exitCode == STILL_ACTIVE;
	CloseHandle(handle);
	return isAlive;
#else
	if (kill(static_cast<pid_t>(pid), 0) == 0) {
		return true;
	}

	return errno != ESRCH;
#endif
}

int readPidFile(const std::filesystem::path &path)
{
	std::ifstream file(path);
	if (!file) {
		return 0;
	}

	int pid = 0;
	file >> pid;
	if (!file) {
		return 0;
	}

	return pid;
}

bool writeFile(const std::filesystem::path &path, const std::string &contents)
{
	std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file) {
		return false;
	}

	file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
	return static_cast<bool>(file);
}

std::vector<std::filesystem::path> statusDirectoryCandidates()
{
	std::vector<std::filesystem::path> candidates;

	const char *overrideDirectory = std::getenv("OBS_STATUS_DIR");
	if (overrideDirectory && *overrideDirectory) {
		candidates.emplace_back(std::filesystem::u8path(overrideDirectory));
		return candidates;
	}

#ifndef _WIN32
	const char *xdgRuntimeDirectory = std::getenv("XDG_RUNTIME_DIR");
	if (xdgRuntimeDirectory && *xdgRuntimeDirectory) {
		candidates.emplace_back(std::filesystem::u8path(xdgRuntimeDirectory) / "obs-studio");
	}
#endif

	std::error_code error;
	const std::filesystem::path tempDirectory = std::filesystem::temp_directory_path(error);
	if (!error) {
#ifdef _WIN32
		candidates.emplace_back(tempDirectory / "obs-studio");
#else
		candidates.emplace_back(tempDirectory / ("obs-studio-" + std::to_string(getuid())));
#endif
	}

	return candidates;
}

} // namespace

namespace OBS {

RuntimeStatus::RuntimeStatus() : processId_{currentProcessId()}, pidText_{std::to_string(processId_) + '\n'}
{
	initialize();
}

RuntimeStatus::~RuntimeStatus()
{
	shutdown();
}

void RuntimeStatus::initialize()
{
	const std::vector<std::filesystem::path> candidates = statusDirectoryCandidates();

	for (const std::filesystem::path &directory : candidates) {
		if (tryInitializeDirectory(directory)) {
			blog(LOG_INFO, "Status directory: %s", statusDirectory_.u8string().c_str());
			return;
		}
	}

	blog(LOG_WARNING, "Failed to create status directory");
}

bool RuntimeStatus::tryInitializeDirectory(const std::filesystem::path &directory)
{
	if (directory.empty()) {
		return false;
	}

	std::error_code error;
	std::filesystem::create_directories(directory, error);
	if (error) {
		blog(LOG_DEBUG, "Failed to create status directory '%s': %s", directory.u8string().c_str(),
		     error.message().c_str());
		return false;
	}

	statusDirectory_ = directory;

	const int existingPid = readPidFile(markerPath(kPidFileName));
	if (existingPid > 0 && existingPid != processId_ && processIsAlive(existingPid)) {
		blog(LOG_WARNING, "Status directory already in use by PID %d", existingPid);
		statusDirectory_.clear();
		return false;
	}

	removeStaleFiles();
	writeMarker(kPidFileName);

	if (!std::filesystem::exists(markerPath(kPidFileName), error) || error) {
		blog(LOG_DEBUG, "Failed to write PID file in '%s'", directory.u8string().c_str());
		statusDirectory_.clear();
		return false;
	}

	isActive_ = true;
	return true;
}

void RuntimeStatus::removeStaleFiles()
{
	removeMarker(kStreamingFileName);
	removeMarker(kRecordingFileName);
}

void RuntimeStatus::writeMarker(const char *name)
{
	if (statusDirectory_.empty() || !name || !*name) {
		return;
	}

	const std::filesystem::path path = markerPath(name);
	if (!writeFile(path, pidText_)) {
		blog(LOG_WARNING, "Failed to write status file '%s'", path.u8string().c_str());
	}
}

void RuntimeStatus::removeMarker(const char *name)
{
	if (statusDirectory_.empty() || !name || !*name) {
		return;
	}

	std::error_code error;
	std::filesystem::remove(markerPath(name), error);
}

void RuntimeStatus::setMarker(const char *name, bool isActive)
{
	if (isActive) {
		writeMarker(name);
	} else {
		removeMarker(name);
	}
}

std::filesystem::path RuntimeStatus::markerPath(const char *name) const
{
	return statusDirectory_ / name;
}

void RuntimeStatus::handleFrontendEvent(enum obs_frontend_event event)
{
	if (!isActive_) {
		return;
	}

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		setMarker(kStreamingFileName, true);
		break;

	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		setMarker(kStreamingFileName, false);
		break;

	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		setMarker(kRecordingFileName, true);
		break;

	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		setMarker(kRecordingFileName, false);
		break;

	case OBS_FRONTEND_EVENT_EXIT:
		shutdown();
		break;

	default:
		break;
	}
}

void RuntimeStatus::shutdown() noexcept
{
	if (!isActive_) {
		return;
	}

	removeMarker(kStreamingFileName);
	removeMarker(kRecordingFileName);
	removeMarker(kPidFileName);

	isActive_ = false;
}

} // namespace OBS
