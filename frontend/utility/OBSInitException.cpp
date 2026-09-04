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

#include "OBSInitException.hpp"

#include "OBSApp.hpp"

QString OBSInitException::localizedMessage() const
{
	switch (code_) {
	case OBSInitErrorCode::UserDirs:
		return QTStr("Init.Error.UserDirs");
	case OBSInitErrorCode::GlobalConfig:
		return QTStr("Init.Error.GlobalConfig");
	case OBSInitErrorCode::Locale:
		return QTStr("Init.Error.Locale");
	case OBSInitErrorCode::Theme:
		return QTStr("Init.Error.Theme");
	case OBSInitErrorCode::ProfileDirs:
		return QTStr("Init.Error.ProfileDirs");
	case OBSInitErrorCode::LocaleIniPath:
		return QTStr("Init.Error.LocaleIniPath");
	case OBSInitErrorCode::LocaleIniOpen:
		return QTStr("Init.Error.LocaleIniOpen");
	}

	return QTStr("Init.Error.Unknown");
}
