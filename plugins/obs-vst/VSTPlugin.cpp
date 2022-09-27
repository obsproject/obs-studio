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

#include <vector>

#include "headers/VSTPlugin.h"
#include "win/VstWinDefs.h"
#include "headers/grpc_vst_communicatorClient.h"

#define CBASE64_IMPLEMENTATION
#include "cbase64.h"
#ifdef WIN32
#include <cstringt.h>
#endif
#include <functional>
#include <filesystem>

VSTPlugin::VSTPlugin(obs_source_t *sourceContext) :
	m_sourceContext{sourceContext},
	m_effect{nullptr},
	m_is_open{false}
{
	memset(m_effectName, 0, sizeof(m_effectName));
	memset(m_vendorString, 0, sizeof(m_vendorString));

	int numChannels = VST_MAX_CHANNELS;
	int blocksize   = BLOCK_SIZE;

	m_inputs = (float **)malloc(sizeof(float **) * numChannels);
	m_outputs = (float **)malloc(sizeof(float **) * numChannels);

	for (int channel = 0; channel < numChannels; channel++)
	{
		m_inputs[channel] = (float *)malloc(sizeof(float *) * blocksize);
		m_outputs[channel] = (float *)malloc(sizeof(float *) * blocksize);
	}
}

VSTPlugin::~VSTPlugin()
{
	int numChannels = VST_MAX_CHANNELS;

	for (int channel = 0; channel < numChannels; channel++)
	{
		if (m_inputs[channel])
		{
			free(m_inputs[channel]);
			m_inputs[channel] = NULL;
		}

		if (m_outputs[channel])
		{
			free(m_outputs[channel]);
			m_outputs[channel] = NULL;
		}
	}

	if (m_inputs)
	{
		free(m_inputs);
		m_inputs = NULL;
	}

	if (m_outputs)
	{
		free(m_outputs);
		m_outputs = NULL;
	}
}

void VSTPlugin::loadEffectFromPath(std::string path)
{
	if (m_proxyDisconnected || m_effect != nullptr)
		return;
	
	std::lock_guard<std::recursive_mutex> grd(m_effectStatusMutex);

	blog(LOG_DEBUG, "VST Plug-in: loadEffectFromPath from pluginPath %s ", path.c_str());
	m_pluginPath = path;

	unloadEffect();
	loadEffect();

	if (!verifyProxy())
	{
		blog(LOG_WARNING, "VST Plug-in: loadEffectFromPath Can't load effect!");
		return;
	}

	// Check plug-in's magic number
	// If incorrect, then the file either was not loaded properly, is not a real VST plug-in, or is otherwise corrupt.
	if (m_effect->magic != kEffectMagic)
	{
		blog(LOG_WARNING, "VST Plug-in: loadEffectFromPath magic number is bad");
		return;
	}
	
	m_remote->dispatcher(m_effect.get(), effGetEffectName, 0, 0, m_effectName, 0, 64);
	m_remote->dispatcher(m_effect.get(), effGetVendorString, 0, 0, m_vendorString, 0, 64);
	m_remote->dispatcher(m_effect.get(), effOpen, 0, 0, nullptr, 0.0f, 0);

	// Set some default properties
	auto sampleRate = audio_output_get_sample_rate(obs_get_audio());
	m_remote->dispatcher(m_effect.get(), effSetSampleRate, 0, 0, nullptr, static_cast<float>(sampleRate), 0);

	int blocksize = BLOCK_SIZE;
	m_remote->dispatcher(m_effect.get(), effSetBlockSize, 0, blocksize, nullptr, 0.0f, 0);
	m_remote->dispatcher(m_effect.get(), effMainsChanged, 0, 1, nullptr, 0, 0);

	if (!verifyProxy())
		return;

	if (m_openInterfaceWhenActive)
		openEditor();
}

