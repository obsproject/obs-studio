/******************************************************************************
 Copyright (C) 2014 by John R. Bradley <jrb@turrettech.com>
 Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <include/cef_app.h>
#include <include/cef_base.h>
#include <include/cef_task.h>
#include <include/cef_client.h>
#include <include/cef_parser.h>
#include <include/cef_scheme.h>
#include <include/cef_version.h>
#if CHROME_VERSION_BUILD >= 6943
#include <include/cef_version_info.h>
#endif
#include <include/cef_render_process_handler.h>
#include <include/cef_request_context_handler.h>
#include <include/cef_jsdialog_handler.h>
#if defined(__APPLE__)
#include "include/wrapper/cef_library_loader.h"
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && \
	(CHROME_VERSION_BUILD >= 6943 || (CHROME_VERSION_BUILD > 6337 && defined(CEF_OSR_EXTRA_INFO)))
#define ENABLE_BROWSER_SHARED_TEXTURE
#endif

#define SendBrowserProcessMessage(browser, pid, msg)             \
	CefRefPtr<CefFrame> mainFrame = browser->GetMainFrame(); \
	if (mainFrame) {                                         \
		mainFrame->SendProcessMessage(pid, msg);         \
	}

#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
