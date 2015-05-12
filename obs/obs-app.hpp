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

#include <QApplication>
#include <QTranslator>
#include <QPointer>
#include <util/lexer.h>
#include <util/util.hpp>
#include <string>
#include <memory>
#include <vector>

#include "window-main.hpp"

std::string CurrentTimeString();
std::string CurrentDateTimeString();
std::string GenerateTimeDateFilename(const char *extension);
QObject *CreateShortcutFilter();

struct BaseLexer {
	lexer lex;
public:
	inline BaseLexer() {lexer_init(&lex);}
	inline ~BaseLexer() {lexer_free(&lex);}
	operator lexer*() {return &lex;}
};

class OBSTranslator : public QTranslator {
	Q_OBJECT

public:
	virtual bool isEmpty() const override {return false;}

	virtual QString translate(const char *context, const char *sourceText,
			const char *disambiguation, int n) const override;
};

class OBSApp : public QApplication {
	Q_OBJECT

private:
	std::string                    locale;
	std::string		       theme;
	ConfigFile                     globalConfig;
	TextLookup                     textLookup;
	QPointer<OBSMainWindow>        mainWindow;

	bool InitGlobalConfig();
	bool InitGlobalConfigDefaults();
	bool InitLocale();
	bool InitTheme();

public:
	OBSApp(int &argc, char **argv);

	void AppInit();
	bool OBSInit();

	inline QMainWindow *GetMainWindow() const {return mainWindow.data();}

	inline config_t *GlobalConfig() const {return globalConfig;}

	inline const char *GetLocale() const
	{
		return locale.c_str();
	}

	inline const char *GetTheme() const {return theme.c_str();}
	bool SetTheme(std::string name, std::string path = "");

	inline lookup_t *GetTextLookup() const {return textLookup;}

	inline const char *GetString(const char *lookupVal) const
	{
		return textLookup.GetString(lookupVal);
	}

	const char *GetLastLog() const;
	const char *GetCurrentLog() const;

	std::string GetVersionString() const;

	const char *InputAudioSource() const;
	const char *OutputAudioSource() const;

	const char *GetRenderModule() const;
};

inline OBSApp *App() {return static_cast<OBSApp*>(qApp);}

inline config_t *GetGlobalConfig() {return App()->GlobalConfig();}

std::vector<std::pair<std::string, std::string>> GetLocaleNames();
inline const char *Str(const char *lookup) {return App()->GetString(lookup);}
#define QTStr(lookupVal) QString::fromUtf8(Str(lookupVal))
