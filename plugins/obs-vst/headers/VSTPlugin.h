/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

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
*****************************************************************************/

#ifndef OBS_STUDIO_VSTPLUGIN_H
#define OBS_STUDIO_VSTPLUGIN_H

#define BLOCK_SIZE 512

#include <mutex>
#include <atomic>
#include <string>
#include <QDirIterator>
#include <obs-module.h>
#include "aeffectx.h"
#include "vst-plugin-callbacks.hpp"
#include "EditorWidget.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

class EditorWidget;

class VSTPlugin : public QObject {
	Q_OBJECT

	/* Because effect is always changed in UI thread, so lockEffect is only necessary for these situations:
	1. access effect object outside of UI thread;
	2. close/delete effect object any where. */
	std::recursive_mutex lockEffect;
	AEffect *effect = nullptr;
	obs_source_t *sourceContext;
	std::string pluginPath;

	float **inputs = nullptr;
	float **outputs = nullptr;
	float **channelrefs = nullptr;
	size_t numChannels = 0;
	void createChannelBuffers(size_t count);
	void cleanupChannelBuffers();

	EditorWidget *editorWidget = nullptr;
	bool editorOpened = false;

	AEffect *loadEffect();

	std::atomic_bool effectReady = false;

	std::string sourceName;
	std::string filterName;
	char effectName[64];
	// Remove below... or comment out
	char vendorString[64];

	VstTimeInfo mTimeInfo;

#ifdef __APPLE__
	CFBundleRef bundle = NULL;
#elif WIN32
	HINSTANCE dllHandle = nullptr;
#elif __linux__
	void *soHandle = nullptr;
#endif

	void unloadLibrary();

	static intptr_t hostCallback_static(AEffect *effect, int32_t opcode,
					    int32_t index, intptr_t value,
					    void *ptr, float opt);
	VstTimeInfo *GetTimeInfo();
	float GetSampleRate();

public:
	VSTPlugin(obs_source_t *sourceContext);
	~VSTPlugin();
	void loadEffectFromPath(const std::string &path);
	void unloadEffect();
	std::string getEffectPath();
	std::string getChunk();
	void setChunk(const std::string &data);
	void setProgram(const int programNumber);
	int getProgram();
	void getSourceNames();
	obs_audio_data *process(struct obs_audio_data *audio);
	bool openInterfaceWhenActive = false;
	bool vstLoaded();

	bool isEditorOpen();
	void onEditorClosed();

public slots:
	void openEditor();
	void closeEditor();
};

#endif // OBS_STUDIO_VSTPLUGIN_H
