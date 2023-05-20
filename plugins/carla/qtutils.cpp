/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qtutils.h"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMainWindow>

void carla_qt_callback_on_main_thread(void (*callback)(void *param),
				      void *param)
{
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

const char *carla_qt_file_dialog(bool save, bool isDir, const char *title,
				 const char *filter)
{
	static QByteArray ret;

	QWidget *parent = nullptr;
	for (QWidget *w : QApplication::topLevelWidgets()) {
		if (QMainWindow *mw = qobject_cast<QMainWindow *>(w)) {
			parent = mw;
			break;
		}
	}

	QFileDialog::Options options;

	if (isDir)
		options |= QFileDialog::ShowDirsOnly;

	ret = save ? QFileDialog::getSaveFileName(parent, title, {}, filter,
						  nullptr, options)
			      .toUtf8()
		   : QFileDialog::getOpenFileName(parent, title, {}, filter,
						  nullptr, options)
			      .toUtf8();

	return ret.constData();
}
