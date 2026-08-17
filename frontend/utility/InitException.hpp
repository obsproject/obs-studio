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

#pragma once

#include <QString>

#include <exception>
#include <string>

namespace OBS {

enum class InitErrorCode {
	Invalid = 0,
	UserDirectories,
	GlobalConfig,
	Locale,
	Theme,
	ProfileDirectories,
	LocaleIniPath,
	LocaleIniOpen,
	BasicConfig,
	Audio,
	VideoModuleNotFound,
	VideoNotSupported,
	VideoInvalidParam,
	VideoUnknown,
	Service,
	SafeModulesUndefined,
	SafeModulesEmpty,
};

class InitException final : public std::exception {
public:
	explicit InitException(InitErrorCode code, std::string detail = {});

	InitErrorCode GetCode() const noexcept { return code_; }
	const std::string &GetDetail() const noexcept { return detail_; }
	const char *GetLocaleKey() const noexcept;
	QString GetUserMessage() const;
	const char *what() const noexcept override;

private:
	InitErrorCode code_;
	std::string detail_;
	std::string whatMessage_;
};

} // namespace OBS
