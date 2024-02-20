#pragma once

#include "aja-props.hpp"

#include <obs-module.h>

#include <ajantv2/includes/ntv2enums.h>
#include <ajantv2/includes/ntv2publicinterface.h>

#include <memory>
#include <map>
#include <mutex>
#include <vector>

class CNTV2Card;
class AJAOutput;
class AJASource;

namespace aja {

using ChannelPwnz = std::map<std::string, int32_t>;

/* A CardEntry for each physical AJA card is added to a map retained by the CardManager.
 * Each CardEntry itself maintains a map representing the AJA card "Channels" the are
 * owned by a particular capture or output plugin instance. The Channel ownership map is
 * then used to determine which "IOSelection" (i.e. SDI1, SDI3+4, HDMI Monitor Output, etc.)
 * drop-down menu items are either accessible or grayed out in the capture and output plugin UIs.
 */
class CardEntry {
public:
	CardEntry(uint32_t cardIndex, const std::string &cardID);
	virtual ~CardEntry();
	CNTV2Card *GetCard();
	virtual bool Initialize();
	virtual uint32_t GetCardIndex() const;
	virtual std::string GetCardID() const;
	virtual std::string GetDisplayName() const;
	virtual std::string GetSerial() const;
	virtual NTV2DeviceID GetDeviceID() const;
	virtual bool ChannelReady(NTV2Channel chan,
				  const std::string &owner) const;
	virtual bool AcquireChannel(NTV2Channel chan, NTV2Mode mode,
				    const std::string &owner);
	virtual bool ReleaseChannel(NTV2Channel chan, NTV2Mode mode,
				    const std::string &owner);
	virtual bool InputSelectionReady(IOSelection io, NTV2DeviceID id,
					 const std::string &owner) const;
	virtual bool OutputSelectionReady(IOSelection io, NTV2DeviceID id,
					  const std::string &owner) const;
	virtual bool AcquireInputSelection(IOSelection io, NTV2DeviceID id,
					   const std::string &owner);
	virtual bool ReleaseInputSelection(IOSelection io, NTV2DeviceID id,
					   const std::string &owner);
	virtual bool AcquireOutputSelection(IOSelection io, NTV2DeviceID id,
					    const std::string &owner);
	virtual bool ReleaseOutputSelection(IOSelection io, NTV2DeviceID id,
					    const std::string &owner);
	virtual bool UpdateChannelOwnerName(const std::string &oldName,
					    const std::string &newName);

private:
	virtual bool isAutoCirculateRunning(NTV2Channel);

protected:
	uint32_t mCardIndex;
	std::string mCardID;
	std::unique_ptr<CNTV2Card> mCard;
	ChannelPwnz mChannelPwnz;
	mutable std::mutex mMutex;
};
using CardEntryPtr = std::shared_ptr<CardEntry>;
using CardEntries = std::map<std::string, CardEntryPtr>;

/* The CardManager enumerates the physical AJA cards in the system, reverts them to a default
 * state on exit, and maintains a map of CardEntry objects corresponding to each physical card.
 * Each CardEntry object holds a pointer to the CNTV2Card instance and a map of NTV2Channels
 * that are "owned" by each plugin instance. NTV2Channels are essentially treated as indices
 * for various firwmare Widgets and sub-systems throughout the AJA NTV2 SDK.
 */
class CardManager {
public:
	static CardManager &Instance();

	void ClearCardEntries();
	void EnumerateCards();

	size_t NumCardEntries() const;
	CNTV2Card *GetCard(const std::string &cardID);
	const CardEntryPtr GetCardEntry(const std::string &cardID) const;
	const CardEntries &GetCardEntries() const;
	const CardEntries::iterator begin();
	const CardEntries::iterator end();

private:
	CardManager() = default;
	~CardManager() = default;
	CardManager(const CardManager &) = delete;
	CardManager(const CardManager &&) = delete;
	CardManager &operator=(const CardManager &) = delete;
	CardManager &operator=(const CardManager &&) = delete;

	CardEntries mCardEntries;
	mutable std::mutex mMutex;
};

} // namespace aja
