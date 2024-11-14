#include "aja-card-manager.hpp"
#include "aja-common.hpp"
#include "aja-routing.hpp"
#include "aja-widget-io.hpp"

#include <ajabase/common/common.h>
#include <ajantv2/includes/ntv2card.h>
#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2utils.h>

#include <obs-module.h>

namespace aja {
/*
 * Parse the widget routing shorthand string into a map of input and output NTV2CrosspointIDs.
 * For example "sdi[0][0]->fb[0][0]" is shorthand for connecting the output crosspoint for
 * SDI1/Datastream1 (NTV2_XptSDIIn1) to the input crosspoint for Framestore1/Datastream1 (NTV2_XptFrameBuffer1Input).
 * These routing shorthand strings are found within the RoutingConfig structs in the "routing" sub-directory of the plugin.
 */
bool Routing::ParseRouteString(const std::string &route,
			       NTV2XptConnections &cnx)
{
	blog(LOG_DEBUG, "aja::Routing::ParseRouteString: Input string: %s",
	     route.c_str());

	std::string route_strip(route);
	aja::lower(route_strip);
	aja::replace(route_strip, " ", "");

	if (route_strip.empty()) {
		blog(LOG_DEBUG,
		     "Routing::ParseRouteString: input string is empty!");
		return false;
	}

	/* TODO(paulh): Tally up the lines and tokens and check that they are all parsed OK.
	 * Right now we just return true if ANY tokens were parsed. This is OK _for now_ because
	 * the route strings currently only come from a known set.
	 */
	NTV2StringList lines;

	lines = aja::split(route_strip, ';');
	if (lines.empty())
		lines.push_back(route_strip);

	int32_t parse_ok = 0;
	for (const auto &l : lines) {
		if (l.empty()) {
			blog(LOG_DEBUG,
			     "aja::Routing::ParseRouteString: Empty line!");
			continue;
		}

		blog(LOG_DEBUG, "aja::Routing::ParseRouteString: Line: %s",
		     l.c_str());

		NTV2StringList tokens = aja::split(l, "->");
		if (tokens.empty() || tokens.size() != 2) {
			blog(LOG_DEBUG,
			     "aja::Routing::ParseRouteString: Invalid token count!");
			continue;
		}

		const std::string &left = tokens[0];  // output crosspoint
		const std::string &right = tokens[1]; // input crosspoint
		if (left.empty() || left.length() > 64) {
			blog(LOG_DEBUG,
			     "aja::Routing::ParseRouteString: Invalid Left token!");
			continue;
		}
		if (right.empty() || right.length() > 64) {
			blog(LOG_DEBUG,
			     "aja::Routing::ParseRouteString: Invalid right token!");
			continue;
		}

		blog(LOG_DEBUG,
		     "aja::Routing::ParseRouteString: Left Token: %s -> Right Token: %s",
		     left.c_str(), right.c_str());

		// Parse Output Crosspoint from left token
		int32_t out_chan = 0;
		int32_t out_ds = 0;
		std::string out_name(64, ' ');
		if (std::sscanf(left.c_str(), "%[A-Za-z_0-9][%d][%d]",
				&out_name[0], &out_chan, &out_ds)) {
			out_name = aja::rstrip(out_name).substr(
				0, out_name.find_first_of('\0'));

			WidgetOutputSocket widget_out;
			if (WidgetOutputSocket::Find(out_name,
						     (NTV2Channel)out_chan,
						     out_ds, widget_out)) {
				blog(LOG_DEBUG,
				     "aja::Routing::ParseRouteString: Found NTV2OutputCrosspointID %s",
				     NTV2OutputCrosspointIDToString(
					     widget_out.id)
					     .c_str());

				// Parse Input Crosspoint from right token
				int32_t inp_chan = 0;
				int32_t inp_ds = 0;
				std::string inp_name(64, ' ');
				if (std::sscanf(right.c_str(),
						"%[A-Za-z_0-9][%d][%d]",
						&inp_name[0], &inp_chan,
						&inp_ds)) {
					inp_name = aja::rstrip(inp_name).substr(
						0,
						inp_name.find_first_of('\0'));

					WidgetInputSocket widget_inp;
					if (WidgetInputSocket::Find(
						    inp_name,
						    (NTV2Channel)inp_chan,
						    inp_ds, widget_inp)) {
						blog(LOG_DEBUG,
						     "aja::Routing::ParseRouteString: Found NTV2InputCrosspointID %s",
						     NTV2InputCrosspointIDToString(
							     widget_inp.id)
							     .c_str());

						cnx[widget_inp.id] =
							widget_out.id;
						parse_ok++;
					} else {
						blog(LOG_DEBUG,
						     "aja::Routing::ParseRouteString: NTV2InputCrosspointID not found!");
					}
				}
			} else {
				blog(LOG_DEBUG,
				     "aja::Routing::ParseRouteString: NTV2OutputCrosspointID not found!");
			}
		}
	}

	return parse_ok > 0;
}

void Routing::StartSourceAudio(const SourceProps &props, CNTV2Card *card)
{
	if (!card)
		return;

	auto inputSrc = props.InitialInputSource();
	auto channel = props.Channel();
	auto audioSys = props.AudioSystem();

	card->WriteAudioSource(0, channel);
	card->SetAudioSystemInputSource(
		audioSys, NTV2InputSourceToAudioSource(inputSrc),
		NTV2InputSourceToEmbeddedAudioInput(inputSrc));

	card->SetNumberAudioChannels(kDefaultAudioChannels, audioSys);
	card->SetAudioRate(props.AudioRate(), audioSys);
	card->SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG, audioSys);

