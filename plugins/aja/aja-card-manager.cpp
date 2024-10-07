#include "aja-card-manager.hpp"
#include "aja-common.hpp"
#include "aja-output.hpp"
#include "aja-source.hpp"

#include "obs-properties.h"

#include <util/base.h>

#include <ajantv2/includes/ntv2card.h>
#include <ajantv2/includes/ntv2devicescanner.h>
#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2utils.h>

#include <ajabase/system/process.h>
#include <ajabase/system/systemtime.h>

static const uint32_t kStreamingAppID = NTV2_FOURCC('O', 'B', 'S', ' ');

namespace aja {

CardManager &CardManager::Instance()
{
	static CardManager instance;
	return instance;
}

CardEntry::CardEntry(uint32_t cardIndex, const std::string &cardID)
	: mCardIndex{cardIndex},
	  mCardID{cardID},
	  mCard{std::make_unique<CNTV2Card>((UWord)cardIndex)},
	  mChannelPwnz{},
	  mMutex{}
{
}

CardEntry::~CardEntry()
{
	if (mCard) {
		mCard->Close();
		mCard.reset();
	}
}

CNTV2Card *CardEntry::GetCard()
{
	return mCard.get();
}

bool CardEntry::Initialize()
{
	if (!mCard) {
		blog(LOG_ERROR, "Invalid card instance %s!", mCardID.c_str());
		return false;
	}

	const NTV2DeviceID deviceID = mCard->GetDeviceID();

	// Briefly enter Standard Tasks mode to reset card via AJA Daemon/Agent.
	auto taskMode = NTV2_STANDARD_TASKS;
	mCard->GetEveryFrameServices(taskMode);
	if (taskMode != NTV2_STANDARD_TASKS) {
		mCard->SetEveryFrameServices(NTV2_STANDARD_TASKS);
		AJATime::Sleep(100);
	}

	mCard->SetEveryFrameServices(NTV2_OEM_TASKS);

	const int32_t obsPid = (int32_t)AJAProcess::GetPid();
	mCard->AcquireStreamForApplicationWithReference((ULWord)kStreamingAppID,
							obsPid);

	mCard->SetSuspendHostAudio(true);

	mCard->ClearRouting();

	if (NTV2DeviceCanDoMultiFormat(deviceID)) {
		mCard->SetMultiFormatMode(true);
	}

	mCard->SetReference(NTV2_REFERENCE_FREERUN);

	for (UWord i = 0; i < aja::CardNumAudioSystems(deviceID); i++) {
		mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF,
					static_cast<NTV2AudioSystem>(i));
	}

	auto numFramestores = aja::CardNumFramestores(deviceID);

	for (UWord i = 0; i < NTV2DeviceGetNumVideoInputs(deviceID); i++) {
		mCard->SetInputFrame(static_cast<NTV2Channel>(i), 0xff);
		// Disable 3G Level B converter by default
		if (NTV2DeviceCanDo3GLevelConversion(deviceID)) {
			mCard->SetSDIInLevelBtoLevelAConversion(
				static_cast<NTV2Channel>(i), false);
		}
	}

	// SDI Outputs Default State
	for (UWord i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++) {
		auto channel = GetNTV2ChannelForIndex(i);
		if (NTV2DeviceCanDo3GOut(deviceID, i)) {
			mCard->SetSDIOut3GEnable(channel, true);
			mCard->SetSDIOut3GbEnable(channel, false);
		}
		if (NTV2DeviceCanDo12GOut(deviceID, i)) {
			mCard->SetSDIOut6GEnable(channel, false);
			mCard->SetSDIOut12GEnable(channel, false);
		}
		if (NTV2DeviceCanDo3GLevelConversion(deviceID)) {
			mCard->SetSDIOutLevelAtoLevelBConversion(i, false);
			mCard->SetSDIOutRGBLevelAConversion(i, false);
		}
	}

	for (UWord i = 0; i < numFramestores; i++) {
		auto channel = GetNTV2ChannelForIndex(i);

		if (isAutoCirculateRunning(channel)) {
			mCard->AutoCirculateStop(channel, true);
		}

		mCard->SetVideoFormat(NTV2_FORMAT_1080p_5994_A, false, false,
				      channel);
		mCard->SetFrameBufferFormat(channel, NTV2_FBF_8BIT_YCBCR);

		mCard->DisableChannel(channel);
	}

