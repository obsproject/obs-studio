/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

//-----------------------------------------------------------------------------

#ifdef __cplusplus
#include <cstdint>
#include <QtCore/QSettings>
#include <QtWidgets/QMainWindow>
extern "C" {
#else
#include <stdint.h>
typedef struct QMainWindow QMainWindow;
#endif

//-----------------------------------------------------------------------------

typedef struct {
	uint build;
	uint type;
	const char *filename;
	const char *label;
	uint64_t uniqueId;
} PluginListDialogResults;

const PluginListDialogResults *carla_exec_plugin_list_dialog();

//-----------------------------------------------------------------------------
// open a qt file dialog

char *carla_qt_file_dialog(bool save, bool isDir, const char *title,
			   const char *filter);

//-----------------------------------------------------------------------------
// call a function on the main thread

void carla_qt_callback_on_main_thread(void (*callback)(void *param),
				      void *param);

//-----------------------------------------------------------------------------
// get the top-level qt main window

QMainWindow *carla_qt_get_main_window(void);

//-----------------------------------------------------------------------------
// show an error dialog (on main thread and without blocking current scope)

void carla_show_error_dialog(const char *text1, const char *text2);

//-----------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"

//-----------------------------------------------------------------------------
// Safer QSettings class, which does not throw if type mismatches

class QSafeSettings : public QSettings {
public:
	inline QSafeSettings() : QSettings("obs-studio", "obs") {}

	bool valueBool(const QString &key, bool defaultValue) const;
	QString valueString(const QString &key,
			    const QString &defaultValue) const;
	QByteArray valueByteArray(const QString &key,
				  QByteArray defaultValue = {}) const;
};

//-----------------------------------------------------------------------------
// Custom QString class with default utf-8 mode and a few extra methods

class QUtf8String : public QString {
public:
	explicit inline QUtf8String() : QString() {}

	explicit inline QUtf8String(const char *const str)
		: QString(fromUtf8(str))
	{
	}

	inline QUtf8String(const QString &s) : QString(s) {}

	inline bool isNotEmpty() const { return !isEmpty(); }

	inline QUtf8String &operator=(const char *const str)
	{
		return (*this = fromUtf8(str));
	}

	inline QUtf8String strip() const { return simplified().remove(' '); }

#if QT_VERSION < 0x60000
	explicit inline QUtf8String(const QChar *const str, const size_t size)
		: QString(str, size)
	{
	}

	inline QUtf8String sliced(const size_t pos) const
	{
		return QUtf8String(data() + pos, size() - pos);
	}
#endif
};

//-----------------------------------------------------------------------------
// Custom QByteArray class with a few extra methods for Qt5 compat

#if QT_VERSION < 0x60000
class QCompatByteArray : public QByteArray {
public:
	explicit inline QCompatByteArray() : QByteArray() {}

	explicit inline QCompatByteArray(const char *const data,
					 const size_t size)
		: QByteArray(data, size)
	{
	}

	inline QCompatByteArray(const QByteArray &b) : QByteArray(b) {}

	inline QCompatByteArray sliced(const size_t pos) const
	{
		return QCompatByteArray(data() + pos, size() - pos);
	}
};
#else
typedef QByteArray QCompatByteArray;
#endif

#endif // __cplusplus
