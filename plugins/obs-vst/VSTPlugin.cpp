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

#include "headers/VSTPlugin.h"
#include <util/platform.h>

intptr_t VSTPlugin::hostCallback_static(AEffect *effect, int32_t opcode,
					int32_t index, intptr_t value,
					void *ptr, float opt)
{
	UNUSED_PARAMETER(opt);
	UNUSED_PARAMETER(ptr);

	VSTPlugin *plugin = nullptr;
	if (effect && effect->user) {
		plugin = static_cast<VSTPlugin *>(effect->user);
	}

	switch (opcode) {
	case audioMasterVersion:
		return (intptr_t)2400;

	case audioMasterGetCurrentProcessLevel:
		return 1;

	// We always replace, never accumulate
	case audioMasterWillReplaceOrAccumulate:
		return 1;

	case audioMasterGetSampleRate:
		if (plugin) {
			return (intptr_t)plugin->GetSampleRate();
		}
		return 0;

	case audioMasterGetTime:
		if (plugin) {
			return (intptr_t)plugin->GetTimeInfo();
		}
		return 0;

	// index: width, value: height
	case audioMasterSizeWindow:
		if (plugin && plugin->editorWidget) {
			plugin->editorWidget->handleResizeRequest(index, value);
		}
		return 1;

	default:
		return 0;
	}
}

VstTimeInfo *VSTPlugin::GetTimeInfo()
{
	mTimeInfo.nanoSeconds = os_gettime_ns() / 1000000;
	return &mTimeInfo;
}

float VSTPlugin::GetSampleRate()
{
	return mTimeInfo.sampleRate;
}

VSTPlugin::VSTPlugin(obs_source_t *sourceContext) : sourceContext{sourceContext}
{
}

VSTPlugin::~VSTPlugin()
{
	unloadEffect();

	cleanupChannelBuffers();
}

void VSTPlugin::createChannelBuffers(size_t count)
{
	cleanupChannelBuffers();

	int blocksize = BLOCK_SIZE;
	numChannels = (std::max)((size_t)0, count);

	if (numChannels > 0) {
		inputs = (float **)malloc(sizeof(float *) * numChannels);
		outputs = (float **)malloc(sizeof(float *) * numChannels);
		channelrefs = (float **)malloc(sizeof(float *) * numChannels);
		for (size_t channel = 0; channel < numChannels; channel++) {
			inputs[channel] =
				(float *)malloc(sizeof(float) * blocksize);
			outputs[channel] =
				(float *)malloc(sizeof(float) * blocksize);
		}
	}
}

void VSTPlugin::cleanupChannelBuffers()
{
	for (size_t channel = 0; channel < numChannels; channel++) {
		if (inputs && inputs[channel]) {
			free(inputs[channel]);
			inputs[channel] = NULL;
		}
		if (outputs && outputs[channel]) {
			free(outputs[channel]);
			outputs[channel] = NULL;
		}
	}
	if (inputs) {
		free(inputs);
		inputs = NULL;
	}
	if (outputs) {
		free(outputs);
		outputs = NULL;
	}
	if (channelrefs) {
		free(channelrefs);
		channelrefs = NULL;
	}
	numChannels = 0;
}

