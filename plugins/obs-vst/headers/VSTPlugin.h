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

#define VST_MAX_CHANNELS 8
#define BLOCK_SIZE 512

#ifdef WIN32
	#include <Windows.h>
#endif

#include <string>
#include <obs-module.h>
#include "aeffectx.h"
#include <thread>
#include <mutex>
#include <memory>

class grpc_vst_communicatorClient;

enum VstChunkType
{
	Bank,
	Program,
	Parameter
};

class VSTPlugin
{
public:
	VSTPlugin(obs_source_t *sourceContext);
	~VSTPlugin();

	void loadEffectFromPath(std::string path);
	void unloadEffect();
	void openEditor();
	void closeEditor();
	void hideEditor();
	void setChunk(VstChunkType type, std::string & data);
	void setProgram(const int programNumber);
	void getSourceNames();
	void setOpenInterfaceWhenActive(const bool val) { m_openInterfaceWhenActive = val; }

	int getProgram();
	
	bool isEditorOpen();
	bool hasWindowOpen();
	bool verifyProxy(const bool notifyAudioPause = false);
	bool isProxyDisconnected() const { return m_proxyDisconnected; }

	AEffect* loadEffect();
	AEffect* getEffect() const { return m_effect.get(); }

	obs_audio_data* process(struct obs_audio_data* audio);

	std::string getPluginPath();
	std::string getChunk(VstChunkType type);

	std::atomic<bool> m_proxyDisconnected{ false };

private:
	void stopProxy();

	int32_t chooseProxyPort();

	bool m_is_open{ false };
	bool m_windowCreated{ false };
	bool m_openInterfaceWhenActive{ false };

	float** m_inputs{ nullptr };
	float** m_outputs{ nullptr };

	char m_effectName[64];
	char m_vendorString[64];

	std::string m_pluginPath;
	std::string m_sourceName;
	std::string m_filterName;
	
	std::recursive_mutex m_effectStatusMutex;

	std::unique_ptr<AEffect> m_effect;

	obs_source_t* m_sourceContext;

	std::unique_ptr<grpc_vst_communicatorClient> m_remote;

#ifdef WIN32
	PROCESS_INFORMATION m_winServer;
#endif
};

#endif // OBS_STUDIO_VSTPLUGIN_H