bool VSTPlugin::verifyProxy(const bool notifyAudioPause /*= false*/)
{
	if (m_effect == nullptr)
		return false;

	if (m_proxyDisconnected)
		return false;

	if (m_remote != nullptr)
	{
		m_proxyDisconnected = !m_remote->m_connected;

		if (m_proxyDisconnected)
		{
			std::string msg;

			if (notifyAudioPause)
				msg = (std::filesystem::path(m_pluginPath).filename().string() + " has stopped working.\n\nThe audio it modifies has paused and will continue after closing this popup but the filter is now disabled. You may restart the application or recreate the filter to enable it again.");
			else
				msg = (std::filesystem::path(m_pluginPath).filename().string() + " has stopped working.\n\nThe filter has been disabled. You may restart the application or recreate the filter to enable it again.");

			#ifdef WIN32
				::MessageBoxA(GetDesktopWindow(), msg.c_str(), "VST Filter Error", MB_ICONERROR | MB_SYSTEMMODAL);
			#endif

			stopProxy();
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;	
}

void silenceChannel(float **channelData, int numChannels, long numFrames)
{
	for (int channel = 0; channel < numChannels; ++channel) {
		for (long frame = 0; frame < numFrames; ++frame) {
			channelData[channel][frame] = 0.0f;
		}
	}
}

obs_audio_data *VSTPlugin::process(struct obs_audio_data *audio)
{
	if (!m_effectStatusMutex.try_lock())
		return audio;
	
	if (m_effect != nullptr && m_remote != nullptr)
	{
		uint32_t passes = (audio->frames + BLOCK_SIZE - 1) / BLOCK_SIZE;
		uint32_t extra  = audio->frames % BLOCK_SIZE;

		for (uint32_t pass = 0; pass < passes; pass++)
		{
			uint32_t frames = pass == passes - 1 && extra ? extra : BLOCK_SIZE;
			silenceChannel(m_outputs, VST_MAX_CHANNELS, BLOCK_SIZE);

			float *adata[VST_MAX_CHANNELS];

			for (size_t d = 0; d < VST_MAX_CHANNELS; d++)
			{
				if (audio->data[d] != nullptr)
					adata[d] = ((float *)audio->data[d]) + (pass * BLOCK_SIZE);
				else 
					adata[d] = m_inputs[d];
			};

			m_remote->processReplacing(m_effect.get(), adata, m_outputs, frames, VST_MAX_CHANNELS);

			if (!verifyProxy(true))
			{
				m_effectStatusMutex.unlock();
				return audio;
			}

			for (size_t c = 0; c < VST_MAX_CHANNELS; c++)
			{
				if (audio->data[c] != nullptr)
				{
					for (size_t i = 0; i < frames; i++) 
						adata[c][i] = m_outputs[c][i];
				}
			}
		}
	}

	m_effectStatusMutex.unlock();
	return audio;
}

void VSTPlugin::unloadEffect()
{
	std::lock_guard<std::recursive_mutex> grd(m_effectStatusMutex);

	m_windowCreated = false;
	m_proxyDisconnected = false;

	if (m_effect != nullptr && m_remote != nullptr)
	{
		m_remote->dispatcher(m_effect.get(), effStopProcess, 0, 0, nullptr, 0, 0);
		m_remote->dispatcher(m_effect.get(), effMainsChanged, 0, 0, nullptr, 0, 0);
		m_remote->dispatcher(m_effect.get(), effClose, 0, 0, nullptr, 0.0f, 0);
	}

	stopProxy();
}

bool VSTPlugin::isEditorOpen()
{
	return m_is_open;	
}

bool VSTPlugin::hasWindowOpen()
{
	if (m_windowCreated == false) 
		return false;

	return m_is_open;
}

void VSTPlugin::openEditor()
{
	if (isProxyDisconnected())
		return;

	if (m_effect != nullptr && m_remote != nullptr)
	{
		if (!m_windowCreated)
		{
			m_remote->sendHwndMsg(m_effect.get(), VstProxy::WM_USER_MSG::WM_USER_CREATE_WINDOW);
			m_windowCreated = true;
		}

		m_remote->sendHwndMsg(m_effect.get(), VstProxy::WM_USER_MSG::WM_USER_SHOW);
		m_is_open = true;
	}

	verifyProxy();
}

void VSTPlugin::hideEditor()
{
	if (isProxyDisconnected())
		return;
		
	if (m_windowCreated && m_effect != nullptr && m_remote != nullptr)
	{
		m_remote->sendHwndMsg(m_effect.get(), VstProxy::WM_USER_MSG::WM_USER_HIDE);
		m_is_open = false;
	}

	verifyProxy();
}

void VSTPlugin::closeEditor()
{
	m_is_open = false;

	if (m_windowCreated && m_effect != nullptr && m_remote != nullptr)
	{
		m_remote->sendHwndMsg(m_effect.get(), VstProxy::WM_USER_MSG::WM_USER_CLOSE);
		m_windowCreated = false;
	}

	verifyProxy();
}

std::string VSTPlugin::getChunk(VstChunkType type)
{
	cbase64_encodestate encoder;
	std::string encodedData;

	if (m_effect == nullptr || m_remote == nullptr)
	{
		blog(LOG_WARNING, "VST Plug-in: getChunk, no effect loaded");
		return "";
	}

	cbase64_init_encodestate(&encoder);

	if (m_effect->flags & effFlagsProgramChunks && type != VstChunkType::Parameter)
	{
		void *buf = nullptr;
		intptr_t chunkSize = m_remote->dispatcher(m_effect.get(), effGetChunk, int(type), 0, &buf, 0.0, 0);

		if (!verifyProxy())
			return "";

		if (!buf || chunkSize==0)
		{
			blog(LOG_WARNING, "VST Plug-in: effGetChunk failed");
			return "";
		}

		encodedData.resize(cbase64_calc_encoded_length(uint32_t(chunkSize)));

		int blockEnd = 
		cbase64_encode_block((const unsigned char*)buf, uint32_t(chunkSize), &encodedData[0], &encoder);
		cbase64_encode_blockend(&encodedData[blockEnd], &encoder);
		return encodedData;
	}
	else if (!(m_effect->flags & effFlagsProgramChunks) && type == VstChunkType::Parameter)
	{
		std::vector<float> params;

		for (int i = 0; i < m_effect->numParams; i++)
		{
			float parameter = m_remote->getParameter(m_effect.get(), i);
			params.push_back(parameter);
		}

		if (!verifyProxy())
			return "";

		if (!params.empty())
		{
			const char *bytes = reinterpret_cast<const char *>(&params[0]);
			size_t size = sizeof(float) * params.size();

			encodedData.resize(cbase64_calc_encoded_length(uint32_t(size)));

			int blockEnd = cbase64_encode_block((const unsigned char*)bytes, uint32_t(size), &encodedData[0], &encoder);
			cbase64_encode_blockend(&encodedData[blockEnd], &encoder);
		}
		else
		{
			blog(LOG_WARNING, "VST Plug-in: getChunk params.empty()");
		}

		return encodedData;
	}

	// Not every VST uses every type, we don't need to know about that in the logs
	//blog(LOG_INFO, "VST Plug-in: getChunk option unavailable");
	return "";
}

void VSTPlugin::setChunk(VstChunkType type, std::string & data)
{
	if (data.size() == 0)
	{
		blog(LOG_DEBUG, "VST Plug-in: setChunk with empty data chunk ignored");
		return;
	}

	cbase64_decodestate decoder;
	cbase64_init_decodestate(&decoder);
	std::string decodedData;
	
	if (m_effect == nullptr || m_remote == nullptr)
	{
		blog(LOG_ERROR, "VST Plug-in: setChunk effect is not ready yet");
		return;
	}

	decodedData.resize(cbase64_calc_decoded_length(data.data(), uint32_t(data.size())));
	cbase64_decode_block(data.data(), uint32_t(data.size()), (unsigned char*)&decodedData[0], &decoder);
	data = "";
	
	if (m_effect->flags & effFlagsProgramChunks && type != VstChunkType::Parameter)
	{
		auto ret = m_remote->dispatcher(m_effect.get(), effSetChunk, type == VstChunkType::Bank ? 0 : 1, decodedData.length(), &decodedData[0], 0.0, decodedData.length());
	}
	else if (!(m_effect->flags & effFlagsProgramChunks) && type == VstChunkType::Parameter)
	{
		const char * p_chars  = &decodedData[0];
		const float *p_floats = reinterpret_cast<const float *>(p_chars);

		int size = uint32_t(decodedData.length()) / sizeof(float);

		std::vector<float> params(p_floats, p_floats + size);

		if (params.size() != (size_t)m_effect->numParams)
		{
			blog(LOG_WARNING, "VST Plug-in: setChunk wrong number of params");
			return;
		}

		for (int i = 0; i < m_effect->numParams; i++)
			m_remote->setParameter(m_effect.get(), i, params[i]);
	}

	verifyProxy();
}

void VSTPlugin::setProgram(const int programNumber)
{
	if (m_effect == nullptr || m_remote == nullptr)
	{
		blog(LOG_ERROR, "VST Plug-in: setProgram effect is not ready yet");
		return;
	}

	if (programNumber < m_effect->numPrograms)
	{
		intptr_t ret = m_remote->dispatcher(m_effect.get(), effSetProgram, 0, programNumber, nullptr, 0.0f, 0);
		blog(LOG_ERROR, "VST Plug-in: setProgram get %lld from effSetProgram", ret);
	}
	else
	{
		blog(LOG_ERROR, "VST Plug-in: setProgram Failed to load program, number was outside possible program range.");
	}

	verifyProxy();
}

int VSTPlugin::getProgram()
{
	if (m_effect == nullptr || m_remote == nullptr)
	{
		blog(LOG_WARNING, "VST Plug-in: getProgram effect is not ready yet");
		return 0;
	}

	intptr_t ret = m_remote->dispatcher(m_effect.get(), effGetProgram, 0, 0, nullptr, 0.0f, 0);
	verifyProxy();
	return static_cast<int>(ret);
}

void VSTPlugin::getSourceNames()
{
	/* Only call inside the vst_filter_audio function! */
	m_sourceName = obs_source_get_name(obs_filter_get_target(m_sourceContext));
	m_filterName = obs_source_get_name(m_sourceContext);
}

std::string VSTPlugin::getPluginPath()
{
	return m_pluginPath;
}
