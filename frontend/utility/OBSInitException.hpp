/******************************************************************************
    Copyright (C) 2026 by Ray Winkelman <rwinkelman@users.noreply.github.com>

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

#pragma once

#include <QString>

#include <exception>
#include <string>
#include <utility>

enum class OBSInitErrorCode {
	UserDirs,
	GlobalConfig,
	Locale,
	Theme,
	ProfileDirs,
	LocaleIniPath,
	LocaleIniOpen,
};

class OBSInitException : public std::exception {
public:
	explicit OBSInitException(OBSInitErrorCode code, std::string detail = {})
		: code_(code),
		  detail_(std::move(detail))
	{
	}

	OBSInitErrorCode code() const { return code_; }

	const char *what() const noexcept override
	{
		return detail_.empty() ? codeName(code_) : detail_.c_str();
	}

	QString localizedMessage() const;

	static const char *codeName(OBSInitErrorCode code)
	{
		switch (code) {
		case OBSInitErrorCode::UserDirs:
			return "UserDirs";
		case OBSInitErrorCode::GlobalConfig:
			return "GlobalConfig";
		case OBSInitErrorCode::Locale:
			return "Locale";
		case OBSInitErrorCode::Theme:
			return "Theme";
		case OBSInitErrorCode::ProfileDirs:
			return "ProfileDirs";
		case OBSInitErrorCode::LocaleIniPath:
			return "LocaleIniPath";
		case OBSInitErrorCode::LocaleIniOpen:
			return "LocaleIniOpen";
		}
		return "Unknown";
	}

private:
	OBSInitErrorCode code_;
	std::string detail_;
};
