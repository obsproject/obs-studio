/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <CarlaBackendUtils.hpp>
#include <CarlaBase64Utils.hpp>
#include <CarlaBinaryUtils.hpp>

#ifdef __APPLE__
#include <CarlaMacUtils.hpp>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtWidgets/QMessageBox>

#include <util/platform.h>

#include "carla-bridge.hpp"

#include "common.h"
#include "qtutils.h"

#if defined(__APPLE__) && defined(__aarch64__)
/* check the header of a plugin binary to see if it matches mach 64bit + intel */
static bool isIntel64BitPlugin(const char *const pluginBundle)
{
	const char *const pluginBinary = findBinaryInBundle(pluginBundle);
	CARLA_SAFE_ASSERT_RETURN(pluginBinary != nullptr, false);

	FILE *const f = fopen(pluginBinary, "r");
	CARLA_SAFE_ASSERT_RETURN(f != nullptr, false);

	bool match = false;
	uint8_t buf[8];
	if (fread(buf, sizeof(buf), 1, f) == 1) {
		const uint32_t magic = *(uint32_t *)buf;
		if (magic == 0xfeedfacf && buf[4] == 0x07)
			match = true;
	}

	fclose(f);
	return match;
}
#endif

/* utility class for reading and deleting incoming bridge text in RAII fashion */
struct BridgeTextReader {
	char *text = nullptr;

	BridgeTextReader(BridgeNonRtServerControl &nonRtServerCtrl)
	{
		const uint32_t size = nonRtServerCtrl.readUInt();
		CARLA_SAFE_ASSERT_RETURN(size != 0, );

		text = new char[size + 1];
		nonRtServerCtrl.readCustomData(text, size);
		text[size] = '\0';
	}

	BridgeTextReader(BridgeNonRtServerControl &nonRtServerCtrl,
			 const uint32_t size)
	{
		text = new char[size + 1];

		if (size != 0)
			nonRtServerCtrl.readCustomData(text, size);

		text[size] = '\0';
	}

	~BridgeTextReader() noexcept { delete[] text; }

	CARLA_DECLARE_NON_COPYABLE(BridgeTextReader)
};

/* custom bridge process implementation */

BridgeProcess::BridgeProcess(const char *const shmIds)
{
	/* move object to the correct/expected thread */
	moveToThread(qApp->thread());

	/* setup environment for client side */
	QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
	env.insert("ENGINE_BRIDGE_SHM_IDS", shmIds);
	setProcessEnvironment(env);
}

void BridgeProcess::start()
{
	/* pass-through all bridge output */
	setInputChannelMode(QProcess::ForwardedInputChannel);
	setProcessChannelMode(QProcess::ForwardedChannels);
	QProcess::start(QIODevice::Unbuffered | QIODevice::ReadOnly);
}

/* NOTE: process instance cannot be used after this! */
void BridgeProcess::stop()
{
	if (state() != QProcess::NotRunning) {
		terminate();
		waitForFinished(2000);

		if (state() != QProcess::NotRunning) {
			blog(LOG_INFO,
			     "[carla] bridge refused to close, force kill now");
			kill();
		}
	}

	deleteLater();
}

/* carla bridge implementation */

bool carla_bridge::init(uint32_t maxBufferSize, double sampleRate)
{
	/* add entropy to rand calls, used for finding unused paths */
	std::srand(static_cast<uint>(os_gettime_ns() / 1000000));

	/* initialize the several communication channels */
	if (!audiopool.initializeServer()) {
		blog(LOG_WARNING,
		     "[carla] Failed to initialize shared memory audio pool");
		goto fail1;
	}

	if (!rtClientCtrl.initializeServer()) {
		blog(LOG_WARNING,
		     "[carla] Failed to initialize RT client control");
		goto fail2;
	}

	if (!nonRtClientCtrl.initializeServer()) {
		blog(LOG_WARNING,
		     "[carla] Failed to initialize Non-RT client control");
		goto fail3;
	}

	if (!nonRtServerCtrl.initializeServer()) {
		blog(LOG_WARNING,
		     "[carla] Failed to initialize Non-RT server control");
		goto fail4;
	}

	/* resize audiopool data to be as large as needed */
	audiopool.resize(maxBufferSize, MAX_AV_PLANES, MAX_AV_PLANES);

	/* clear realtime data */
	rtClientCtrl.data->procFlags = 0;
	carla_zeroStruct(rtClientCtrl.data->timeInfo);
	carla_zeroBytes(rtClientCtrl.data->midiOut,
			kBridgeRtClientDataMidiOutSize);

	/* clear ringbuffers */
	rtClientCtrl.clearData();
	nonRtClientCtrl.clearData();
	nonRtServerCtrl.clearData();

	/* first ever message is bridge API version */
	nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientVersion);
	nonRtClientCtrl.writeUInt(CARLA_PLUGIN_BRIDGE_API_VERSION_CURRENT);

	/* then expected size for each data channel */
	nonRtClientCtrl.writeUInt(
		static_cast<uint32_t>(sizeof(BridgeRtClientData)));
	nonRtClientCtrl.writeUInt(
		static_cast<uint32_t>(sizeof(BridgeNonRtClientData)));
	nonRtClientCtrl.writeUInt(
		static_cast<uint32_t>(sizeof(BridgeNonRtServerData)));

	/* and finally the initial buffer size and sample rate */
	nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientInitialSetup);
	nonRtClientCtrl.writeUInt(maxBufferSize);
	nonRtClientCtrl.writeDouble(sampleRate);

	nonRtClientCtrl.commitWrite();

	/* report audiopool size to client side */
	rtClientCtrl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
	rtClientCtrl.writeULong(static_cast<uint64_t>(audiopool.dataSize));
	rtClientCtrl.commitWrite();

	/* FIXME maybe not needed */
	rtClientCtrl.writeOpcode(kPluginBridgeRtClientSetBufferSize);
	rtClientCtrl.writeUInt(maxBufferSize);
	rtClientCtrl.commitWrite();

	bufferSize = maxBufferSize;
	return true;

