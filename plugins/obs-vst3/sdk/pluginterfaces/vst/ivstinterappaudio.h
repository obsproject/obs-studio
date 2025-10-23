//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstinterappaudio.h
// Created by  : Steinberg, 08/2013
// Description : VST InterAppAudio Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"

//------------------------------------------------------------------------
namespace Steinberg {
struct ViewRect;
namespace Vst {
struct Event;
class IInterAppAudioPresetManager;

//------------------------------------------------------------------------
/** Inter-App Audio host Interface.
\ingroup vstIHost vst360
- [host imp]
- [passed as 'context' to IPluginBase::initialize () ]
- [released: 3.6.0]
- [optional]

Implemented by the InterAppAudio Wrapper.
*/
class IInterAppAudioHost : public FUnknown
{
public:
	/** get the size of the screen
	 *	@param size size of the screen
	 *	@param scale scale of the screen
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API getScreenSize (ViewRect* size, float* scale) = 0;

	/** get status of connection
	 *	@return kResultTrue if an Inter-App Audio connection is established
	 */
	virtual tresult PLUGIN_API connectedToHost () = 0;

	/** switch to the host.
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API switchToHost () = 0;

	/** send a remote control event to the host
	 *	@param event event type, see AudioUnitRemoteControlEvent in the iOS SDK documentation for possible types
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API sendRemoteControlEvent (uint32 event) = 0;

	/** ask for the host icon.
	 *	@param icon pointer to a CGImageRef
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API getHostIcon (void** icon) = 0;

	/** schedule an event from the user interface thread
	 *	@param event the event to schedule
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API scheduleEventFromUI (Event& event) = 0;

	/** get the preset manager
	 *	@param cid class ID to use by the preset manager
	 *	@return the preset manager. Needs to be released by called.
	 */
	virtual IInterAppAudioPresetManager* PLUGIN_API createPresetManager (const TUID& cid) = 0;

	/** show the settings view
	 *	currently includes MIDI settings and Tempo setting
	 *	@return kResultTrue on success
	 */
	virtual tresult PLUGIN_API showSettingsView () = 0;

	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IInterAppAudioHost, 0x0CE5743D, 0x68DF415E, 0xAE285BD4, 0xE2CDC8FD)

//------------------------------------------------------------------------
/** Extended plug-in interface IEditController for Inter-App Audio connection state change notifications
\ingroup vstIPlug vst360
- [plug imp]
- [extends IEditController]
- [released: 3.6.0]
*/
class IInterAppAudioConnectionNotification : public FUnknown
{
public:
	/** called when the Inter-App Audio connection state changes
	 *	@param newState true if an Inter-App Audio connection is established, otherwise false
	*/
	virtual void PLUGIN_API onInterAppAudioConnectionStateChange (TBool newState) = 0;

	//------------------------------------------------------------------------	
	static const FUID iid;
};

DECLARE_CLASS_IID (IInterAppAudioConnectionNotification, 0x6020C72D, 0x5FC24AA1, 0xB0950DB5, 0xD7D6D5CF)

//------------------------------------------------------------------------
/** Extended plug-in interface IEditController for Inter-App Audio Preset Management
\ingroup vstIPlug vst360
- [plug imp]
- [extends IEditController]
- [released: 3.6.0]
*/
class IInterAppAudioPresetManager : public FUnknown
{
public:
	/** Open the Preset Browser in order to load a preset */
	virtual tresult PLUGIN_API runLoadPresetBrowser () = 0;
	/** Open the Preset Browser in order to save a preset */
	virtual tresult PLUGIN_API runSavePresetBrowser () = 0;
	/** Load the next available preset */
	virtual tresult PLUGIN_API loadNextPreset () = 0;
	/** Load the previous available preset */
	virtual tresult PLUGIN_API loadPreviousPreset () = 0;

	//------------------------------------------------------------------------	
	static const FUID iid;
};

DECLARE_CLASS_IID (IInterAppAudioPresetManager, 0xADE6FCC4, 0x46C94E1D, 0xB3B49A80, 0xC93FEFDD)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
