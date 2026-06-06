/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#include "OBSBasic.hpp"

#ifdef RESTREAM_ENABLED
#include <dialogs/OBSRestreamActions.hpp>
#endif

#include <qt-wrappers.hpp>

using namespace std;

extern bool cef_js_avail;

#ifdef RESTREAM_ENABLED
void OBSBasic::RestreamActionDialogOk(bool start_now)
{
	auto *restreamAuth = dynamic_cast<RestreamAuth *>(GetAuth());

	autoStartBroadcast = true;
	autoStopBroadcast = true;
	broadcastReady = restreamAuth->IsBroadcastReady();

	emit BroadcastStreamReady(broadcastReady);

	if (broadcastReady && start_now)
		QMetaObject::invokeMethod(this, "StartStreaming");
}
#endif