fail4:
	nonRtClientCtrl.clear();

fail3:
	rtClientCtrl.clear();

fail2:
	audiopool.clear();

fail1:
	setLastError("Failed to initialize shared memory");
	return false;
}

void carla_bridge::cleanup(const bool clearPluginData)
{
	/* signal to stop processing audio */
	const bool wasActivated = activated;
	ready = activated = false;

	/* stop bridge process */
	if (childprocess != nullptr) {
		/* make `childprocess` null first */
		BridgeProcess *proc = childprocess;
		childprocess = nullptr;

		/* if process is running, ask nicely for it to close */
		if (proc->state() != QProcess::NotRunning) {
			{
				const CarlaMutexLocker cml(
					nonRtClientCtrl.mutex);

				if (wasActivated) {
					nonRtClientCtrl.writeOpcode(
						kPluginBridgeNonRtClientDeactivate);
					nonRtClientCtrl.commitWrite();
				}

				nonRtClientCtrl.writeOpcode(
					kPluginBridgeNonRtClientQuit);
				nonRtClientCtrl.commitWrite();
			}

			rtClientCtrl.writeOpcode(kPluginBridgeRtClientQuit);
			rtClientCtrl.commitWrite();

			if (!timedErr && !timedOut)
				wait("stopping", 3000);
		} else {
			/* log warning in case plugin process crashed */
			if (proc->exitStatus() == QProcess::CrashExit) {
				blog(LOG_WARNING, "[carla] bridge crashed");

				if (!clearPluginData) {
					carla_show_error_dialog(
						"A plugin bridge has crashed",
						info.name);
				}
			}
		}

		/* let Qt do the final cleanup on the main thread */
		QMetaObject::invokeMethod(proc, "stop");
	}

	/* cleanup shared memory bits */
	nonRtServerCtrl.clear();
	nonRtClientCtrl.clear();
	rtClientCtrl.clear();
	audiopool.clear();

	/* clear cached plugin data if requested */
	if (clearPluginData) {
		info.clear();
		chunk.clear();
		clear_custom_data();
	}
}