void VSTPlugin::loadEffectFromPath(std::string path)
{
	if (this->pluginPath.compare(path) != 0) {
		unloadEffect();
		blog(LOG_INFO, "User selected new VST plugin: '%s'",
		     path.c_str());
	}

	if (!effect) {
		// TODO: alert user of error if VST is not available.

		pluginPath = path;

		AEffect *effectTemp = loadEffect();
		if (!effectTemp) {
			blog(LOG_WARNING, "VST Plug-in: Can't load effect!");
			return;
		}

		{
			std::lock_guard<std::recursive_mutex> lock(lockEffect);
			effect = effectTemp;
		}

		// Check plug-in's magic number
		// If incorrect, then the file either was not loaded properly,
		// is not a real VST plug-in, or is otherwise corrupt.
		if (effect->magic != kEffectMagic) {
			blog(LOG_WARNING, "VST Plug-in's magic number is bad");
			return;
		}

		int maxchans =
			(std::max)(effect->numInputs, effect->numOutputs);
		// sanity check
		if (maxchans < 0 || maxchans > 256) {
			blog(LOG_WARNING,
			     "VST Plug-in has invalid number of channels");
			return;
		}

		createChannelBuffers(maxchans);

		// It is better to invoke this code after checking magic number
		effect->dispatcher(effect, effGetEffectName, 0, 0, effectName,
				   0);
		effect->dispatcher(effect, effGetVendorString, 0, 0,
				   vendorString, 0);

		// This check logic is refer to open source project : Audacity
		if ((effect->flags & effFlagsIsSynth) ||
		    !(effect->flags & effFlagsCanReplacing)) {
			blog(LOG_WARNING,
			     "VST Plug-in can't support replacing. '%s'",
			     path.c_str());
			return;
		}

		// Ask the plugin to identify itself...might be needed for older plugins
		effect->dispatcher(effect, effIdentify, 0, 0, nullptr, 0.0f);

		effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);

		// Set some default properties
		size_t sampleRate =
			audio_output_get_sample_rate(obs_get_audio());

		// Initialize time info
		memset(&mTimeInfo, 0, sizeof(mTimeInfo));
		mTimeInfo.sampleRate = sampleRate;
		mTimeInfo.nanoSeconds = os_gettime_ns() / 1000000;
		mTimeInfo.tempo = 120.0;
		mTimeInfo.timeSigNumerator = 4;
		mTimeInfo.timeSigDenominator = 4;
		mTimeInfo.flags = kVstTempoValid | kVstNanosValid |
				  kVstTransportPlaying;

		effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr,
				   sampleRate);
		int blocksize = BLOCK_SIZE;
		effect->dispatcher(effect, effSetBlockSize, 0, blocksize,
				   nullptr, 0.0f);

		effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0);

		effectReady = true;

		if (openInterfaceWhenActive) {
			openEditor();
		}
	}
}

static void silenceChannel(float **channelData, size_t numChannels,
			   long numFrames)
{
	for (size_t channel = 0; channel < numChannels; ++channel) {
		for (long frame = 0; frame < numFrames; ++frame) {
			channelData[channel][frame] = 0.0f;
		}
	}
}

obs_audio_data *VSTPlugin::process(struct obs_audio_data *audio)
{
	// Here we check the status firstly,
	// which help avoid waiting for lock while unloadEffect() is running.
	bool effectValid = (effect && effectReady && numChannels > 0);
	if (!effectValid)
		return audio;

	std::lock_guard<std::recursive_mutex> lock(lockEffect);

	if (effect && effectReady && numChannels > 0) {
		uint passes = (audio->frames + BLOCK_SIZE - 1) / BLOCK_SIZE;
		uint extra = audio->frames % BLOCK_SIZE;
		for (uint pass = 0; pass < passes; pass++) {
			uint frames = pass == passes - 1 && extra ? extra
								  : BLOCK_SIZE;
			silenceChannel(outputs, numChannels, BLOCK_SIZE);

			for (size_t d = 0; d < numChannels; d++) {
				if (d < MAX_AV_PLANES &&
				    audio->data[d] != nullptr) {
					channelrefs[d] =
						((float *)audio->data[d]) +
						(pass * BLOCK_SIZE);
				} else {
					channelrefs[d] = inputs[d];
				}
			};

			effect->processReplacing(effect, channelrefs, outputs,
						 frames);

			// only copy back the channels the plugin may have generated
			for (size_t c = 0; c < (size_t)effect->numOutputs &&
					   c < MAX_AV_PLANES;
			     c++) {
				if (audio->data[c]) {
					for (size_t i = 0; i < frames; i++) {
						channelrefs[c][i] =
							outputs[c][i];
					}
				}
			}
		}
	}

	return audio;
}

void VSTPlugin::unloadEffect()
{
	closeEditor();

	{
		std::lock_guard<std::recursive_mutex> lock(lockEffect);

		// Reset the status firstly to avoid VSTPlugin::process is blocked
		effectReady = false;

		if (effect) {
			effect->dispatcher(effect, effMainsChanged, 0, 0,
					   nullptr, 0);
			effect->dispatcher(effect, effClose, 0, 0, nullptr,
					   0.0f);
		}

		effect = nullptr;
	}

	unloadLibrary();

	pluginPath = "";
}

