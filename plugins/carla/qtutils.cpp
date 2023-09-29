/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <obs.h>

#include "qtutils.h"

//-----------------------------------------------------------------------------
// open a qt file dialog

char *carla_qt_file_dialog(bool save, bool isDir, const char *title,
			   const char *filter)
{
	static QByteArray ret;

	QWidget *parent = carla_qt_get_main_window();
	QFileDialog::Options options;

	if (isDir)
		options |= QFileDialog::ShowDirsOnly;

	ret = save ? QFileDialog::getSaveFileName(parent, title, {}, filter,
						  nullptr, options)
			      .toUtf8()
		   : QFileDialog::getOpenFileName(parent, title, {}, filter,
						  nullptr, options)
			      .toUtf8();

	return ret.data();
}

//-----------------------------------------------------------------------------
// call a function on the main thread

void carla_qt_callback_on_main_thread(void (*callback)(void *param),
				      void *param)
{
	if (QThread::currentThread() == qApp->thread()) {
		callback(param);
		return;
	}

	QTimer *const maintimer = new QTimer;
	maintimer->moveToThread(qApp->thread());
	maintimer->setSingleShot(true);
	QObject::connect(maintimer, &QTimer::timeout,
			 [maintimer, callback, param]() {
				 callback(param);
				 maintimer->deleteLater();
			 });
	QMetaObject::invokeMethod(maintimer, "start", Qt::QueuedConnection,
				  Q_ARG(int, 0));
}

//-----------------------------------------------------------------------------
// get the top-level qt main window

QMainWindow *carla_qt_get_main_window(void)
{
	for (QWidget *w : QApplication::topLevelWidgets()) {
		if (QMainWindow *mw = qobject_cast<QMainWindow *>(w))
			return mw;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// show an error dialog (on main thread and without blocking current scope)

static void carla_show_error_dialog_later(void *const param)
{
	char **const texts = static_cast<char **>(param);
	carla_show_error_dialog(texts[0], texts[1]);
	bfree(texts[0]);
	bfree(texts[1]);
	bfree(texts);
}

void carla_show_error_dialog(const char *const text1, const char *const text2)
{
	// there is no point showing incomplete error messages
	if (text1 == nullptr || text2 == nullptr)
		return;

	// we cannot do Qt gui stuff outside the main thread
	// do a little dance so we call ourselves later on the main thread
	if (QThread::currentThread() != qApp->thread()) {
		char **const texts =
			static_cast<char **>(bmalloc(sizeof(char *) * 2));
		texts[0] = bstrdup(text1);
		texts[1] = bstrdup(text2);
		carla_qt_callback_on_main_thread(carla_show_error_dialog_later,
						 texts);
		return;
	}

	QMessageBox *const box = new QMessageBox(carla_qt_get_main_window());
	box->setWindowTitle("Error");
	box->setText(QString("%1: %2").arg(text1).arg(text2));
	QObject::connect(box, &QDialog::finished, box, &QWidget::deleteLater);
	QMetaObject::invokeMethod(box, "show", Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------

#if QT_VERSION >= 0x60000
static const auto q_meta_bool = QMetaType(QMetaType::Bool);
static const auto q_meta_bytearray = QMetaType(QMetaType::QByteArray);
static const auto q_meta_string = QMetaType(QMetaType::QString);
#else
constexpr auto q_meta_bool = QVariant::Bool;
constexpr auto q_meta_bytearray = QVariant::ByteArray;
constexpr auto q_meta_string = QVariant::String;
#endif

bool QSafeSettings::valueBool(const QString &key, const bool defaultValue) const
{
	QVariant var(value(key, defaultValue));

	if (!var.isNull() && var.convert(q_meta_bool) && var.isValid())
		return var.toBool();

	return defaultValue;
}

QString QSafeSettings::valueString(const QString &key,
				   const QString &defaultValue) const
{
	QVariant var(value(key, defaultValue));

	if (!var.isNull() && var.convert(q_meta_string) && var.isValid())
		return var.toString();

	return defaultValue;
}

QByteArray QSafeSettings::valueByteArray(const QString &key,
					 const QByteArray defaultValue) const
{
	QVariant var(value(key, defaultValue));

	if (!var.isNull() && var.convert(q_meta_bytearray) && var.isValid())
		return var.toByteArray();

	return defaultValue;
}

//-----------------------------------------------------------------------------