bool carla_bridge::start(const BinaryType btype, const PluginType ptype,
			 const char *label, const char *filename,
			 const int64_t uniqueId)
{
	/* make sure we are trying to load something valid */
	if (btype == BINARY_NONE || ptype == PLUGIN_NONE) {
		setLastError("Invalid plugin state");
		return false;
	}

	/* find path to bridge binary */
	QString bridgeBinary(QString::fromUtf8(get_carla_bin_path()));

	if (btype == BINARY_NATIVE) {
		bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-native";
	} else {
		switch (btype) {
		case BINARY_POSIX32:
			bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-posix32";
			break;
		case BINARY_POSIX64:
			bridgeBinary += CARLA_OS_SEP_STR "carla-bridge-posix64";
			break;
		case BINARY_WIN32:
			bridgeBinary += CARLA_OS_SEP_STR
				"carla-bridge-win32.exe";
			break;
		case BINARY_WIN64:
			bridgeBinary += CARLA_OS_SEP_STR
				"carla-bridge-win64.exe";
			break;
		default:
			bridgeBinary.clear();
			break;
		}
	}

	if (bridgeBinary.isEmpty() || !QFileInfo(bridgeBinary).isExecutable()) {
		setLastError("Required plugin bridge is not available");
		return false;
	}

	/* create string of shared memory ids to pass into the bridge process */
	char shmIdsStr[6 * 4 + 1] = {};

	size_t len = audiopool.filename.length();
	CARLA_SAFE_ASSERT_RETURN(len > 6, false);
	std::strncpy(shmIdsStr, &audiopool.filename[len - 6], 6);

	len = rtClientCtrl.filename.length();
	CARLA_SAFE_ASSERT_RETURN(len > 6, false);
	std::strncpy(shmIdsStr + 6, &rtClientCtrl.filename[len - 6], 6);

	len = nonRtClientCtrl.filename.length();
	CARLA_SAFE_ASSERT_RETURN(len > 6, false);
	std::strncpy(shmIdsStr + 12, &nonRtClientCtrl.filename[len - 6], 6);

	len = nonRtServerCtrl.filename.length();
	CARLA_SAFE_ASSERT_RETURN(len > 6, false);
	std::strncpy(shmIdsStr + 18, &nonRtServerCtrl.filename[len - 6], 6);

	/* create bridge process and setup arguments */
	BridgeProcess *proc = new BridgeProcess(shmIdsStr);

	QStringList arguments;

#if defined(__APPLE__) && defined(__aarch64__)
	/* see if this binary needs special help (x86_64 plugins under arm64 systems) */
	switch (ptype) {
	case PLUGIN_VST2:
	case PLUGIN_VST3:
	case PLUGIN_CLAP:
		if (isIntel64BitPlugin(filename)) {
			/* TODO we need to hook into qprocess for:
			 * posix_spawnattr_setbinpref_np + CPU_TYPE_X86_64
			 */
			arguments.append("-arch");
			arguments.append("x86_64");
			arguments.append(bridgeBinary);
			bridgeBinary = "arch";
		}
	default:
		break;
	}
#endif

	/* do not use null strings for label and filename */
	if (label == nullptr || label[0] == '\0')
		label = "(none)";
	if (filename == nullptr || filename[0] == '\0')
		filename = "(none)";

	/* arg 1: plugin type */
	arguments.append(QString::fromUtf8(getPluginTypeAsString(ptype)));

	/* arg 2: filename */
	arguments.append(QString::fromUtf8(filename));

	/* arg 3: label */
	arguments.append(QString::fromUtf8(label));

	/* arg 4: uniqueId */
	arguments.append(QString::number(uniqueId));

	proc->setProgram(bridgeBinary);
	proc->setArguments(arguments);

	/* start process on main thread */
	QMetaObject::invokeMethod(proc, "start");

	/* check if it started correctly */
	const bool started = proc->waitForStarted(5000);

	if (!started) {
		QMetaObject::invokeMethod(proc, "stop");
		setLastError("Plugin bridge failed to start");
		return false;
	}

	/* wait for plugin process to start talking to us */
	ready = false;
	timedErr = false;
	timedOut = false;

	const uint64_t start_time = os_gettime_ns();

	/* NOTE: we cannot rely on `proc->state() == QProcess::Running` here
	 * as Qt only updates QProcess state on main thread
	 */
	while (proc != nullptr && !ready && !timedErr) {
		os_sleep_ms(5);

		/* timeout after 5s */
		if (os_gettime_ns() - start_time > 5 * 1000000000ULL)
			break;

		readMessages();
	}

	if (!ready) {
		QMetaObject::invokeMethod(proc, "stop");
		if (!timedErr)
			setLastError(
				"Timeout while waiting for plugin bridge to start");
		return false;
	}

	/* refuse to load plugin with incompatible IO */
	if (info.hasCV || info.numAudioIns > MAX_AV_PLANES ||
	    info.numAudioOuts > MAX_AV_PLANES) {
		/* tell bridge process to quit */
		nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientQuit);
		nonRtClientCtrl.commitWrite();
		rtClientCtrl.writeOpcode(kPluginBridgeRtClientQuit);
		rtClientCtrl.commitWrite();
		wait("stopping", 3000);
		QMetaObject::invokeMethod(proc, "stop");

		/* cleanup shared memory bits */
		nonRtServerCtrl.clear();
		nonRtClientCtrl.clear();
		rtClientCtrl.clear();
		audiopool.clear();

		/* also clear cached info */
		info.clear();
		chunk.clear();
		clear_custom_data();
		delete[] paramDetails;
		paramDetails = nullptr;
		paramCount = 0;

		setLastError("Selected plugin has IO incompatible with OBS");
		return false;
	}

	/* cache relevant information for later */
	info.btype = btype;
	info.ptype = ptype;
	info.filename = filename;
	info.label = label;
	info.uniqueId = uniqueId;

	/* finally assign childprocess and set active */
	childprocess = proc;

	return true;
}

bool carla_bridge::is_running() const
{
	return childprocess != nullptr &&
	       childprocess->state() == QProcess::Running;
}

bool carla_bridge::idle()
{
	if (childprocess == nullptr)
		return false;

	switch (childprocess->state()) {
	case QProcess::Running:
		if (!pendingPing) {
			pendingPing = true;

			const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientPing);
			nonRtClientCtrl.commitWrite();
		}
		break;
	case QProcess::NotRunning:
		activated = false;
		timedErr = true;
		cleanup(false);
		return false;
	default:
		return false;
	}

	if (timedOut && activated) {
		deactivate();
		return idle();
	}

	try {
		readMessages();
	}
	CARLA_SAFE_EXCEPTION("readMessages");

	return true;
}

bool carla_bridge::wait(const char *const action, const uint msecs)
{
	CARLA_SAFE_ASSERT_RETURN(!timedErr, false);
	CARLA_SAFE_ASSERT_RETURN(!timedOut, false);

	if (rtClientCtrl.waitForClient(msecs))
		return true;

	timedOut = true;
	blog(LOG_WARNING, "[carla] wait(%s) timed out", action);
	return false;
}

