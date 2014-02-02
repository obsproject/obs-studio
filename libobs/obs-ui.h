/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct obs_modal_ui {
	const char *name;
	const char *task;
	const char *target;
	bool (*callback)(void *object, void *ui_data);
};

struct obs_modeless_ui {
	const char *name;
	const char *task;
	const char *target;
	void *(*callback)(void *object, void *ui_data);
};

/*
 * ===========================================
 *   Module UI calls
 * ===========================================
 *
 *   Modules can specify custom user-interface-specific exports.  UI exports
 * can be within the same library as the actual core logic, or separated in to
 * different modules to split up UI logic and core module logic.
 *
 *   The reasoning for this is to allow for custom user interface of differing
 * toolkits or for automatically generated user interface, or to simply allow
 * separation of UI code from core code (which may often be in differing
 * languages)
 *
 *   A module with UI calls needs to export one or both of these functions:
 *       + enum_modal_ui
 *       + enum_modeless_ui
 *
 *   The enum_ui function provides an obs_ui_info structure for each
 * input/output/etc.  Modeless UI should be exported enum_modeless_ui.  For
 * example, to export Qt-specific configuration functions, the values given to
 * enum_modal_ui might look something like this:
 *
 *    struct obs_modal_ui ui_list[] = {
 *            {"mysource",  "config", "qt", mysource_config},
 *            {"myoutput",  "config", "qt", myoutput_config},
 *            {"myencoder", "config", "qt", myenoder_config}
 *    };
 *
 *   'qt' could be replaced with 'wx' or something similar if using wxWidgets,
 * or 'win32' if using bare windows API.  It could also specify a custom name
 * if desired (use with discretion).
 *
 * ===========================================
 *   Primary Exports
 * ===========================================
 *   bool enum_modal_ui(size_t idx, struct obs_modal_ui *ui_info);
 *
 *                idx: index of the enumeration
 *            ui_info: pointer to the ui data for this enumeration
 *       Return value: false when no more available.
 *
 * ---------------------------------------------------------
 *   bool enum_modeless_ui(size_t idx, struct obs_modeless_ui *ui_info);
 *
 *                idx: index of the enumeration
 *            ui_info: pointer to the ui data for this enumeration
 *       Return value: false when no more available.
 *
 * ===========================================
 *   Modal UI Callback
 * ===========================================
 *       bool modal_callback(void *object, void *ui_data);
 *
 *   The 'object' variable points to the input/output/encoder/etc.  The
 * 'ui_data' varaible points to the UI parent or UI-specific data to be used
 * with the custom user interface.
 *
 *   What 'ui_data' points to differs depending on the target, and you should
 * use discretion and consistency when using this variable to relay
 * information to the UI function.  For example, it would be ideal to have
 * 'ui_data' point to a parent, QWidget for Qt, or a wxWindow for wxWidgets,
 * etc., though it's up to the discretion of the developer to define that
 * value.  Because of the nature of void pointers, discretion and consistency
 * is advised.
 *
 * ===========================================
 *   Modeless UI Callback
 * ===========================================
 *       void *modeless_callback(void *data, void *ui_data);
 *
 *   Modeless UI calls return immediately, and typically are supposed to return
 * a pointer or handle to the specific UI object that was created.  For
 * example, a Qt object would ideally return a pointer to a QWidget.  Again,
 * discretion and consistency is advised for the return value.
 */

/**
 * ===========================================
 *   obs_exec_ui
 * ===========================================
 * Requests modal UI to be displayed.  Returns when user is complete.
 *
 *           name: Name of the input/output/etc type that UI was requested for
 *           task: Task of the user interface (usually "config")
 *         target: Desired target (i.e. "qt", "wx", "gtk3", "win32", etc)
 *           data: Pointer to the obs input/output/etc
 *        ui_data: UI-specific data, usually a parent pointer/handle (if any)
 *
 *   Return value: OBS_UI_SUCCESS if the UI was successful
 *                 OBS_UI_CANCEL if the UI was cancelled by the user
 *                 OBS_UI_NOTFOUND if the UI callback was not found
 */
#define OBS_UI_SUCCESS   0
#define OBS_UI_CANCEL   -1
#define OBS_UI_NOTFOUND -2

EXPORT int obs_exec_ui(const char *name, const char *task, const char *target,
		void *data, void *ui_data);

/**
 * ===========================================
 *   obs_create_ui
 * ===========================================
 * Requests modeless UI to be created.  Returns immediately.
 *
 *           name: Name of the input/output/etc type that UI was requested for
 *           task: Task of the user interface
 *         target: Desired target (i.e. "qt", "wx", "gtk3", "win32", etc)
 *           data: Pointer to the obs input/output/etc
 *        ui_data: UI-specific data, usually a parent pointer/handle (if any)
 *
 *   Return value: Pointer to the target-specific modeless object, or NULL if
 *                 not found or failed.
 */
EXPORT void *obs_create_ui(const char *name, const char *task,
		const char *target, void *data, void *ui_data);

#ifdef __cplusplus
}
#endif