	blog(LOG_DEBUG, "NTV2 Card Initialized: %s", mCardID.c_str());

	return true;
}

uint32_t CardEntry::GetCardIndex() const
{
	return mCardIndex;
}

std::string CardEntry::GetCardID() const
{
	return mCardID;
}

std::string CardEntry::GetDisplayName() const
{
	if (mCard) {
		std::ostringstream oss;
		oss << mCard->GetIndexNumber() << " - "
		    << mCard->GetModelName();
		const std::string &serial = GetSerial();
		if (!serial.empty())
			oss << " (" << serial << ")";

		return oss.str();
	}

	// very bad if we get here...
	return "Unknown";
}

std::string CardEntry::GetSerial() const
{
	std::string serial;
	if (mCard)
		mCard->GetSerialNumberString(serial);

	return serial;
}

NTV2DeviceID CardEntry::GetDeviceID() const
{
	NTV2DeviceID id = DEVICE_ID_NOTFOUND;
	if (mCard)
		id = mCard->GetDeviceID();

	return id;
}

bool CardEntry::ChannelReady(NTV2Channel chan, const std::string &owner) const
{
	const std::lock_guard<std::mutex> lock(mMutex);
	for (const auto &pwn : mChannelPwnz) {
		if (pwn.second & (1 << static_cast<int32_t>(chan))) {
			return pwn.first == owner;
		}
	}

	return true;
}

bool CardEntry::AcquireChannel(NTV2Channel chan, NTV2Mode mode,
			       const std::string &owner)
{
	bool acquired = false;

	if (ChannelReady(chan, owner)) {
		const std::lock_guard<std::mutex> lock(mMutex);
		if (mChannelPwnz.find(owner) != mChannelPwnz.end()) {
			if (mChannelPwnz[owner] &
			    (1 << static_cast<int32_t>(chan))) {
				acquired = true;
			} else {
				mChannelPwnz[owner] |=
					(1 << static_cast<int32_t>(chan));
				acquired = true;
			}
		} else {
			mChannelPwnz[owner] |=
				(1 << static_cast<int32_t>(chan));
			acquired = true;
		}

		// Acquire interrupt handles
		if (acquired && mCard) {
			if (mode == NTV2_MODE_CAPTURE) {
				mCard->EnableInputInterrupt(chan);
				mCard->SubscribeInputVerticalEvent(chan);
			} else if (mode == NTV2_MODE_DISPLAY) {
				mCard->EnableOutputInterrupt(chan);
				mCard->SubscribeOutputVerticalEvent(chan);
			}
		}
	}

	return acquired;
}

bool CardEntry::ReleaseChannel(NTV2Channel chan, NTV2Mode mode,
			       const std::string &owner)
{
	const std::lock_guard<std::mutex> lock(mMutex);
	for (const auto &pwn : mChannelPwnz) {
		if (pwn.first == owner) {
			if (mChannelPwnz[owner] &
			    (1 << static_cast<int32_t>(chan))) {
				mChannelPwnz[owner] ^=
					(1 << static_cast<int32_t>(chan));

				// Release interrupt handles
				if (mCard) {
					if (mode == NTV2_MODE_CAPTURE) {
						mCard->DisableInputInterrupt(
							chan);
						mCard->UnsubscribeInputVerticalEvent(
							chan);
					} else if (mode == NTV2_MODE_DISPLAY) {
						mCard->DisableOutputInterrupt(
							chan);
						mCard->UnsubscribeOutputVerticalEvent(
							chan);
					}
				}

				return true;
			}
		}
	}

	return false;
}

bool CardEntry::InputSelectionReady(IOSelection io, NTV2DeviceID id,
				    const std::string &owner) const
{
	if (id == DEVICE_ID_KONA1 && io == IOSelection::SDI1) {
		return true;
	} else {
		NTV2InputSourceSet inputSources;
		aja::IOSelectionToInputSources(io, inputSources);
		if (inputSources.size() > 0) {
			size_t channelsReady = 0;
			for (auto &&src : inputSources) {
				auto channel = NTV2InputSourceToChannel(src);
				if (ChannelReady(channel, owner))
					channelsReady++;
			}
			if (channelsReady == inputSources.size())
				return true;
		}
	}

	return false;
}