bool VSTPlugin::isEditorOpen()
{
	return editorWidget ? true : false;
}

void VSTPlugin::onEditorClosed()
{
	if (!editorWidget)
		return;

	editorWidget->deleteLater();
	editorWidget = nullptr;

	if (effect && editorOpened) {
		editorOpened = false;
		effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0);
	}
}

void VSTPlugin::openEditor()
{
	if (effect && !editorWidget) {
		// This check logic is refer to open source project : Audacity
		if (!(effect->flags & effFlagsHasEditor)) {
			blog(LOG_WARNING,
			     "VST Plug-in: Can't support edit feature. '%s'",
			     pluginPath.c_str());
			return;
		}

		editorOpened = true;
		editorWidget = new EditorWidget(nullptr, this);
		editorWidget->buildEffectContainer(effect);

		if (sourceName.empty()) {
			sourceName = "VST 2.x";
		}

		if (filterName.empty()) {
			editorWidget->setWindowTitle(QString("%1 - %2").arg(
				sourceName.c_str(), effectName));
		} else {
			editorWidget->setWindowTitle(
				QString("%1: %2 - %3")
					.arg(sourceName.c_str(),
					     filterName.c_str(), effectName));
		}
		editorWidget->show();
	}
}

void VSTPlugin::closeEditor()
{
	if (editorWidget)
		editorWidget->close();
}

std::string VSTPlugin::getEffectPath()
{
	return pluginPath;
}

std::string VSTPlugin::getChunk()
{
	if (!effect) {
		return "";
	}

	if (effect->flags & effFlagsProgramChunks) {
		void *buf = nullptr;

		intptr_t chunkSize = effect->dispatcher(effect, effGetChunk, 1,
							0, &buf, 0.0);

		QByteArray data = QByteArray((char *)buf, chunkSize);
		return QString(data.toBase64()).toStdString();
	} else {
		std::vector<float> params;
		for (int i = 0; i < effect->numParams; i++) {
			float parameter = effect->getParameter(effect, i);
			params.push_back(parameter);
		}

		const char *bytes = reinterpret_cast<const char *>(&params[0]);
		QByteArray data =
			QByteArray(bytes, (int)(sizeof(float) * params.size()));
		std::string encoded = QString(data.toBase64()).toStdString();
		return encoded;
	}
}

void VSTPlugin::setChunk(std::string data)
{
	if (!effect) {
		return;
	}

	if (effect->flags & effFlagsProgramChunks) {
		QByteArray base64Data =
			QByteArray(data.c_str(), (int)data.length());
		QByteArray chunkData = QByteArray::fromBase64(base64Data);
		void *buf = nullptr;
		buf = chunkData.data();
		effect->dispatcher(effect, effSetChunk, 1, chunkData.length(),
				   buf, 0);
	} else {
		QByteArray base64Data =
			QByteArray(data.c_str(), (int)data.length());
		QByteArray paramData = QByteArray::fromBase64(base64Data);

		const char *p_chars = paramData.data();
		const float *p_floats =
			reinterpret_cast<const float *>(p_chars);

		const size_t size = paramData.length() / sizeof(float);

		std::vector<float> params(p_floats, p_floats + size);

		if (params.size() != (size_t)effect->numParams) {
			return;
		}

		for (int i = 0; i < effect->numParams; i++) {
			effect->setParameter(effect, i, params[i]);
		}
	}
}

void VSTPlugin::setProgram(const int programNumber)
{
	if (programNumber < effect->numPrograms) {
		effect->dispatcher(effect, effSetProgram, 0, programNumber,
				   NULL, 0.0f);
	} else {
		blog(LOG_ERROR,
		     "Failed to load program, number was outside possible program range.");
	}
}

int VSTPlugin::getProgram()
{
	return effect->dispatcher(effect, effGetProgram, 0, 0, NULL, 0.0f);
}

void VSTPlugin::getSourceNames()
{
	/* Only call inside the vst_filter_audio function! */
	sourceName = obs_source_get_name(obs_filter_get_parent(sourceContext));
	filterName = obs_source_get_name(sourceContext);
}