void carla_bridge::set_value(uint index, float value)
{
	CARLA_SAFE_ASSERT_UINT2_RETURN(index < paramCount, index, paramCount, );

	paramDetails[index].value = value;

	if (is_running()) {
		const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

		nonRtClientCtrl.writeOpcode(
			kPluginBridgeNonRtClientSetParameterValue);
		nonRtClientCtrl.writeUInt(index);
		nonRtClientCtrl.writeFloat(value);
		nonRtClientCtrl.commitWrite();

		if (info.hints & PLUGIN_HAS_CUSTOM_UI) {
			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientUiParameterChange);
			nonRtClientCtrl.writeUInt(index);
			nonRtClientCtrl.writeFloat(value);
			nonRtClientCtrl.commitWrite();
		}

		nonRtClientCtrl.waitIfDataIsReachingLimit();
	}
}

void carla_bridge::show_ui()
{
	if (is_running()) {
		const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

		nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientShowUI);
		nonRtClientCtrl.commitWrite();
	}
}

bool carla_bridge::is_active() const noexcept
{
	return activated;
}

void carla_bridge::activate()
{
	if (activated)
		return;

	if (is_running()) {
		{
			const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientActivate);
			nonRtClientCtrl.commitWrite();
		}

		try {
			wait("activate", 2000);
		}
		CARLA_SAFE_EXCEPTION("activate - waitForClient");

		activated = true;
	}
}

void carla_bridge::deactivate()
{
	CARLA_SAFE_ASSERT_RETURN(activated, );

	activated = false;
	timedErr = false;
	timedOut = false;

	if (is_running()) {
		{
			const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientDeactivate);
			nonRtClientCtrl.commitWrite();
		}

		try {
			wait("deactivate", 2000);
		}
		CARLA_SAFE_EXCEPTION("deactivate - waitForClient");
	}
}

void carla_bridge::reload()
{
	ready = false;
	timedErr = false;
	timedOut = false;

	if (activated)
		deactivate();

	if (is_running()) {
		{
			const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientReload);
			nonRtClientCtrl.commitWrite();
		}
	}

	activate();

	if (is_running()) {
		try {
			wait("reload", 2000);
		}
		CARLA_SAFE_EXCEPTION("reload - waitForClient");
	}

	/* wait for plugin process to start talking back to us */
	const uint64_t start_time = os_gettime_ns();

	while (childprocess != nullptr && !ready) {
		os_sleep_ms(5);

		/* timeout after 1s */
		if (os_gettime_ns() - start_time > 1000000000ULL)
			break;

		readMessages();
	}
}

void carla_bridge::restore_state()
{
	const uint32_t maxLocalValueLen = clientBridgeVersion >= 10 ? 4096
								    : 16384;

	const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

	for (CustomData &cdata : customData) {
		const uint32_t typeLen =
			static_cast<uint32_t>(std::strlen(cdata.type));
		const uint32_t keyLen =
			static_cast<uint32_t>(std::strlen(cdata.key));
		const uint32_t valueLen =
			static_cast<uint32_t>(std::strlen(cdata.value));

		nonRtClientCtrl.writeOpcode(
			kPluginBridgeNonRtClientSetCustomData);

		nonRtClientCtrl.writeUInt(typeLen);
		nonRtClientCtrl.writeCustomData(cdata.type, typeLen);

		nonRtClientCtrl.writeUInt(keyLen);
		nonRtClientCtrl.writeCustomData(cdata.key, keyLen);

		nonRtClientCtrl.writeUInt(valueLen);

		if (valueLen > 0) {
			if (valueLen > maxLocalValueLen) {
				QString filePath(QDir::tempPath());

				filePath += CARLA_OS_SEP_STR
					".CarlaCustomData_";
				filePath += audiopool.getFilenameSuffix();

				QFile file(filePath);
				if (file.open(QIODevice::WriteOnly) &&
				    file.write(cdata.value) !=
					    static_cast<qint64>(valueLen)) {
					const uint32_t ulength =
						static_cast<uint32_t>(
							filePath.length());

					nonRtClientCtrl.writeUInt(ulength);
					nonRtClientCtrl.writeCustomData(
						filePath.toUtf8().constData(),
						ulength);
				} else {
					nonRtClientCtrl.writeUInt(0);
				}
			} else {
				nonRtClientCtrl.writeCustomData(cdata.value,
								valueLen);
			}
		}

		nonRtClientCtrl.commitWrite();

		nonRtClientCtrl.waitIfDataIsReachingLimit();
	}

	if (info.ptype == PLUGIN_LV2) {
		nonRtClientCtrl.writeOpcode(
			kPluginBridgeNonRtClientRestoreLV2State);
		nonRtClientCtrl.commitWrite();
	}

	if (info.options & PLUGIN_OPTION_USE_CHUNKS) {
		QString filePath(QDir::tempPath());

		filePath += CARLA_OS_SEP_STR ".CarlaChunk_";
		filePath += audiopool.getFilenameSuffix();

		QFile file(filePath);
		if (file.open(QIODevice::WriteOnly) &&
		    file.write(CarlaString::asBase64(chunk.data(), chunk.size())
				       .buffer()) != 0) {
			file.close();

			const uint32_t ulength =
				static_cast<uint32_t>(filePath.length());

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientSetChunkDataFile);
			nonRtClientCtrl.writeUInt(ulength);
			nonRtClientCtrl.writeCustomData(
				filePath.toUtf8().constData(), ulength);
			nonRtClientCtrl.commitWrite();

			nonRtClientCtrl.waitIfDataIsReachingLimit();
		}
	} else {
		for (uint32_t i = 0; i < paramCount; ++i) {
			const carla_param_data &param(paramDetails[i]);

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientSetParameterValue);
			nonRtClientCtrl.writeUInt(i);
			nonRtClientCtrl.writeFloat(param.value);
			nonRtClientCtrl.commitWrite();

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientUiParameterChange);
			nonRtClientCtrl.writeUInt(i);
			nonRtClientCtrl.writeFloat(param.value);
			nonRtClientCtrl.commitWrite();

			nonRtClientCtrl.waitIfDataIsReachingLimit();
		}
	}
}

