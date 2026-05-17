/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "NativeEventFilter.hpp"

#include <widgets/OBSBasic.hpp>

#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace OBS {

bool NativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
	if (eventType == "windows_generic_MSG") {
		MSG *msg = static_cast<MSG *>(message);

		OBSBasic *main = OBSBasic::Get();
		if (!main) {
			return false;
		}

		switch (msg->message) {
		case WM_QUERYENDSESSION:
			main->saveAll();
			if (msg->lParam == ENDSESSION_CRITICAL) {
				break;
			}

			if (main->shouldPromptForClose()) {
				if (result) {
					*result = FALSE;
				}
				QTimer::singleShot(1, main, &OBSBasic::close);
				return true;
			}

			return false;
		case WM_ENDSESSION:
			if (msg->wParam == TRUE) {
				// Session is ending, start closing the main window now with no checks or prompts.
				main->closeWindow();
			}

			return true;
		}
	}

	return false;
}
} // namespace OBS