	// Fix for AJA NTV2 internal bug #11467
	ULWord magicAudioBits = 0;
	if (NTV2_INPUT_SOURCE_IS_HDMI(inputSrc)) {
		switch (inputSrc) {
		default:
		case NTV2_INPUTSOURCE_HDMI1:
			magicAudioBits = 0x00100000;
			break;
		case NTV2_INPUTSOURCE_HDMI2:
			magicAudioBits = 0x00110000;
			break;
		case NTV2_INPUTSOURCE_HDMI3:
			magicAudioBits = 0x00900000;
			break;
		case NTV2_INPUTSOURCE_HDMI4:
			magicAudioBits = 0x00910000;
			break;
		}
	} else if (NTV2_INPUT_SOURCE_IS_ANALOG(inputSrc)) {
		magicAudioBits = 0x00000990;
	} else { // SDI
		magicAudioBits = 0x00000320;
	}

	// TODO(paulh): Ask aja-seanl about these deprecated calls and if they are still needed
	ULWord oldValue = 0;
	if (card->ReadAudioSource(oldValue, channel)) {
		card->WriteAudioSource(oldValue | magicAudioBits, channel);
	}

	for (int a = 0; a < NTV2DeviceGetNumAudioSystems(card->GetDeviceID());
	     a++) {
		card->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_ON,
				       NTV2AudioSystem(a));
	}

	card->StartAudioInput(audioSys);
	card->SetAudioCaptureEnable(audioSys, true);
}

void Routing::StopSourceAudio(const SourceProps &props, CNTV2Card *card)
{
	if (card) {
		auto audioSys = props.AudioSystem();
		card->SetAudioCaptureEnable(audioSys, false);
		card->StopAudioInput(audioSys);
	}
}