void carla_bridge::process(float *buffers[MAX_AV_PLANES], const uint32_t frames)
{
	if (!ready || !activated)
		return;

	rtClientCtrl.data->timeInfo.usecs = os_gettime_ns() / 1000;

	for (uint32_t c = 0; c < MAX_AV_PLANES; ++c)
		carla_copyFloats(audiopool.data + (c * bufferSize), buffers[c],
				 frames);

	rtClientCtrl.writeOpcode(kPluginBridgeRtClientProcess);
	rtClientCtrl.writeUInt(frames);
	rtClientCtrl.commitWrite();

	if (wait("process", 1000)) {
		for (uint32_t c = 0; c < MAX_AV_PLANES; ++c)
			carla_copyFloats(
				buffers[c],
				audiopool.data +
					((c + info.numAudioIns) * bufferSize),
				frames);
	}
}

void carla_bridge::add_custom_data(const char *const type,
				   const char *const key,
				   const char *const value,
				   const bool sendToPlugin)
{
	CARLA_SAFE_ASSERT_RETURN(type != nullptr && type[0] != '\0', );
	CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0', );
	CARLA_SAFE_ASSERT_RETURN(value != nullptr, );

	/* Check if we already have this key */
	bool found = false;
	for (CustomData &cdata : customData) {
		if (std::strcmp(cdata.key, key) == 0) {
			bfree(const_cast<char *>(cdata.value));
			cdata.value = bstrdup(value);
			found = true;
			break;
		}
	}

	/* Otherwise store it */
	if (!found) {
		CustomData cdata = {};
		cdata.type = bstrdup(type);
		cdata.key = bstrdup(key);
		cdata.value = bstrdup(value);
		customData.push_back(cdata);
	}

	if (sendToPlugin) {
		const uint32_t maxLocalValueLen =
			clientBridgeVersion >= 10 ? 4096 : 16384;

		const uint32_t typeLen =
			static_cast<uint32_t>(std::strlen(type));
		const uint32_t keyLen = static_cast<uint32_t>(std::strlen(key));
		const uint32_t valueLen =
			static_cast<uint32_t>(std::strlen(value));

		const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

		if (valueLen > maxLocalValueLen)
			nonRtClientCtrl.waitIfDataIsReachingLimit();

		nonRtClientCtrl.writeOpcode(
			kPluginBridgeNonRtClientSetCustomData);

		nonRtClientCtrl.writeUInt(typeLen);
		nonRtClientCtrl.writeCustomData(type, typeLen);

		nonRtClientCtrl.writeUInt(keyLen);
		nonRtClientCtrl.writeCustomData(key, keyLen);

		nonRtClientCtrl.writeUInt(valueLen);

		if (valueLen > 0) {
			if (valueLen > maxLocalValueLen) {
				QString filePath(QDir::tempPath());

				filePath += CARLA_OS_SEP_STR
					".CarlaCustomData_";
				filePath += audiopool.getFilenameSuffix();

				QFile file(filePath);
				if (file.open(QIODevice::WriteOnly) &&
				    file.write(value) !=
					    static_cast<qint64>(valueLen)) {
					const uint32_t ulength =
						static_cast<uint32_t>(
							filePath.length());

					nonRtClientCtrl.writeUInt(ulength);
					nonRtClientCtrl.writeCustomData(
						filePath.toUtf8().constData(),
						ulength);
				} else {
					nonRtClientCtrl.writeUInt(0);
				}
			} else {
				nonRtClientCtrl.writeCustomData(value,
								valueLen);
			}
		}

		nonRtClientCtrl.commitWrite();

		nonRtClientCtrl.waitIfDataIsReachingLimit();
	}
}

void carla_bridge::custom_data_loaded()
{
	if (info.ptype != PLUGIN_LV2)
		return;

	const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

	nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientRestoreLV2State);
	nonRtClientCtrl.commitWrite();
}

void carla_bridge::clear_custom_data()
{
	for (CustomData &cdata : customData) {
		bfree(const_cast<char *>(cdata.type));
		bfree(const_cast<char *>(cdata.key));
		bfree(const_cast<char *>(cdata.value));
	}
	customData.clear();
}

