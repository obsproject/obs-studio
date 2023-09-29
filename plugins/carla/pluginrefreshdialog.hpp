/*
 * Carla plugin host, adjusted for OBS
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "ui_pluginrefreshdialog.h"

#include "qtutils.h"

// ----------------------------------------------------------------------------
// Plugin Refresh Dialog

struct PluginRefreshDialog : QDialog, Ui_PluginRefreshDialog {
	explicit PluginRefreshDialog(QWidget *const parent) : QDialog(parent)
	{
		setupUi(this);

		setWindowFlags(windowFlags() &
			       ~Qt::WindowContextHelpButtonHint);
#ifdef __APPLE__
		setWindowModality(Qt::WindowModal);
#endif

		b_skip->setEnabled(false);
		ch_invalid->setEnabled(false);

		// ------------------------------------------------------------
		// Load settings

		{
			const QSafeSettings settings;

			restoreGeometry(settings.valueByteArray(
				"PluginRefreshDialog/Geometry"));

			if (settings.valueBool("PluginRefreshDialog/RefreshAll",
					       false))
				ch_all->setChecked(true);
			else
				ch_updated->setChecked(true);

			ch_invalid->setChecked(settings.valueBool(
				"PluginRefreshDialog/CheckInvalid", false));
		}

		// ------------------------------------------------------------
		// Set-up Icons

		b_start->setProperty("themeID", "playIcon");

		// ------------------------------------------------------------
		// Set-up connections

		QObject::connect(this, &QDialog::finished, this,
				 &PluginRefreshDialog::saveSettings);
	}

	// --------------------------------------------------------------------
	// private slots

private Q_SLOTS:
	void saveSettings()
	{
		QSafeSettings settings;
		settings.setValue("PluginRefreshDialog/Geometry",
				  saveGeometry());
		settings.setValue("PluginRefreshDialog/RefreshAll",
				  ch_all->isChecked());
		settings.setValue("PluginRefreshDialog/CheckInvalid",
				  ch_invalid->isChecked());
	}
};

// ----------------------------------------------------------------------------