bool CardEntry::OutputSelectionReady(IOSelection io, NTV2DeviceID id,
				     const std::string &owner) const
{
	/* Handle checking special case outputs before all other outputs.
	 * 1. HDMI Monitor uses framestore 4
	 * 2. SDI Monitor on Io 4K/Io 4K Plus, etc. uses framestore 4.
	 * 3. Everything else...
	 */
	if (aja::CardCanDoHDMIMonitorOutput(id) &&
	    io == IOSelection::HDMIMonitorOut) {
		NTV2Channel hdmiMonChannel = NTV2_CHANNEL4;
		return ChannelReady(hdmiMonChannel, owner);
	} else if (aja::CardCanDoSDIMonitorOutput(id) &&
		   io == IOSelection::SDI5) {
		NTV2Channel sdiMonChannel = NTV2_CHANNEL4;
		return ChannelReady(sdiMonChannel, owner);
	} else if (id == DEVICE_ID_KONA1 && io == IOSelection::SDI1) {
		return true;
	} else {
		NTV2OutputDestinations outputDests;
		aja::IOSelectionToOutputDests(io, outputDests);
		if (outputDests.size() > 0) {
			size_t channelsReady = 0;
			for (auto &&dst : outputDests) {
				auto channel =
					NTV2OutputDestinationToChannel(dst);
				if (ChannelReady(channel, owner))
					channelsReady++;
			}
			if (channelsReady == outputDests.size())
				return true;
		}
	}

	return false;
}

bool CardEntry::AcquireInputSelection(IOSelection io, NTV2DeviceID id,
				      const std::string &owner)
{
	UNUSED_PARAMETER(id);

	NTV2InputSourceSet inputSources;
	aja::IOSelectionToInputSources(io, inputSources);

	std::vector<NTV2Channel> acquiredChannels;
	for (auto &&src : inputSources) {
		auto acqChan = NTV2InputSourceToChannel(src);
		if (AcquireChannel(acqChan, NTV2_MODE_CAPTURE, owner)) {
			blog(LOG_DEBUG, "Source %s acquired channel %s",
			     owner.c_str(),
			     NTV2ChannelToString(acqChan).c_str());
			acquiredChannels.push_back(acqChan);
		} else {
			blog(LOG_DEBUG,
			     "Source %s could not acquire channel %s",
			     owner.c_str(),
			     NTV2ChannelToString(acqChan).c_str());
		}
	}

	// Release channels if we couldn't acquire all required channels.
	if (acquiredChannels.size() != inputSources.size()) {
		for (auto &&ac : acquiredChannels) {
			ReleaseChannel(ac, NTV2_MODE_CAPTURE, owner);
		}
	}

	return acquiredChannels.size() == inputSources.size();
}

bool CardEntry::ReleaseInputSelection(IOSelection io, NTV2DeviceID id,
				      const std::string &owner)
{
	UNUSED_PARAMETER(id);

	NTV2InputSourceSet currentInputSources;
	aja::IOSelectionToInputSources(io, currentInputSources);
	uint32_t releasedCount = 0;
	for (auto &&src : currentInputSources) {
		auto relChan = NTV2InputSourceToChannel(src);
		if (ReleaseChannel(relChan, NTV2_MODE_CAPTURE, owner)) {
			blog(LOG_DEBUG, "Released Channel %s",
			     NTV2ChannelToString(relChan).c_str());
			releasedCount++;
		}
	}
	return releasedCount == currentInputSources.size();
}

