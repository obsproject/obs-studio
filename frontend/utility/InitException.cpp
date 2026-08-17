/******************************************************************************
    Copyright (C) 2026 by SpookyJumpyBeans <116844292+SpookyJumpyBeans@users.noreply.github.com>

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

#include "InitException.hpp"

#include <OBSApp.hpp>

namespace {

const char *GetEnglishMessage(OBS::InitErrorCode code)
{
	switch (code) {
	case OBS::InitErrorCode::UserDirectories:
		return "Failed to create required user directories";
	case OBS::InitErrorCode::GlobalConfig:
		return "Failed to initialize global config";
	case OBS::InitErrorCode::Locale:
		return "Failed to load locale";
	case OBS::InitErrorCode::Theme:
		return "Failed to load theme";
	case OBS::InitErrorCode::ProfileDirectories:
		return "Failed to create profile directories";
	case OBS::InitErrorCode::LocaleIniPath:
		return "Could not find locale.ini path";
	case OBS::InitErrorCode::LocaleIniOpen:
		return "Could not open locale.ini";
	case OBS::InitErrorCode::BasicConfig:
		return "Failed to load basic.ini";
	case OBS::InitErrorCode::Audio:
		return "Failed to initialize audio";
	case OBS::InitErrorCode::VideoModuleNotFound:
		return "Failed to initialize video:  Graphics module not found";
	case OBS::InitErrorCode::VideoNotSupported:
		return "Failed to initialize video:\n\nRequired graphics API functionality "
		       "not found.  Your GPU may not be supported.";
	case OBS::InitErrorCode::VideoInvalidParam:
		return "Failed to initialize video:  Invalid parameters";
	case OBS::InitErrorCode::VideoUnknown:
		return "Failed to initialize video.  Your GPU may not be supported, "
		       "or your graphics drivers may need to be updated.";
	case OBS::InitErrorCode::Service:
		return "Failed to initialize service";
	case OBS::InitErrorCode::SafeModulesUndefined:
		return "SAFE_MODULES not defined";
	case OBS::InitErrorCode::SafeModulesEmpty:
		return "SAFE_MODULES is empty";
	case OBS::InitErrorCode::Invalid:
		return "Unknown initialization error";
	}

	return "Unknown initialization error";
}

} // namespace

namespace OBS {

InitException::InitException(InitErrorCode code, std::string detail)
	: code_(code),
	  detail_(std::move(detail)),
	  whatMessage_(GetEnglishMessage(code_))
{
	if (!detail_.empty()) {
		whatMessage_ += " (";
		whatMessage_ += detail_;
		whatMessage_ += ')';
	}
}

const char *InitException::GetLocaleKey() const noexcept
{
	switch (code_) {
	case InitErrorCode::UserDirectories:
		return "Init.Error.UserDirectories";
	case InitErrorCode::GlobalConfig:
		return "Init.Error.GlobalConfig";
	case InitErrorCode::Locale:
		return "Init.Error.Locale";
	case InitErrorCode::Theme:
		return "Init.Error.Theme";
	case InitErrorCode::ProfileDirectories:
		return "Init.Error.ProfileDirectories";
	case InitErrorCode::LocaleIniPath:
		return "Init.Error.LocaleIniPath";
	case InitErrorCode::LocaleIniOpen:
		return "Init.Error.LocaleIniOpen";
	case InitErrorCode::BasicConfig:
		return "Init.Error.BasicConfig";
	case InitErrorCode::Audio:
		return "Init.Error.Audio";
	case InitErrorCode::VideoModuleNotFound:
		return "Init.Error.VideoModuleNotFound";
	case InitErrorCode::VideoNotSupported:
		return "Init.Error.VideoNotSupported";
	case InitErrorCode::VideoInvalidParam:
		return "Init.Error.VideoInvalidParam";
	case InitErrorCode::VideoUnknown:
		return "Init.Error.VideoUnknown";
	case InitErrorCode::Service:
		return "Init.Error.Service";
	case InitErrorCode::SafeModulesUndefined:
		return "Init.Error.SafeModulesUndefined";
	case InitErrorCode::SafeModulesEmpty:
		return "Init.Error.SafeModulesEmpty";
	case InitErrorCode::Invalid:
		return "Init.Error.Unknown";
	}

	return "Init.Error.Unknown";
}

QString InitException::GetUserMessage() const
{
	if (!qApp) {
		return QString::fromUtf8(whatMessage_.c_str());
	}

	const char *key = GetLocaleKey();
	const char *translated = Str(key);
	if (!translated || translated == key) {
		return QString::fromUtf8(whatMessage_.c_str());
	}

	return QString::fromUtf8(translated);
}

const char *InitException::what() const noexcept
{
	return whatMessage_.c_str();
}

} // namespace OBS