void carla_bridge::load_chunk(const char *b64chunk)
{
	chunk = QByteArray::fromBase64(b64chunk);

	QString filePath(QDir::tempPath());

	filePath += CARLA_OS_SEP_STR ".CarlaChunk_";
	filePath += audiopool.getFilenameSuffix();

	QFile file(filePath);
	if (file.open(QIODevice::WriteOnly) && file.write(b64chunk) != 0) {
		file.close();

		const uint32_t ulength =
			static_cast<uint32_t>(filePath.length());

		const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

		nonRtClientCtrl.writeOpcode(
			kPluginBridgeNonRtClientSetChunkDataFile);
		nonRtClientCtrl.writeUInt(ulength);
		nonRtClientCtrl.writeCustomData(filePath.toUtf8().constData(),
						ulength);
		nonRtClientCtrl.commitWrite();

		nonRtClientCtrl.waitIfDataIsReachingLimit();
	}
}

void carla_bridge::save_and_wait()
{
	if (!is_running())
		return;

	saved = false;
	pendingPing = false;

	{
		const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

		/* deactivate bridge client-side ping check
		 * some plugins block during save, preventing regular ping timings
		 */
		nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientPingOnOff);
		nonRtClientCtrl.writeBool(false);
		nonRtClientCtrl.commitWrite();

		/* tell plugin bridge to save and report any pending data */
		nonRtClientCtrl.writeOpcode(
			kPluginBridgeNonRtClientPrepareForSave);
		nonRtClientCtrl.commitWrite();
	}

	/* wait for "saved" reply */
	const uint64_t start_time = os_gettime_ns();

	while (is_running() && !saved) {
		os_sleep_ms(5);

		/* timeout after 10s */
		if (os_gettime_ns() - start_time > 10 * 1000000000ULL)
			break;

		readMessages();

		/* deactivate plugin if we timeout during save */
		if (timedOut && activated) {
			activated = false;

			const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

			nonRtClientCtrl.writeOpcode(
				kPluginBridgeNonRtClientDeactivate);
			nonRtClientCtrl.commitWrite();
		}
	}

	if (is_running()) {
		const CarlaMutexLocker cml(nonRtClientCtrl.mutex);

		/* reactivate ping check */
		nonRtClientCtrl.writeOpcode(kPluginBridgeNonRtClientPingOnOff);
		nonRtClientCtrl.writeBool(true);
		nonRtClientCtrl.commitWrite();
	}
}

void carla_bridge::set_buffer_size(const uint32_t maxBufferSize)
{
	if (bufferSize == maxBufferSize)
		return;

	bufferSize = maxBufferSize;

	if (is_running()) {
		audiopool.resize(maxBufferSize, MAX_AV_PLANES, MAX_AV_PLANES);

		rtClientCtrl.writeOpcode(kPluginBridgeRtClientSetAudioPool);
		rtClientCtrl.writeULong(
			static_cast<uint64_t>(audiopool.dataSize));
		rtClientCtrl.commitWrite();

		rtClientCtrl.writeOpcode(kPluginBridgeRtClientSetBufferSize);
		rtClientCtrl.writeUInt(maxBufferSize);
		rtClientCtrl.commitWrite();
	}
}

const char *carla_bridge::get_last_error() const noexcept
{
	return lastError;
}