bool CardEntry::AcquireOutputSelection(IOSelection io, NTV2DeviceID id,
				       const std::string &owner)
{
	std::vector<NTV2Channel> acquiredChannels;
	NTV2OutputDestinations outputDests;
	aja::IOSelectionToOutputDests(io, outputDests);

	// Handle acquiring special case outputs --
	// HDMI Monitor uses framestore 4
	if (aja::CardCanDoHDMIMonitorOutput(id) &&
	    io == IOSelection::HDMIMonitorOut) {
		NTV2Channel hdmiMonChannel = NTV2_CHANNEL4;
		if (AcquireChannel(hdmiMonChannel, NTV2_MODE_DISPLAY, owner)) {
			blog(LOG_DEBUG, "Output %s acquired channel %s",
			     owner.c_str(),
			     NTV2ChannelToString(hdmiMonChannel).c_str());
			acquiredChannels.push_back(hdmiMonChannel);
		} else {
			blog(LOG_DEBUG,
			     "Output %s could not acquire channel %s",
			     owner.c_str(),
			     NTV2ChannelToString(hdmiMonChannel).c_str());
		}
	} else if (aja::CardCanDoSDIMonitorOutput(id) &&
		   io == IOSelection::SDI5) {
		// SDI Monitor on io4K/io4K+/etc. uses framestore 4
		NTV2Channel sdiMonChannel = NTV2_CHANNEL4;
		if (AcquireChannel(sdiMonChannel, NTV2_MODE_DISPLAY, owner)) {
			blog(LOG_DEBUG, "Output %s acquired channel %s",
			     owner.c_str(),
			     NTV2ChannelToString(sdiMonChannel).c_str());
			acquiredChannels.push_back(sdiMonChannel);
		} else {
			blog(LOG_DEBUG,
			     "Output %s could not acquire channel %s",
			     owner.c_str(),
			     NTV2ChannelToString(sdiMonChannel).c_str());
		}
	} else {
		// Handle acquiring all other channels
		for (auto &&dst : outputDests) {
			auto acqChan = NTV2OutputDestinationToChannel(dst);
			if (AcquireChannel(acqChan, NTV2_MODE_DISPLAY, owner)) {
				acquiredChannels.push_back(acqChan);
				blog(LOG_DEBUG, "Output %s acquired channel %s",
				     owner.c_str(),
				     NTV2ChannelToString(acqChan).c_str());
			} else {
				blog(LOG_DEBUG,
				     "Output %s could not acquire channel %s",
				     owner.c_str(),
				     NTV2ChannelToString(acqChan).c_str());
			}
		}

		// Release channels if we couldn't acquire all required channels.
		if (acquiredChannels.size() != outputDests.size()) {
			for (auto &&ac : acquiredChannels) {
				ReleaseChannel(ac, NTV2_MODE_DISPLAY, owner);
			}
		}
	}

	return acquiredChannels.size() == outputDests.size();
}

bool CardEntry::ReleaseOutputSelection(IOSelection io, NTV2DeviceID id,
				       const std::string &owner)
{
	NTV2OutputDestinations currentOutputDests;
	aja::IOSelectionToOutputDests(io, currentOutputDests);
	uint32_t releasedCount = 0;

	// Handle releasing special case outputs --
	// HDMI Monitor uses framestore 4
	if (aja::CardCanDoHDMIMonitorOutput(id) &&
	    io == IOSelection::HDMIMonitorOut) {
		NTV2Channel hdmiMonChannel = NTV2_CHANNEL4;
		if (ReleaseChannel(hdmiMonChannel, NTV2_MODE_DISPLAY, owner)) {
			blog(LOG_DEBUG, "Released Channel %s",
			     NTV2ChannelToString(hdmiMonChannel).c_str());
			releasedCount++;
		}
	} else if (aja::CardCanDoSDIMonitorOutput(id) &&
		   io == IOSelection::SDI5) {
		// SDI Monitor on io4K/io4K+/etc. uses framestore 4
		NTV2Channel sdiMonChannel = NTV2_CHANNEL4;
		if (ReleaseChannel(sdiMonChannel, NTV2_MODE_DISPLAY, owner)) {
			blog(LOG_DEBUG, "Released Channel %s",
			     NTV2ChannelToString(sdiMonChannel).c_str());
			releasedCount++;
		}
	} else {
		// Release all other channels
		for (auto &&dst : currentOutputDests) {
			auto relChan = NTV2OutputDestinationToChannel(dst);
			if (ReleaseChannel(relChan, NTV2_MODE_DISPLAY, owner)) {
				blog(LOG_DEBUG, "Released Channel %s",
				     NTV2ChannelToString(relChan).c_str());
				releasedCount++;
			}
		}
	}
	return releasedCount == currentOutputDests.size();
}

