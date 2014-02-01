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

struct obs_ui_info {
	const char *name;
	const char *task;
	const char *target;
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
 *   A module with UI calls needs to export this function:
 *       + enum_ui
 *
 *   The enum_ui function provides an obs_ui_info structure for each
 * input/output/etc.  For example, to export Qt-specific configuration
 * functions, the exports might be something like:
 *       + mysource_config_qt
 *       + myoutput_config_qt
 *       + myencoder_config_panel_qt
 *
 *    ..And the values given to enum_ui would be something this:
 *
 *    struct obs_ui_info ui_list[] = {
 *            {"mysource",  "config",       "qt"},
 *            {"myoutput",  "config",       "qt"},
 *            {"myencoder", "config_panel", "qt"}
 *    };
 *
 *   'qt' could be replaced with 'wx' or something similar if using wxWidgets,
 * or 'win32' if using bare windows API.
 *
 * ===========================================
 *   Primary Exports
 * ===========================================
 *   bool enum_ui(size_t idx, const struct obs_ui_info **ui_info);
 *
 *                idx: index of the enumeration
 *            ui_info: pointer to the ui data for this enumeration
 *       Return value: false when no more available.
 *
 * ===========================================
 *   Export Format
 * ===========================================
 *   Although the 'export' variable specifies the full export name, each
 * export should be formatted as so:
 *
 *       bool [name]_[task]_[target](void *data, void *ui_data);
 *
 *     [name]: specifies the name of the input/output/encoder/etc.
 *     [task]: specifies the task of the user interface, most often 'config',
 *             or 'config_panel'
 *   [target]: specifies the target or toolkit of the user interface, such as
 *             but not limited to 'qt', 'wx', 'win32', 'cocoa', etc.  If
 *             a custom solution is desired, it can use a program-specific
 *             name, such as 'myprogram'.
 *
 *   The 'data' variable points to the input/output/encoder/etc.  The 'ui_data'
 * varaible points to the UI parent or UI-specific data to be used with the
 * custom user interface.
 *
 *   What 'ui_data' points to differs depending on the target, and you should
 * use discretion and consistency when using this variable to relay
 * information to the UI function.  For example, it would be ideal to have
 * 'ui_data' point to a parent, QWidget for Qt, or a wxWindow for wxWidgets,
 * etc.
 *
 *   For example, if I had a source called 'mysource', and I wanted to export
 * a function that handles the task 'config' with the Qt library, the export
 * would be:
 *
 *   bool mysource_config_qt(void *data, void *ui_data);
 *
 *   In this example, the ui_data variable should ideally be a pointer to the
 * QWidget parent, if any.
 */

/**
 * ===========================================
 *   obs_call_ui
 * ===========================================
 * Requests UI to be displayed
 *
 *   This is typically used for things like creating dialogs/panels/etc for
 * specific toolkits.
 *
 *           name: Name of the input/output/etc type that UI was requested for
 *           task: Task of the user interface (i.e. "config", "config_panel")
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

EXPORT int obs_call_ui(const char *name, const char *task, const char *target,
		void *data, void *ui_data);

#ifdef __cplusplus
}
#endif
