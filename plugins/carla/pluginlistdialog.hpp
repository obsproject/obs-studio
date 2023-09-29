/*
 * Carla plugin host, adjusted for OBS
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CarlaDefines.h>

#include "ui_pluginlistdialog.h"

#if CARLA_VERSION_HEX >= 0x020591
#define CARLA_2_6_FEATURES
#endif

class QSafeSettings;
typedef struct _CarlaPluginDiscoveryInfo CarlaPluginDiscoveryInfo;
struct PluginInfo;

// ----------------------------------------------------------------------------
// Plugin List Dialog

class PluginListDialog : public QDialog {
	enum TableIndex {
		TW_FAVORITE,
		TW_NAME,
		TW_LABEL,
		TW_MAKER,
		TW_BINARY,
	};

	enum UserRoles {
		UR_PLUGIN_INFO = 1,
		UR_SEARCH_TEXT,
	};

	struct PrivateData;
	PrivateData *const p;

	Ui_PluginListDialog ui;

	// --------------------------------------------------------------------
	// public methods

public:
	explicit PluginListDialog(QWidget *parent);
	~PluginListDialog() override;

	const PluginInfo &getSelectedPluginInfo() const;
#ifdef CARLA_2_6_FEATURES
	void addPluginInfo(const CarlaPluginDiscoveryInfo *info,
			   const char *sha1sum);
	bool checkPluginCache(const char *filename, const char *sha1sum);
#endif

	// --------------------------------------------------------------------
	// protected methods

protected:
	void done(int) override;
	void showEvent(QShowEvent *) override;
	void timerEvent(QTimerEvent *) override;

	// --------------------------------------------------------------------
	// private methods

private:
	void addPluginsToTable();
	void loadSettings();

	// --------------------------------------------------------------------
	// private slots

private Q_SLOTS:
	void cellClicked(int row, int column);
	void cellDoubleClicked(int row, int column);
	void focusSearchFieldAndSelectAllText();
	void checkFilters();
	void checkFiltersCategoryAll(bool clicked);
	void checkFiltersCategorySpecific(bool clicked);
	void clearFilters();
	void checkPlugin(int row);
	void refreshPlugins();
	void refreshPluginsStart();
	void refreshPluginsStop();
	void refreshPluginsSkip();
	void saveSettings();
};

// ----------------------------------------------------------------------------