void carla_bridge::readMessages()
{
	while (nonRtServerCtrl.isDataAvailableForReading()) {
		const PluginBridgeNonRtServerOpcode opcode =
			nonRtServerCtrl.readOpcode();

		/* #ifdef DEBUG */
		if (opcode != kPluginBridgeNonRtServerPong &&
		    opcode != kPluginBridgeNonRtServerParameterValue2) {
			blog(LOG_DEBUG, "[carla] got opcode: %s",
			     PluginBridgeNonRtServerOpcode2str(opcode));
		}
		/* #endif */

		switch (opcode) {
		case kPluginBridgeNonRtServerNull:
			break;

		case kPluginBridgeNonRtServerPong:
			pendingPing = false;
			break;

		/* uint/version */
		case kPluginBridgeNonRtServerVersion:
			clientBridgeVersion = nonRtServerCtrl.readUInt();
			break;

		/* uint/category
		 * uint/hints
		 * uint/optionsAvailable
		 * uint/optionsEnabled
		 * long/uniqueId
		 */
		case kPluginBridgeNonRtServerPluginInfo1: {
			nonRtServerCtrl.readUInt(); /* category */
			info.hints = nonRtServerCtrl.readUInt() |
				     PLUGIN_IS_BRIDGE;
			nonRtServerCtrl.readUInt(); /* optionsAvailable */
			info.options = nonRtServerCtrl.readUInt();
			const int64_t uniqueId = nonRtServerCtrl.readLong();

			if (info.uniqueId != 0) {
				CARLA_SAFE_ASSERT_INT2(info.uniqueId ==
							       uniqueId,
						       info.uniqueId, uniqueId);
			}
		} break;

		/* uint/size, str[] (realName)
		 * uint/size, str[] (label)
		 * uint/size, str[] (maker)
		 * uint/size, str[] (copyright)
		 */
		case kPluginBridgeNonRtServerPluginInfo2: {
			/* realName */
			const BridgeTextReader name(nonRtServerCtrl);
			info.name = name.text;

			/* label */
			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);

			/* maker */
			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);

			/* copyright */
			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);
		} break;

		/* uint/ins
		 * uint/outs
		 */
		case kPluginBridgeNonRtServerAudioCount:
			info.numAudioIns = nonRtServerCtrl.readUInt();
			info.numAudioOuts = nonRtServerCtrl.readUInt();
			break;

		/* uint/ins
		 * uint/outs
		 */
		case kPluginBridgeNonRtServerMidiCount:
			nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readUInt();
			break;

		/* uint/ins
		 * uint/outs
		 */
		case kPluginBridgeNonRtServerCvCount: {
			const uint32_t cvIns = nonRtServerCtrl.readUInt();
			const uint32_t cvOuts = nonRtServerCtrl.readUInt();
			info.hasCV = cvIns + cvOuts != 0;
		} break;

		/* uint/count */
		case kPluginBridgeNonRtServerParameterCount: {
			paramCount = nonRtServerCtrl.readUInt();

			delete[] paramDetails;

			if (paramCount != 0)
				paramDetails = new carla_param_data[paramCount];
			else
				paramDetails = nullptr;
		} break;

		/* uint/count */
		case kPluginBridgeNonRtServerProgramCount:
			nonRtServerCtrl.readUInt();
			break;

		/* uint/count */
		case kPluginBridgeNonRtServerMidiProgramCount:
			nonRtServerCtrl.readUInt();
			break;

		/* byte/type
		 * uint/index
		 * uint/size, str[] (name)
		 */
		case kPluginBridgeNonRtServerPortName: {
			nonRtServerCtrl.readByte();
			nonRtServerCtrl.readUInt();

			/* name */
			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);

		} break;

		/* uint/index
		 * int/rindex
		 * uint/type
		 * uint/hints
		 * short/cc
		 */
		case kPluginBridgeNonRtServerParameterData1: {
			const uint32_t index = nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readInt();
			const uint32_t type = nonRtServerCtrl.readUInt();
			const uint32_t hints = nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readShort();

			CARLA_SAFE_ASSERT_UINT2_BREAK(index < paramCount, index,
						      paramCount);

			if (type != PARAMETER_INPUT)
				break;
			if ((hints & PARAMETER_IS_ENABLED) == 0)
				break;
			if (hints &
			    (PARAMETER_IS_READ_ONLY | PARAMETER_IS_NOT_SAVED))
				break;

			paramDetails[index].hints = hints;
		} break;

		/* uint/index
		 * uint/size, str[] (name)
		 * uint/size, str[] (symbol)
		 * uint/size, str[] (unit)
		 */
		case kPluginBridgeNonRtServerParameterData2: {
			const uint32_t index = nonRtServerCtrl.readUInt();

			/* name */
			const BridgeTextReader name(nonRtServerCtrl);

			/* symbol */
			const BridgeTextReader symbol(nonRtServerCtrl);

			/* unit */
			const BridgeTextReader unit(nonRtServerCtrl);

			CARLA_SAFE_ASSERT_UINT2_BREAK(index < paramCount, index,
						      paramCount);

			if (paramDetails[index].hints & PARAMETER_IS_ENABLED) {
				paramDetails[index].name = name.text;
				paramDetails[index].symbol = symbol.text;
				paramDetails[index].unit = unit.text;
			}
		} break;

		/* uint/index
		 * float/def
		 * float/min
		 * float/max
		 * float/step
		 * float/stepSmall
		 * float/stepLarge
		 */
		case kPluginBridgeNonRtServerParameterRanges: {
			const uint32_t index = nonRtServerCtrl.readUInt();
			const float def = nonRtServerCtrl.readFloat();
			const float min = nonRtServerCtrl.readFloat();
			const float max = nonRtServerCtrl.readFloat();
			const float step = nonRtServerCtrl.readFloat();
			nonRtServerCtrl.readFloat();
			nonRtServerCtrl.readFloat();

			CARLA_SAFE_ASSERT_BREAK(min < max);
			CARLA_SAFE_ASSERT_BREAK(def >= min);
			CARLA_SAFE_ASSERT_BREAK(def <= max);
			CARLA_SAFE_ASSERT_UINT2_BREAK(index < paramCount, index,
						      paramCount);

			if (paramDetails[index].hints & PARAMETER_IS_ENABLED) {
				paramDetails[index].def =
					paramDetails[index].value = def;
				paramDetails[index].min = min;
				paramDetails[index].max = max;
				paramDetails[index].step = step;
			}
		} break;

		/* uint/index
		 * float/value
		 */
		case kPluginBridgeNonRtServerParameterValue: {
			const uint32_t index = nonRtServerCtrl.readUInt();
			const float value = nonRtServerCtrl.readFloat();

			if (index < paramCount) {
				const float fixedValue = carla_fixedValue(
					paramDetails[index].min,
					paramDetails[index].max, value);

				if (carla_isNotEqual(paramDetails[index].value,
						     fixedValue)) {
					paramDetails[index].value = fixedValue;

					if (callback != nullptr) {
						/* skip parameters that we do not show */
						if ((paramDetails[index].hints &
						     PARAMETER_IS_ENABLED) == 0)
							break;

						callback->bridge_parameter_changed(
							index, fixedValue);
					}
				}
			}
		} break;

		/* uint/index
		 * float/value
		 */
		case kPluginBridgeNonRtServerParameterValue2: {
			const uint32_t index = nonRtServerCtrl.readUInt();
			const float value = nonRtServerCtrl.readFloat();

			if (index < paramCount) {
				const float fixedValue = carla_fixedValue(
					paramDetails[index].min,
					paramDetails[index].max, value);
				paramDetails[index].value = fixedValue;
			}
		} break;

		/* uint/index
		 * bool/touch
		 */
		case kPluginBridgeNonRtServerParameterTouch:
			nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readBool();
			break;

		/* uint/index
		 * float/value
		 */
		case kPluginBridgeNonRtServerDefaultValue: {
			const uint32_t index = nonRtServerCtrl.readUInt();
			const float value = nonRtServerCtrl.readFloat();

			if (index < paramCount)
				paramDetails[index].def = value;
		} break;

		/* int/index */
		case kPluginBridgeNonRtServerCurrentProgram:
			nonRtServerCtrl.readInt();
			break;

		/* int/index */
		case kPluginBridgeNonRtServerCurrentMidiProgram:
			nonRtServerCtrl.readInt();
			break;

		/* uint/index
		 * uint/size, str[] (name)
		 */
		case kPluginBridgeNonRtServerProgramName: {
			nonRtServerCtrl.readUInt();

			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);
		} break;

		/* uint/index
		 * uint/bank
		 * uint/program
		 * uint/size, str[] (name)
		 */
		case kPluginBridgeNonRtServerMidiProgramData: {
			nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readUInt();

			/* name */
			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);
		} break;

		/* uint/size, str[]
		 * uint/size, str[]
		 * uint/size, str[]
		 */
		case kPluginBridgeNonRtServerSetCustomData: {
			const uint32_t maxLocalValueLen =
				clientBridgeVersion >= 10 ? 4096 : 16384;

			/* type */
			const BridgeTextReader type(nonRtServerCtrl);

			/* key */
			const BridgeTextReader key(nonRtServerCtrl);

			/* value */
			const uint32_t valueSize = nonRtServerCtrl.readUInt();

			/* special case for big values */
			if (valueSize > maxLocalValueLen) {
				const BridgeTextReader bigValueFilePath(
					nonRtServerCtrl, valueSize);

				QString realBigValueFilePath(QString::fromUtf8(
					bigValueFilePath.text));

				QFile bigValueFile(realBigValueFilePath);
				CARLA_SAFE_ASSERT_BREAK(bigValueFile.exists());

				if (bigValueFile.open(QIODevice::ReadOnly)) {
					add_custom_data(type.text, key.text,
							bigValueFile.readAll()
								.constData(),
							false);
					bigValueFile.remove();
				}
			} else {
				const BridgeTextReader value(nonRtServerCtrl,
							     valueSize);

				add_custom_data(type.text, key.text, value.text,
						false);
			}

		} break;

		/* uint/size, str[] (filename, base64 content) */
		case kPluginBridgeNonRtServerSetChunkDataFile: {
			const BridgeTextReader chunkFilePath(nonRtServerCtrl);

			QString realChunkFilePath(
				QString::fromUtf8(chunkFilePath.text));

			QFile chunkFile(realChunkFilePath);
			CARLA_SAFE_ASSERT_BREAK(chunkFile.exists());

			if (chunkFile.open(QIODevice::ReadOnly)) {
				chunk = QByteArray::fromBase64(
					chunkFile.readAll());
				chunkFile.remove();
			}
		} break;

		/* uint/latency */
		case kPluginBridgeNonRtServerSetLatency:
			nonRtServerCtrl.readUInt();
			break;

		/* uint/index
		 * uint/size, str[] (name)
		 */
		case kPluginBridgeNonRtServerSetParameterText: {
			nonRtServerCtrl.readInt();

			if (const uint32_t size = nonRtServerCtrl.readUInt())
				nonRtServerCtrl.skipRead(size);
		} break;

		case kPluginBridgeNonRtServerReady:
			ready = true;
			break;

		case kPluginBridgeNonRtServerSaved:
			saved = true;
			break;

		/* ulong/window-id */
		case kPluginBridgeNonRtServerRespEmbedUI:
			nonRtServerCtrl.readULong();
			break;

		/* uint/width
		 * uint/height
		 */
		case kPluginBridgeNonRtServerResizeEmbedUI:
			nonRtServerCtrl.readUInt();
			nonRtServerCtrl.readUInt();
			break;

		case kPluginBridgeNonRtServerUiClosed:
			break;

		/* uint/size, str[] */
		case kPluginBridgeNonRtServerError: {
			const BridgeTextReader error(nonRtServerCtrl);
			timedErr = true;
			blog(LOG_ERROR, "[carla] %s", error.text);
			setLastError(error.text);
		} break;
		}
	}
}

void carla_bridge::setLastError(const char *const error)
{
	bfree(lastError);
	lastError = bstrdup(error);
}