bool CardEntry::UpdateChannelOwnerName(const std::string &oldName,
				       const std::string &newName)
{
	const std::lock_guard<std::mutex> lock(mMutex);
	for (const auto &pwn : mChannelPwnz) {
		if (pwn.first == oldName) {
			mChannelPwnz.insert(std::pair<std::string, int32_t>{
				newName, pwn.second});

			mChannelPwnz.erase(oldName);

			return true;
		}
	}

	return false;
}

bool CardEntry::isAutoCirculateRunning(NTV2Channel chan)
{
	if (!mCard)
		return false;

	AUTOCIRCULATE_STATUS acStatus;
	if (mCard->AutoCirculateGetStatus(chan, acStatus)) {
		if (acStatus.acState != NTV2_AUTOCIRCULATE_RUNNING &&
		    acStatus.acState != NTV2_AUTOCIRCULATE_STARTING &&
		    acStatus.acState != NTV2_AUTOCIRCULATE_PAUSED &&
		    acStatus.acState != NTV2_AUTOCIRCULATE_INIT) {
			return false;
		}
	}

	return true;
}

void CardManager::ClearCardEntries()
{
	const std::lock_guard<std::mutex> lock(mMutex);
	for (auto &&entry : mCardEntries) {
		CNTV2Card *card = entry.second->GetCard();
		if (card) {
			card->SetSuspendHostAudio(false);
			card->SetEveryFrameServices(NTV2_STANDARD_TASKS);
			// Workaround for AJA internal bug #11378
			// Set HDMI output back to Audio System 1 on card release
			if (NTV2DeviceGetNumHDMIVideoOutputs(
				    card->GetDeviceID()) > 0) {
				card->SetHDMIOutAudioSource8Channel(
					NTV2_AudioChannel1_8,
					NTV2_AUDIOSYSTEM_1);
			}

			int32_t pid = (int32_t)AJAProcess::GetPid();
			card->ReleaseStreamForApplicationWithReference(
				(ULWord)kStreamingAppID, pid);
		}
	}

	mCardEntries.clear();
}

void CardManager::EnumerateCards()
{
	const std::lock_guard<std::mutex> lock(mMutex);
	CNTV2DeviceScanner scanner;
	for (const auto &iter : scanner.GetDeviceInfoList()) {
		CNTV2Card card((UWord)iter.deviceIndex);
		const std::string &cardID = aja::MakeCardID(card);

		// New Card Entry
		if (mCardEntries.find(cardID) == mCardEntries.end()) {
			CardEntryPtr cardEntry = std::make_shared<CardEntry>(
				iter.deviceIndex, cardID);
			if (cardEntry && cardEntry->Initialize())
				mCardEntries.emplace(cardID, cardEntry);
		} else {
			// Card fell off of the bus and came back with a new physical index?
			auto currEntry = mCardEntries[cardID];
			if (currEntry) {
				if (currEntry->GetCardIndex() !=
				    iter.deviceIndex) {
					mCardEntries.erase(cardID);
					CardEntryPtr cardEntry =
						std::make_shared<CardEntry>(
							iter.deviceIndex,
							cardID);
					if (cardEntry &&
					    cardEntry->Initialize())
						mCardEntries.emplace(cardID,
								     cardEntry);
				}
			}
		}
	}
}

size_t CardManager::NumCardEntries() const
{
	const std::lock_guard<std::mutex> lock(mMutex);
	return mCardEntries.size();
}

CNTV2Card *CardManager::GetCard(const std::string &cardID)
{
	auto entry = GetCardEntry(cardID);
	if (entry)
		return entry->GetCard();
	return nullptr;
}

const CardEntryPtr CardManager::GetCardEntry(const std::string &cardID) const
{
	const std::lock_guard<std::mutex> lock(mMutex);
	for (const auto &entry : mCardEntries) {
		if (entry.second && entry.second->GetCardID() == cardID)
			return entry.second;
	}
	return nullptr;
}

const CardEntries &CardManager::GetCardEntries() const
{
	const std::lock_guard<std::mutex> lock(mMutex);
	return mCardEntries;
}

const CardEntries::iterator CardManager::begin()
{
	const std::lock_guard<std::mutex> lock(mMutex);
	return mCardEntries.begin();
}

const CardEntries::iterator CardManager::end()
{
	const std::lock_guard<std::mutex> lock(mMutex);
	return mCardEntries.end();
}

} // namespace aja