bool Routing::ConfigureSourceRoute(const SourceProps &props, NTV2Mode mode,
				   CNTV2Card *card, NTV2XptConnections &cnx)
{
	if (!card)
		return false;

	auto deviceID = props.deviceID;
	NTV2VideoFormat vf = props.videoFormat;
	auto init_src = props.InitialInputSource();
	auto init_channel = props.Channel();

	RoutingConfigurator rc;
	RoutingPreset rp;
	ConnectionKind kind = ConnectionKind::Unknown;
	VPIDStandard vpidStandard = VPIDStandard_Unknown;
	HDMIWireFormat hwf = HDMIWireFormat::Unknown;
	if (NTV2_INPUT_SOURCE_IS_SDI(init_src)) {
		kind = ConnectionKind::SDI;
		if (props.autoDetect) {
			auto vpidList = props.vpids;
			if (vpidList.size() > 0)
				vpidStandard = vpidList.at(0).Standard();
		}
		if (vpidStandard == VPIDStandard_Unknown) {
			vpidStandard = DetermineVPIDStandard(
				props.ioSelect, props.videoFormat,
				props.pixelFormat, props.sdiTransport,
				props.sdi4kTransport);
		}
	} else if (NTV2_INPUT_SOURCE_IS_HDMI(init_src)) {
		kind = ConnectionKind::HDMI;
		if (NTV2_IS_FBF_RGB(props.pixelFormat)) {
			if (NTV2_IS_HD_VIDEO_FORMAT(vf)) {
				hwf = HDMIWireFormat::SD_HD_RGB;
			} else if (NTV2_IS_4K_VIDEO_FORMAT(vf)) {
				hwf = HDMIWireFormat::UHD_4K_RGB;
			}
		} else {
			if (NTV2_IS_HD_VIDEO_FORMAT(vf)) {
				hwf = HDMIWireFormat::SD_HD_YCBCR;
			} else if (NTV2_IS_4K_VIDEO_FORMAT(vf)) {
				hwf = HDMIWireFormat::UHD_4K_YCBCR;
			}
		}
	} else {
		blog(LOG_WARNING,
		     "Unsupported connection kind. SDI and HDMI only!");
		return false;
	}

	if (!rc.FindFirstPreset(kind, props.deviceID, NTV2_MODE_CAPTURE, vf,
				props.pixelFormat, vpidStandard, hwf, rp)) {
		blog(LOG_WARNING, "No SDI capture routing preset found!");
		return false;
	}

	LogRoutingPreset(rp);

	// Substitute channel placeholders for actual indices
	std::string route_string = rp.route_string;

	// Channel-substitution for widgets associated with framestore channel(s)
	ULWord start_framestore_index =
		GetIndexForNTV2Channel(props.Framestore());
	const std::vector<std::string> fs_associated = {"fb", "tsi", "dli"};
	for (ULWord c = 0; c < NTV2_MAX_NUM_CHANNELS; c++) {
		for (const auto &name : fs_associated) {
			std::string placeholder = std::string(
				name + "[{ch" + aja::to_string(c + 1) + "}]");
			aja::replace(
				route_string, placeholder,
				name + "[" +
					aja::to_string(start_framestore_index) +
					"]");
		}
		start_framestore_index++;
	}

	// Replace other channel placeholders
	ULWord start_channel_index = GetIndexForNTV2Channel(init_channel);
	for (ULWord c = 0; c < NTV2_MAX_NUM_CHANNELS; c++) {
		std::string channel_placeholder =
			std::string("{ch" + aja::to_string(c + 1) + "}");
		aja::replace(route_string, channel_placeholder,
			     aja::to_string(start_channel_index++));
	}

	if (!ParseRouteString(route_string, cnx))
		return false;

	card->ApplySignalRoute(cnx, false);

	// Apply SDI widget settings
	start_channel_index = GetIndexForNTV2Channel(init_channel);
	for (uint32_t i = (uint32_t)start_channel_index;
	     i < (start_channel_index + rp.num_channels); i++) {
		NTV2Channel channel = GetNTV2ChannelForIndex(i);
		if (::NTV2DeviceHasBiDirectionalSDI(deviceID)) {
			card->SetSDITransmitEnable(channel,
						   mode == NTV2_MODE_DISPLAY);
		}
		card->SetSDIOut3GEnable(channel, rp.flags & kEnable3GOut);
		card->SetSDIOut3GbEnable(channel, rp.flags & kEnable3GbOut);
		card->SetSDIOut6GEnable(channel, rp.flags & kEnable6GOut);
		card->SetSDIOut12GEnable(channel, rp.flags & kEnable12GOut);
		card->SetSDIInLevelBtoLevelAConversion((UWord)i,
						       rp.flags & kConvert3GIn);
		card->SetSDIOutLevelAtoLevelBConversion(
			(UWord)i, rp.flags & kConvert3GOut);
		card->SetSDIOutRGBLevelAConversion(
			(UWord)i, rp.flags & kConvert3GaRGBOut);
	}

	// Apply HDMI settings
	if (aja::IsIOSelectionHDMI(props.ioSelect)) {
		if (NTV2_IS_4K_VIDEO_FORMAT(props.videoFormat))
			card->SetHDMIV2Mode(NTV2_HDMI_V2_4K_CAPTURE);
		else
			card->SetHDMIV2Mode(NTV2_HDMI_V2_HDSD_BIDIRECTIONAL);
	}

	// Apply Framestore settings
	start_framestore_index = GetIndexForNTV2Channel(props.Framestore());
	for (uint32_t i = (uint32_t)start_framestore_index;
	     i < (start_framestore_index + rp.num_framestores); i++) {
		NTV2Channel channel = GetNTV2ChannelForIndex(i);
		card->EnableChannel(channel);
		card->SetMode(channel, mode);
		card->SetVANCMode(NTV2_VANCMODE_OFF, channel);
		card->SetVideoFormat(vf, false, false, channel);
		card->SetFrameBufferFormat(channel, props.pixelFormat);
		card->SetTsiFrameEnable(rp.flags & kEnable4KTSI, channel);
		card->Set4kSquaresEnable(rp.flags & kEnable4KSquares, channel);
		card->SetQuadQuadSquaresEnable(rp.flags & kEnable8KSquares,
					       channel);
	}

	return true;
}

bool Routing::ConfigureOutputRoute(const OutputProps &props, NTV2Mode mode,
				   CNTV2Card *card, NTV2XptConnections &cnx)
{
	if (!card)
		return false;

	auto deviceID = props.deviceID;
	NTV2OutputDestinations outputDests;
	aja::IOSelectionToOutputDests(props.ioSelect, outputDests);
	if (outputDests.empty()) {
		blog(LOG_DEBUG,
		     "No Output Destinations specified to configure routing!");
		return false;
	}
	auto init_dest = *outputDests.begin();

	RoutingConfigurator rc;
	RoutingPreset rp;
	ConnectionKind kind = ConnectionKind::Unknown;
	VPIDStandard vpidStandard = VPIDStandard_Unknown;
	HDMIWireFormat hwf = HDMIWireFormat::Unknown;
	if (NTV2_OUTPUT_DEST_IS_SDI(init_dest)) {
		kind = ConnectionKind::SDI;
		vpidStandard = DetermineVPIDStandard(
			props.ioSelect, props.videoFormat, props.pixelFormat,
			props.sdiTransport, props.sdi4kTransport);
	} else if (NTV2_OUTPUT_DEST_IS_HDMI(init_dest)) {
		kind = ConnectionKind::HDMI;
		hwf = HDMIWireFormat::Unknown;
		if (NTV2_IS_FBF_RGB(props.pixelFormat)) {
			if (NTV2_IS_HD_VIDEO_FORMAT(props.videoFormat)) {
				hwf = HDMIWireFormat::SD_HD_RGB;
			} else if (NTV2_IS_4K_VIDEO_FORMAT(props.videoFormat)) {
				hwf = HDMIWireFormat::UHD_4K_RGB;
			}
		} else {
			if (NTV2_IS_HD_VIDEO_FORMAT(props.videoFormat)) {
				hwf = HDMIWireFormat::SD_HD_YCBCR;
			} else if (NTV2_IS_4K_VIDEO_FORMAT(props.videoFormat)) {
				hwf = HDMIWireFormat::UHD_4K_YCBCR;
			}
		}
	} else {
		blog(LOG_WARNING,
		     "Unsupported connection kind. SDI and HDMI only!");
		return false;
	}

	if (!rc.FindFirstPreset(kind, props.deviceID, NTV2_MODE_DISPLAY,
				props.videoFormat, props.pixelFormat,
				vpidStandard, hwf, rp)) {
		blog(LOG_WARNING, "No HDMI output routing preset found!");
		return false;
	}

	LogRoutingPreset(rp);

	std::string route_string = rp.route_string;

	// Replace framestore channel placeholders
	auto init_channel = NTV2OutputDestinationToChannel(init_dest);
	ULWord start_framestore_index =
		GetIndexForNTV2Channel(props.Framestore());
	if (rp.verbatim) {
		// Presets marked "verbatim" must only be routed on the specified channels
		start_framestore_index = 0;
		init_channel = NTV2_CHANNEL1;
	}

	// Channel-substitution for widgets associated with framestore channel(s)
	const std::vector<std::string> fs_associated = {"fb", "tsi", "dlo"};
	for (ULWord c = 0; c < NTV2_MAX_NUM_CHANNELS; c++) {
		for (const auto &name : fs_associated) {
			std::string placeholder = std::string(
				name + "[{ch" + aja::to_string(c + 1) + "}]");
			aja::replace(
				route_string, placeholder,
				name + "[" +
					aja::to_string(start_framestore_index) +
					"]");
		}
		start_framestore_index++;
	}

	// Replace other channel placeholders
	ULWord start_channel_index = GetIndexForNTV2Channel(init_channel);
	for (ULWord c = 0; c < NTV2_MAX_NUM_CHANNELS; c++) {
		std::string channel_placeholder =
			std::string("{ch" + aja::to_string(c + 1) + "}");
		aja::replace(route_string, channel_placeholder,
			     aja::to_string(start_channel_index++));
	}

	if (!ParseRouteString(route_string, cnx))
		return false;

	card->ApplySignalRoute(cnx, false);

	// Apply SDI widget settings
	if (props.ioSelect != IOSelection::HDMIMonitorOut ||
	    props.deviceID == DEVICE_ID_TTAP_PRO) {
		start_channel_index = GetIndexForNTV2Channel(init_channel);
		for (uint32_t i = (uint32_t)start_channel_index;
		     i < (start_channel_index + rp.num_channels); i++) {
			NTV2Channel channel = GetNTV2ChannelForIndex(i);
			if (::NTV2DeviceHasBiDirectionalSDI(deviceID)) {
				card->SetSDITransmitEnable(
					channel, mode == NTV2_MODE_DISPLAY);
			}
			card->SetSDIOut3GEnable(channel,
						rp.flags & kEnable3GOut);
			card->SetSDIOut3GbEnable(channel,
						 rp.flags & kEnable3GbOut);
			card->SetSDIOut6GEnable(channel,
						rp.flags & kEnable6GOut);
			card->SetSDIOut12GEnable(channel,
						 rp.flags & kEnable12GOut);
			card->SetSDIInLevelBtoLevelAConversion(
				(UWord)i, rp.flags & kConvert3GIn);
			card->SetSDIOutLevelAtoLevelBConversion(
				(UWord)i, rp.flags & kConvert3GOut);
			card->SetSDIOutRGBLevelAConversion(
				(UWord)i, rp.flags & kConvert3GaRGBOut);
		}
	}

	// Apply Framestore settings
	start_framestore_index = GetIndexForNTV2Channel(props.Framestore());
	if (rp.verbatim) {
		start_framestore_index = 0;
	}
	for (uint32_t i = (uint32_t)start_framestore_index;
	     i < (start_framestore_index + rp.num_framestores); i++) {
		NTV2Channel channel = GetNTV2ChannelForIndex(i);
		card->EnableChannel(channel);
		card->SetMode(channel, mode);
		card->SetVANCMode(NTV2_VANCMODE_OFF, channel);
		card->SetVideoFormat(props.videoFormat, false, false, channel);
		card->SetFrameBufferFormat(channel, props.pixelFormat);
		card->SetTsiFrameEnable(rp.flags & kEnable4KTSI, channel);
		card->Set4kSquaresEnable(rp.flags & kEnable4KSquares, channel);
		card->SetQuadQuadSquaresEnable(rp.flags & kEnable8KSquares,
					       channel);
	}

	return true;
}

void Routing::ConfigureOutputAudio(const OutputProps &props, CNTV2Card *card)
{
	if (!card)
		return;

	auto deviceID = card->GetDeviceID();
	auto audioSys = props.AudioSystem();
	auto channel = props.Channel();

	card->SetNumberAudioChannels(props.audioNumChannels, audioSys);
	card->SetAudioRate(props.AudioRate(), audioSys);
	card->SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG, audioSys);
	card->SetAudioOutputDelay(audioSys, 0);

	card->SetSDIOutputAudioSystem(channel, audioSys);
	card->SetSDIOutputDS2AudioSystem(channel, audioSys);

	/* NOTE(paulh):
	 * The SDK has a specifies an SDI audio system by Channel rather than by SDI output
	 * and certain devices require setting the SDI audio system to NTV2_CHANNEL1.
	 * i.e.
	 * SDI 1 = NTV2_CHANNEL1
	 * SDI 2 = NTV2_CHANNEL2
	 * ...
	 * SDI 5/Monitor = NTV2_CHANNEL5
	 * etc...
	 *
	 * This fixes AJA internal bugs: 10730, 10986, 16274
	 */
	if (deviceID == DEVICE_ID_IOXT || deviceID == DEVICE_ID_IO4KUFC ||
	    deviceID == DEVICE_ID_IO4KPLUS || deviceID == DEVICE_ID_KONA1 ||
	    deviceID == DEVICE_ID_KONA3G || deviceID == DEVICE_ID_KONA4UFC ||
	    deviceID == DEVICE_ID_KONA5 || deviceID == DEVICE_ID_KONA5_2X4K) {
		// Make sure SDI out 1 (aka Channel 1) is set to the correct sub-system
		card->SetSDIOutputAudioSystem(NTV2_CHANNEL1, audioSys);
		card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL1, audioSys);
	}

	// make sure that audio is setup for the SDI monitor output on devices that support it
	if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1)) {
		card->SetSDIOutputAudioSystem(NTV2_CHANNEL5, audioSys);
		card->SetSDIOutputDS2AudioSystem(NTV2_CHANNEL5, audioSys);
	}

	card->SetHDMIOutAudioRate(props.AudioRate());
	card->SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);

	card->SetAudioOutputMonitorSource(NTV2_AudioChannel1_2, channel);
	card->SetAESOutputSource(NTV2_AudioChannel1_4, audioSys,
				 NTV2_AudioChannel1_4);
	card->SetAESOutputSource(NTV2_AudioChannel5_8, audioSys,
				 NTV2_AudioChannel5_8);
	card->SetAESOutputSource(NTV2_AudioChannel9_12, audioSys,
				 NTV2_AudioChannel9_12);
	card->SetAESOutputSource(NTV2_AudioChannel13_16, audioSys,
				 NTV2_AudioChannel13_16);

	// make sure that audio is setup for HDMI output on devices that support it
	if (NTV2DeviceGetNumHDMIVideoOutputs(deviceID) > 0) {
		if (NTV2DeviceCanDoAudioMixer(deviceID)) {
			card->SetAudioMixerInputAudioSystem(
				NTV2_AudioMixerInputMain, audioSys);
			card->SetAudioMixerInputChannelSelect(
				NTV2_AudioMixerInputMain, NTV2_AudioChannel1_2);
			card->SetAudioMixerInputChannelsMute(
				NTV2_AudioMixerInputAux1,
				NTV2AudioChannelsMuteAll);
			card->SetAudioMixerInputChannelsMute(
				NTV2_AudioMixerInputAux2,
				NTV2AudioChannelsMuteAll);
		}

		card->SetHDMIOutAudioChannels(NTV2_HDMIAudio8Channels);
		card->SetHDMIOutAudioSource2Channel(NTV2_AudioChannel1_2,
						    audioSys);
		card->SetHDMIOutAudioSource8Channel(NTV2_AudioChannel1_8,
						    audioSys);
	}

	card->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, audioSys);

	card->StopAudioOutput(audioSys);
}

void Routing::LogRoutingPreset(const RoutingPreset &rp)
{
	auto hexStr = [&](uint8_t val) -> std::string {
		std::stringstream ss;
		ss << std::setfill('0') << std::setw(sizeof(uint8_t) * 2)
		   << std::hex << (val | 0);
		return ss.str();
	};

	std::stringstream ss;
	ss << "\nPreset: " << rp.name;
	if (rp.kind == ConnectionKind::SDI) {
		ss << "\nVPID Standard: 0x"
		   << hexStr(static_cast<uint8_t>(rp.vpid_standard));
	}
	ss << "\nMode: " << NTV2ModeToString(rp.mode)
	   << "\nChannels: " << rp.num_channels
	   << "\nFramestores: " << rp.num_framestores;

	blog(LOG_INFO, "[ AJA Crosspoint Routing Preset ]%s", ss.str().c_str());

	if (rp.device_ids.size() > 0) {
		ss.clear();
		for (auto id : rp.device_ids) {
			ss << " - " << NTV2DeviceIDToString(id) << "\n";
		}
		blog(LOG_INFO, "\nCompatible Device IDs: \n%s",
		     ss.str().c_str());
	}
}

} // namespace aja
