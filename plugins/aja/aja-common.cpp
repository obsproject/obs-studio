#include "aja-card-manager.hpp"
#include "aja-common.hpp"
#include "aja-ui-props.hpp"
#include "aja-props.hpp"

#include <ajantv2/includes/ntv2debug.h>
#include <ajantv2/includes/ntv2devicescanner.h>
#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2signalrouter.h>
#include <ajantv2/includes/ntv2utils.h>

void filter_io_selection_input_list(const std::string &cardID,
				    const std::string &channelOwner,
				    obs_property_t *list)
{
	auto &cardManager = aja::CardManager::Instance();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_DEBUG,
		     "filter_io_selection_input_list: Card Entry not found for %s",
		     cardID.c_str());
		return;
	}

	NTV2DeviceID deviceID = DEVICE_ID_NOTFOUND;
	CNTV2Card *card = cardEntry->GetCard();
	if (card)
		deviceID = card->GetDeviceID();

	// Gray out the IOSelection list items that are in use by other plugin instances
	for (size_t idx = 0; idx < obs_property_list_item_count(list); idx++) {
		auto io_select = static_cast<IOSelection>(
			obs_property_list_item_int(list, idx));

		if (io_select == IOSelection::Invalid) {
			obs_property_list_item_disable(list, idx, false);
			continue;
		}

		bool enabled = cardEntry->InputSelectionReady(
			io_select, deviceID, channelOwner);
		obs_property_list_item_disable(list, idx, !enabled);
		blog(LOG_DEBUG, "IOSelection %s = %s",
		     aja::IOSelectionToString(io_select).c_str(),
		     enabled ? "enabled" : "disabled");
	}
}

void filter_io_selection_output_list(const std::string &cardID,
				     const std::string &channelOwner,
				     obs_property_t *list)
{
	auto &cardManager = aja::CardManager::Instance();
	auto cardEntry = cardManager.GetCardEntry(cardID);
	if (!cardEntry) {
		blog(LOG_DEBUG,
		     "filter_io_selection_output_list: Card Entry not found for %s",
		     cardID.c_str());
		return;
	}

	NTV2DeviceID deviceID = DEVICE_ID_NOTFOUND;
	CNTV2Card *card = cardEntry->GetCard();
	if (card)
		deviceID = card->GetDeviceID();

	// Gray out the IOSelection list items that are in use by other plugin instances
	for (size_t idx = 0; idx < obs_property_list_item_count(list); idx++) {
		auto io_select = static_cast<IOSelection>(
			obs_property_list_item_int(list, idx));
		if (io_select == IOSelection::Invalid) {
			obs_property_list_item_disable(list, idx, false);
			continue;
		}

		bool enabled = cardEntry->OutputSelectionReady(
			io_select, deviceID, channelOwner);
		obs_property_list_item_disable(list, idx, !enabled);
		blog(LOG_DEBUG, "IOSelection %s = %s",
		     aja::IOSelectionToString(io_select).c_str(),
		     enabled ? "enabled" : "disabled");
	}
}

void populate_io_selection_input_list(const std::string &cardID,
				      const std::string &channelOwner,
				      NTV2DeviceID deviceID,
				      obs_property_t *list)
{
	obs_property_list_clear(list);
	obs_property_list_add_int(list,
				  obs_module_text(kUIPropIOSelectNone.text),
				  static_cast<long long>(IOSelection::Invalid));

	for (auto i = 0; i < static_cast<int32_t>(IOSelection::NumIOSelections);
	     i++) {
		auto ioSelect = static_cast<IOSelection>(i);
		if (ioSelect == IOSelection::AnalogIn)
			continue;
		if (aja::DeviceCanDoIOSelectionIn(deviceID, ioSelect)) {
			obs_property_list_add_int(
				list,
				aja::IOSelectionToString(ioSelect).c_str(),
				static_cast<long long>(ioSelect));
		}
	}

	filter_io_selection_input_list(cardID, channelOwner, list);
}

void populate_io_selection_output_list(const std::string &cardID,
				       const std::string &channelOwner,
				       NTV2DeviceID deviceID,
				       obs_property_t *list)
{
	obs_property_list_clear(list);
	obs_property_list_add_int(list,
				  obs_module_text(kUIPropIOSelectNone.text),
				  static_cast<long long>(IOSelection::Invalid));

	if (deviceID == DEVICE_ID_TTAP_PRO) {
		obs_property_list_add_int(
			list, "SDI & HDMI",
			static_cast<long long>(IOSelection::HDMIMonitorOut));
	} else {
		for (auto i = 0;
		     i < static_cast<int32_t>(IOSelection::NumIOSelections);
		     i++) {
			auto ioSelect = static_cast<IOSelection>(i);
			if (ioSelect == IOSelection::Invalid ||
			    ioSelect == IOSelection::AnalogOut)
				continue;

			if (aja::DeviceCanDoIOSelectionOut(deviceID,
							   ioSelect)) {
				obs_property_list_add_int(
					list,
					aja::IOSelectionToString(ioSelect)
						.c_str(),
					static_cast<long long>(ioSelect));
			}
		}
	}

	filter_io_selection_output_list(cardID, channelOwner, list);
}

void populate_video_format_list(NTV2DeviceID deviceID, obs_property_t *list,
				NTV2VideoFormat genlockFormat, bool want4KHFR,
				bool matchFPS)
{
	VideoFormatList videoFormats = {};
	VideoStandardList orderedStandards = {};
	orderedStandards.push_back(NTV2_STANDARD_525);
	orderedStandards.push_back(NTV2_STANDARD_625);
	if (NTV2DeviceCanDoHDVideo(deviceID)) {
		orderedStandards.push_back(NTV2_STANDARD_720);
		orderedStandards.push_back(NTV2_STANDARD_1080);
		orderedStandards.push_back(NTV2_STANDARD_1080p);
		orderedStandards.push_back(NTV2_STANDARD_2K);
		orderedStandards.push_back(NTV2_STANDARD_2Kx1080p);
		orderedStandards.push_back(NTV2_STANDARD_2Kx1080i);
	}
	if (NTV2DeviceCanDo4KVideo(deviceID)) {
		orderedStandards.push_back(NTV2_STANDARD_3840i);
		orderedStandards.push_back(NTV2_STANDARD_3840x2160p);
		if (want4KHFR)
			orderedStandards.push_back(NTV2_STANDARD_3840HFR);
		orderedStandards.push_back(NTV2_STANDARD_4096i);
		orderedStandards.push_back(NTV2_STANDARD_4096x2160p);
		if (want4KHFR)
			orderedStandards.push_back(NTV2_STANDARD_4096HFR);
	}

	aja::GetSortedVideoFormats(deviceID, orderedStandards, videoFormats);
	for (const auto &vf : videoFormats) {
		bool addFormat = true;

		// Filter formats by framerate family if specified
		if (genlockFormat != NTV2_FORMAT_UNKNOWN)
			addFormat = IsMultiFormatCompatible(genlockFormat, vf);

		struct obs_video_info ovi;
		if (matchFPS && obs_get_video_info(&ovi)) {
			NTV2FrameRate frameRate =
				GetNTV2FrameRateFromVideoFormat(vf);
			ULWord fpsNum = 0;
			ULWord fpsDen = 0;
			GetFramesPerSecond(frameRate, fpsNum, fpsDen);
			uint32_t obsFrameTime =
				1000000 * ovi.fps_den / ovi.fps_num;
			uint32_t ajaFrameTime = 1000000 * fpsDen / fpsNum;
			if (obsFrameTime != ajaFrameTime)
				addFormat = false;
		}

		if (addFormat) {
			std::string name = NTV2VideoFormatToString(vf, true);
			obs_property_list_add_int(list, name.c_str(), (int)vf);
		}
	}
}

void populate_pixel_format_list(NTV2DeviceID deviceID,
				const std::vector<NTV2PixelFormat> &fmts,
				obs_property_t *list)
{
	for (auto &&pf : fmts) {
		if (NTV2DeviceCanDoFrameBufferFormat(deviceID, pf)) {
			obs_property_list_add_int(
				list,
				NTV2FrameBufferFormatToString(pf, true).c_str(),
				static_cast<long long>(pf));
		}
	}
}

void populate_sdi_transport_list(obs_property_t *list, NTV2DeviceID deviceID,
				 bool capture)
{
	if (capture) {
		obs_property_list_add_int(list, obs_module_text("Auto"),
					  kAutoDetect);
	}
	for (int i = 0; i < (int)SDITransport::Unknown; i++) {
		SDITransport sdi_trx = static_cast<SDITransport>(i);
		if (sdi_trx == SDITransport::SDI6G ||
		    sdi_trx == SDITransport::SDI12G) {
			if (!NTV2DeviceCanDo12GSDI(deviceID))
				continue;
		}
		// Disabling 12G in Output plugin until AJA 4K HFR bug is fixed
		if (!capture && sdi_trx == SDITransport::SDI12G)
			continue;
		obs_property_list_add_int(
			list, aja::SDITransportToString(sdi_trx).c_str(),
			static_cast<long long>(sdi_trx));
	}
}

void populate_sdi_4k_transport_list(obs_property_t *list)
{
	obs_property_list_add_int(
		list,
		aja::SDITransport4KToString(SDITransport4K::Squares).c_str(),
		static_cast<long long>(SDITransport4K::Squares));
	obs_property_list_add_int(
		list,
		aja::SDITransport4KToString(SDITransport4K::TwoSampleInterleave)
			.c_str(),
		static_cast<long long>(SDITransport4K::TwoSampleInterleave));
}

bool aja_video_format_changed(obs_properties_t *props, obs_property_t *list,
			      obs_data_t *settings)
{
	auto vid_fmt = static_cast<NTV2VideoFormat>(
		obs_data_get_int(settings, kUIPropVideoFormatSelect.id));
	size_t itemCount = obs_property_list_item_count(list);
	bool itemFound = false;
	for (size_t i = 0; i < itemCount; i++) {
		auto itemFormat = static_cast<NTV2VideoFormat>(
			obs_property_list_item_int(list, i));
		if (itemFormat == vid_fmt) {
			itemFound = true;
			break;
		}
	}

	if (!itemFound) {
		obs_property_list_insert_int(list, 0, "", vid_fmt);
		obs_property_list_item_disable(list, 0, true);
		return true;
	}

	obs_property_t *sdi_4k_trx =
		obs_properties_get(props, kUIPropSDITransport4K.id);
	obs_property_set_visible(sdi_4k_trx, NTV2_IS_4K_VIDEO_FORMAT(vid_fmt));

	return true;
}

namespace aja {

video_format AJAPixelFormatToOBSVideoFormat(NTV2PixelFormat pf)
{
	video_format obs_video_format = VIDEO_FORMAT_NONE;
	switch (pf) {
	case NTV2_FBF_8BIT_YCBCR:
		obs_video_format = VIDEO_FORMAT_UYVY;
		break;
	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
		obs_video_format = VIDEO_FORMAT_BGR3;
		break;
	case NTV2_FBF_ARGB:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_RGBA:
		obs_video_format = VIDEO_FORMAT_BGRA;
		break;
	case NTV2_FBF_10BIT_YCBCR:
		obs_video_format = VIDEO_FORMAT_V210;
		break;
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_8BIT_YCBCR_YUY2:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_10BIT_YCBCR_DPX:
	case NTV2_FBF_8BIT_DVCPRO:
	case NTV2_FBF_8BIT_YCBCR_420PL3:
	case NTV2_FBF_8BIT_HDV:
	case NTV2_FBF_10BIT_YCBCRA:
	case NTV2_FBF_10BIT_DPX_LE:
	case NTV2_FBF_48BIT_RGB:
	case NTV2_FBF_12BIT_RGB_PACKED:
	case NTV2_FBF_PRORES_DVCPRO:
	case NTV2_FBF_PRORES_HDV:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_10BIT_ARGB:
	case NTV2_FBF_16BIT_ARGB:
	case NTV2_FBF_8BIT_YCBCR_422PL3:
	case NTV2_FBF_10BIT_RAW_RGB:
	case NTV2_FBF_10BIT_RAW_YCBCR:
	case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
	case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
	case NTV2_FBF_10BIT_YCBCR_420PL2:
	case NTV2_FBF_10BIT_YCBCR_422PL2:
	case NTV2_FBF_8BIT_YCBCR_420PL2:
	case NTV2_FBF_8BIT_YCBCR_422PL2:
	default:
		obs_video_format = VIDEO_FORMAT_NONE;
		break;
	}

	return obs_video_format;
}

void GetSortedVideoFormats(NTV2DeviceID id, const VideoStandardList &standards,
			   VideoFormatList &videoFormats)
{
	if (standards.empty())
		return;

	VideoFormatMap videoFormatMap;

	// Bin all the formats based on video standard
	for (size_t i = (size_t)NTV2_FORMAT_UNKNOWN;
	     i < (size_t)NTV2_MAX_NUM_VIDEO_FORMATS; i++) {
		NTV2VideoFormat fmt = (NTV2VideoFormat)i;
		NTV2Standard standard = GetNTV2StandardFromVideoFormat(fmt);
		if (id != DEVICE_ID_NOTFOUND &&
		    NTV2DeviceCanDoVideoFormat(id, fmt)) {
			if (videoFormatMap.count(standard)) {
				videoFormatMap.at(standard).push_back(fmt);
			} else {
				std::vector<NTV2VideoFormat> v;
				v.push_back(fmt);
				videoFormatMap.insert(
					std::pair<NTV2Standard,
						  std::vector<NTV2VideoFormat>>(
						standard, v));
			}
		}
	}

	for (size_t v = (size_t)NTV2_STANDARD_1080;
	     v < (size_t)NTV2_NUM_STANDARDS; v++) {
		NTV2Standard standard = (NTV2Standard)v;

		if (videoFormatMap.count(standard)) {
			std::sort(videoFormatMap.at(standard).begin(),
				  videoFormatMap.at(standard).end(),
				  [&](const NTV2VideoFormat &d1,
				      const NTV2VideoFormat &d2) {
					  std::string d1Str, d2Str;

					  d1Str = NTV2VideoFormatToString(d1);
					  d2Str = NTV2VideoFormatToString(d2);

					  return d1Str < d2Str;
				  });
		}
	}

	for (size_t v = 0; v < standards.size(); v++) {
		NTV2Standard standard = standards.at(v);
		if (videoFormatMap.count(standard)) {
			for (size_t i = 0;
			     i < videoFormatMap.at(standard).size(); i++) {
				NTV2VideoFormat vf =
					videoFormatMap.at(standard).at(i);
				videoFormats.push_back(vf);
			}
		}
	}
}

NTV2VideoFormat HandleSpecialCaseFormats(IOSelection io, NTV2VideoFormat vf,
					 NTV2DeviceID id)
{
	// 1080p Level-B formats and ST372M
	if (NTV2_VIDEO_FORMAT_IS_B(vf) &&
	    !(IsSDITwoWireIOSelection(io) && NTV2_IS_HD_VIDEO_FORMAT(vf))) {
		vf = aja::GetLevelAFormatForLevelBFormat(vf);
	}
	// UHD/4K Square Division auto-detect
	if ((io == IOSelection::SDI1__4 || io == IOSelection::SDI5__8) &&
	    NTV2_IS_HD_VIDEO_FORMAT(vf)) {
		vf = GetQuadSizedVideoFormat(vf, true);
	}
	// Kona5/io4K+ auto-detection of UHD/4K 6G/12G SDI formats.
	if (aja::IsSDIOneWireIOSelection(io) && NTV2_IS_4K_VIDEO_FORMAT(vf) &&
	    !NTV2_IS_SQUARE_DIVISION_FORMAT(vf) &&
	    !NTV2DeviceCanDo12gRouting(id)) {
		vf = GetQuadSizedVideoFormat(GetQuarterSizedVideoFormat(vf));
	}
	return vf;
}

NTV2Channel WidgetIDToChannel(NTV2WidgetID id)
{
#if AJA_NTV2_SDK_VERSION_MAJOR <= 16 && AJA_NTV2_SDK_VERSION_MINOR <= 2
	// Workaround for bug in NTV2 SDK <= 16.2 where NTV2_WgtSDIMonOut1 maps incorrectly to NTV2_CHANNEL1.
	if (id == NTV2_WgtSDIMonOut1)
		return NTV2_CHANNEL5;
#endif
	return CNTV2SignalRouter::WidgetIDToChannel(id);
}

uint32_t CardNumFramestores(NTV2DeviceID id)
{
	auto numFramestores = NTV2DeviceGetNumFrameStores(id);
	if (id == DEVICE_ID_CORVIDHBR) {
		numFramestores = 1;
	}
	return numFramestores;
}

uint32_t CardNumAudioSystems(NTV2DeviceID id)
{
	if (id == DEVICE_ID_KONALHI || id == DEVICE_ID_KONALHEPLUS)
		return 2;

	return NTV2DeviceGetNumAudioSystems(id);
}

// IO4K and IO4K+ perform SDI Monitor Output on "SDI5" and "Framestore 4".
bool CardCanDoSDIMonitorOutput(NTV2DeviceID id)
{
	return (id == DEVICE_ID_IO4K || id == DEVICE_ID_IO4KPLUS);
}

// Cards with a dedicated HDMI Monitor Input, tied to "Framestore 4".
bool CardCanDoHDMIMonitorInput(NTV2DeviceID id)
{
	return (id == DEVICE_ID_IO4K || id == DEVICE_ID_IO4KUFC ||
		id == DEVICE_ID_IO4KPLUS || id == DEVICE_ID_IOXT ||
		id == DEVICE_ID_IOX3 || id == DEVICE_ID_KONALHI);
}

// Cards with a dedicated HDMI Monitor Output, tied to "Framestore 4".
bool CardCanDoHDMIMonitorOutput(NTV2DeviceID id)
{
	return (id == DEVICE_ID_IO4K || id == DEVICE_ID_IO4KPLUS ||
		id == DEVICE_ID_IOXT || id == DEVICE_ID_IOX3 ||
		id == DEVICE_ID_KONA4 || id == DEVICE_ID_KONA5 ||
		id == DEVICE_ID_KONA5_8K || id == DEVICE_ID_KONA5_2X4K ||
		id == DEVICE_ID_KONA5_8KMK);
}

// Cards capable of 1x SDI at 6G/12G.
bool CardCanDo1xSDI12G(NTV2DeviceID id)
{
	return (id == DEVICE_ID_KONA5_8K || id == DEVICE_ID_KONA5_8KMK ||
		id == DEVICE_ID_KONA5 || id == DEVICE_ID_KONA5_2X4K ||
		id == DEVICE_ID_IO4KPLUS || id == DEVICE_ID_CORVID44_12G);
}

// Check for 3G level-B SDI on the wire.
bool Is3GLevelB(CNTV2Card *card, NTV2Channel channel)
{
	if (!card)
		return false;

	bool levelB = false;
	auto deviceID = card->GetDeviceID();
	UWord channelIndex = static_cast<UWord>(channel);

	if (NTV2DeviceCanDo3GIn(deviceID, channelIndex) ||
	    NTV2DeviceCanDo12GIn(deviceID, channelIndex)) {
		if (!card->GetSDIInput3GbPresent(levelB, channel))
			return false;
	}

	return levelB;
}

// Get the 3G Level-A enum for a 3G Level-B format enum.
NTV2VideoFormat GetLevelAFormatForLevelBFormat(NTV2VideoFormat vf)
{
	NTV2VideoFormat result = vf;
	switch (vf) {
	default:
		break;
	case NTV2_FORMAT_1080p_5000_B:
		result = NTV2_FORMAT_1080p_5000_A;
		break;
	case NTV2_FORMAT_1080p_5994_B:
		result = NTV2_FORMAT_1080p_5994_A;
		break;
	case NTV2_FORMAT_1080p_6000_B:
		result = NTV2_FORMAT_1080p_6000_A;
		break;
	case NTV2_FORMAT_1080p_2K_4795_B:
		result = NTV2_FORMAT_1080p_2K_4795_A;
		break;
	case NTV2_FORMAT_1080p_2K_4800_B:
		result = NTV2_FORMAT_1080p_2K_4800_A;
		break;
	case NTV2_FORMAT_1080p_2K_5000_B:
		result = NTV2_FORMAT_1080p_2K_5000_A;
		break;
	case NTV2_FORMAT_1080p_2K_5994_B:
		result = NTV2_FORMAT_1080p_2K_5994_A;
		break;
	case NTV2_FORMAT_1080p_2K_6000_B:
		result = NTV2_FORMAT_1080p_2K_6000_A;
		break;
	}
	return result;
}

NTV2VideoFormat InterlacedFormatForPsfFormat(NTV2VideoFormat vf)
{
	NTV2VideoFormat result = vf;
	switch (vf) {
	default:
		break;
	case NTV2_FORMAT_1080psf_2500_2:
		result = NTV2_FORMAT_1080i_5000;
		break;
	case NTV2_FORMAT_1080psf_2997_2:
		result = NTV2_FORMAT_1080i_5994;
		break;
	}
	return result;
}

// Certain cards only have 1 SDI spigot.
bool IsSingleSDIDevice(NTV2DeviceID id)
{
	return (id == DEVICE_ID_TTAP_PRO || id == DEVICE_ID_KONA1);
}

bool IsIODevice(NTV2DeviceID id)
{
	return (id == DEVICE_ID_IOXT || id == DEVICE_ID_IOX3 ||
		id == DEVICE_ID_IO4K || id == DEVICE_ID_IO4KPLUS ||
		id == DEVICE_ID_IOIP_2022 || id == DEVICE_ID_IOIP_2110);
}

/* Kona5 retail firmware and io4K+ have limited support for 6G/12G SDI
 * where SDI 1 is capable of capture and SDI 3 is capable of output.
 */
bool IsRetail12GSDICard(NTV2DeviceID id)
{
	return (id == DEVICE_ID_KONA5 || id == DEVICE_ID_IO4KPLUS);
}

bool IsOutputOnlyDevice(NTV2DeviceID id)
{
	return id == DEVICE_ID_TTAP_PRO;
}

std::string SDITransportToString(SDITransport mode)
{
	std::string str = "";
	switch (mode) {
	case SDITransport::SingleLink:
		str = "SD/HD Single Link";
		break;
	case SDITransport::HDDualLink:
		str = "HD Dual-Link";
		break;
	case SDITransport::SDI3Ga:
		str = "3G Level-A (3Ga)";
		break;
	case SDITransport::SDI3Gb:
		str = "3G Level-B (3Gb)";
		break;
	case SDITransport::SDI6G:
		str = "6G";
		break;
	case SDITransport::SDI12G:
		str = "12G";
		break;
	case SDITransport::Unknown:
		str = "Unknown";
		break;
	}
	return str;
}

std::string SDITransport4KToString(SDITransport4K mode)
{
	std::string str = "";
	switch (mode) {
	case SDITransport4K::Squares:
		str = "Squares";
		break;
	case SDITransport4K::TwoSampleInterleave:
		str = "2SI";
		break;
	default:
	case SDITransport4K::Unknown:
		str = "Unknown";
		break;
	}
	return str;
}

std::string IOSelectionToString(IOSelection io)
{
	std::string str;

	switch (io) {
	case IOSelection::SDI1:
		str = "SDI 1";
		break;
	case IOSelection::SDI2:
		str = "SDI 2";
		break;
	case IOSelection::SDI3:
		str = "SDI 3";
		break;
	case IOSelection::SDI4:
		str = "SDI 4";
		break;
	case IOSelection::SDI5:
		str = "SDI 5";
		break;
	case IOSelection::SDI6:
		str = "SDI 6";
		break;
	case IOSelection::SDI7:
		str = "SDI 7";
		break;
	case IOSelection::SDI8:
		str = "SDI 8";
		break;
	case IOSelection::SDI1_2:
		str = "SDI 1 & 2";
		break;
	case IOSelection::SDI3_4:
		str = "SDI 3 & 4";
		break;
	case IOSelection::SDI5_6:
		str = "SDI 5 & 6";
		break;
	case IOSelection::SDI7_8:
		str = "SDI 7 & 8";
		break;
	case IOSelection::SDI1__4:
		str = "SDI 1-4";
		break;
	case IOSelection::SDI5__8:
		str = "SDI 5-8";
		break;
	case IOSelection::HDMI1:
		str = "HDMI 1";
		break;
	case IOSelection::HDMI2:
		str = "HDMI 2";
		break;
	case IOSelection::HDMI3:
		str = "HDMI 3";
		break;
	case IOSelection::HDMI4:
		str = "HDMI 4";
		break;
	case IOSelection::HDMIMonitorIn:
		str = "HDMI IN";
		break;
	case IOSelection::HDMIMonitorOut:
		str = "HDMI OUT";
		break;
	case IOSelection::AnalogIn:
		str = "ANALOG IN";
		break;
	case IOSelection::AnalogOut:
		str = "ANALOG OUT";
		break;
	case IOSelection::Invalid:
		str = "Invalid";
		break;
	}

	return str;
}

void IOSelectionToInputSources(IOSelection io, NTV2InputSourceSet &inputSources)
{
	switch (io) {
	case IOSelection::SDI1:
		inputSources.insert(NTV2_INPUTSOURCE_SDI1);
		break;
	case IOSelection::SDI2:
		inputSources.insert(NTV2_INPUTSOURCE_SDI2);
		break;
	case IOSelection::SDI3:
		inputSources.insert(NTV2_INPUTSOURCE_SDI3);
		break;
	case IOSelection::SDI4:
		inputSources.insert(NTV2_INPUTSOURCE_SDI4);
		break;
	case IOSelection::SDI5:
		inputSources.insert(NTV2_INPUTSOURCE_SDI5);
		break;
	case IOSelection::SDI6:
		inputSources.insert(NTV2_INPUTSOURCE_SDI6);
		break;
	case IOSelection::SDI7:
		inputSources.insert(NTV2_INPUTSOURCE_SDI7);
		break;
	case IOSelection::SDI8:
		inputSources.insert(NTV2_INPUTSOURCE_SDI8);
		break;
	case IOSelection::SDI1_2:
		inputSources.insert(NTV2_INPUTSOURCE_SDI1);
		inputSources.insert(NTV2_INPUTSOURCE_SDI2);
		break;
	case IOSelection::SDI3_4:
		inputSources.insert(NTV2_INPUTSOURCE_SDI3);
		inputSources.insert(NTV2_INPUTSOURCE_SDI4);
		break;
	case IOSelection::SDI5_6:
		inputSources.insert(NTV2_INPUTSOURCE_SDI5);
		inputSources.insert(NTV2_INPUTSOURCE_SDI6);
		break;
	case IOSelection::SDI7_8:
		inputSources.insert(NTV2_INPUTSOURCE_SDI7);
		inputSources.insert(NTV2_INPUTSOURCE_SDI8);
		break;
	case IOSelection::SDI1__4:
		inputSources.insert(NTV2_INPUTSOURCE_SDI1);
		inputSources.insert(NTV2_INPUTSOURCE_SDI2);
		inputSources.insert(NTV2_INPUTSOURCE_SDI3);
		inputSources.insert(NTV2_INPUTSOURCE_SDI4);
		break;
	case IOSelection::SDI5__8:
		inputSources.insert(NTV2_INPUTSOURCE_SDI5);
		inputSources.insert(NTV2_INPUTSOURCE_SDI6);
		inputSources.insert(NTV2_INPUTSOURCE_SDI7);
		inputSources.insert(NTV2_INPUTSOURCE_SDI8);
		break;
	case IOSelection::HDMI1:
		inputSources.insert(NTV2_INPUTSOURCE_HDMI1);
		break;
	case IOSelection::HDMI2:
		inputSources.insert(NTV2_INPUTSOURCE_HDMI2);
		break;
	case IOSelection::HDMI3:
		inputSources.insert(NTV2_INPUTSOURCE_HDMI3);
		break;
	case IOSelection::HDMI4:
		inputSources.insert(NTV2_INPUTSOURCE_HDMI4);
		break;
	case IOSelection::HDMIMonitorIn:
		inputSources.insert(NTV2_INPUTSOURCE_HDMI1);
		break;
	case IOSelection::AnalogIn:
		inputSources.insert(NTV2_INPUTSOURCE_ANALOG1);
		break;
	default:
	case IOSelection::HDMIMonitorOut:
	case IOSelection::AnalogOut:
	case IOSelection::Invalid:
		break;
	}
}

void IOSelectionToOutputDests(IOSelection io,
			      NTV2OutputDestinations &outputDests)
{
	switch (io) {
	case IOSelection::SDI1:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI1);
		break;
	case IOSelection::SDI2:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI2);
		break;
	case IOSelection::SDI3:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI3);
		break;
	case IOSelection::SDI4:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI4);
		break;
	case IOSelection::SDI5:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI5);
		break;
	case IOSelection::SDI6:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI6);
		break;
	case IOSelection::SDI7:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI7);
		break;
	case IOSelection::SDI8:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI8);
		break;
	case IOSelection::SDI1_2:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI1);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI2);
		break;
	case IOSelection::SDI3_4:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI3);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI4);
		break;
	case IOSelection::SDI5_6:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI5);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI6);
		break;
	case IOSelection::SDI7_8:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI7);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI8);
		break;
	case IOSelection::SDI1__4:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI1);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI2);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI3);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI4);
		break;
	case IOSelection::SDI5__8:
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI5);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI6);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI7);
		outputDests.insert(NTV2_OUTPUTDESTINATION_SDI8);
		break;
	case IOSelection::HDMIMonitorOut:
		outputDests.insert(NTV2_OUTPUTDESTINATION_HDMI);
		break;
	case IOSelection::AnalogOut:
		outputDests.insert(NTV2_OUTPUTDESTINATION_ANALOG);
		break;
	default:
	case IOSelection::HDMI1:
	case IOSelection::HDMI2:
	case IOSelection::HDMI3:
	case IOSelection::HDMI4:
	case IOSelection::HDMIMonitorIn:
	case IOSelection::AnalogIn:
	case IOSelection::Invalid:
		break;
	}
}

bool DeviceCanDoIOSelectionIn(NTV2DeviceID id, IOSelection io)
{
	// Hide "HDMI1" list selection on devices which have a discrete "HDMI IN" port.
	if (io == IOSelection::HDMI1 && CardCanDoHDMIMonitorInput(id)) {
		return false;
	}
	if (io == IOSelection::HDMIMonitorIn && id == DEVICE_ID_KONAHDMI) {
		return false;
	}

	NTV2InputSourceSet inputSources;
	if (io != IOSelection::Invalid) {
		IOSelectionToInputSources(io, inputSources);
		size_t numSrcs = inputSources.size();
		size_t canDo = 0;
		if (numSrcs > 0) {
			for (auto &&inp : inputSources) {
				if (NTV2DeviceCanDoInputSource(id, inp))
					canDo++;
			}

			if (canDo == numSrcs)
				return true;
		}
	}
	return false;
}

bool DeviceCanDoIOSelectionOut(NTV2DeviceID id, IOSelection io)
{
	NTV2OutputDestinations outputDests;
	if (io != IOSelection::Invalid) {
		IOSelectionToOutputDests(io, outputDests);
		size_t numOuts = outputDests.size();
		size_t canDo = 0;
		if (numOuts > 0) {
			for (auto &&out : outputDests) {
				if (NTV2DeviceCanDoOutputDestination(id, out))
					canDo++;
			}

			if (canDo == numOuts)
				return true;
		}
	}
	return false;
}

bool IsSDIOneWireIOSelection(IOSelection io)
{
	bool result = false;
	switch (io) {
	case IOSelection::SDI1:
	case IOSelection::SDI2:
	case IOSelection::SDI3:
	case IOSelection::SDI4:
	case IOSelection::SDI5:
	case IOSelection::SDI6:
	case IOSelection::SDI7:
	case IOSelection::SDI8:
		result = true;
		break;
	default:
		result = false;
	}
	return result;
}

bool IsSDITwoWireIOSelection(IOSelection io)
{
	bool result = false;
	switch (io) {
	case IOSelection::SDI1_2:
	case IOSelection::SDI3_4:
	case IOSelection::SDI5_6:
	case IOSelection::SDI7_8:
		result = true;
		break;
	default:
		result = false;
	}
	return result;
}

bool IsSDIFourWireIOSelection(IOSelection io)
{
	bool result = false;
	switch (io) {
	case IOSelection::SDI1__4:
	case IOSelection::SDI5__8:
		result = true;
		break;
	default:
		result = false;
	}
	return result;
}

bool IsMonitorOutputSelection(NTV2DeviceID id, IOSelection io)
{
	if (CardCanDoSDIMonitorOutput(id) && io == IOSelection::SDI5)
		return true;

	if (CardCanDoHDMIMonitorOutput(id) && io == IOSelection::HDMIMonitorOut)
		return true;

	return false;
}

bool IsIOSelectionSDI(IOSelection io)
{
	if (io == IOSelection::SDI1 || io == IOSelection::SDI2 ||
	    io == IOSelection::SDI3 || io == IOSelection::SDI4 ||
	    io == IOSelection::SDI5 || io == IOSelection::SDI6 ||
	    io == IOSelection::SDI7 || io == IOSelection::SDI8 ||
	    io == IOSelection::SDI1_2 || io == IOSelection::SDI3_4 ||
	    io == IOSelection::SDI5_6 || io == IOSelection::SDI7_8 ||
	    io == IOSelection::SDI1__4 || io == IOSelection::SDI5__8) {
		return true;
	}
	return false;
}

bool IsIOSelectionHDMI(IOSelection io)
{
	if (io == IOSelection::HDMI1 || io == IOSelection::HDMI2 ||
	    io == IOSelection::HDMI3 || io == IOSelection::HDMI4 ||
	    io == IOSelection::HDMIMonitorIn ||
	    io == IOSelection::HDMIMonitorOut) {
		return true;
	}
	return false;
}

std::string MakeCardID(CNTV2Card &card)
{
	std::string cardID;
	if (card.GetSerialNumberString(cardID)) {
		// Try to construct CardID from device ID and serial number...
		cardID = NTV2DeviceIDToString(card.GetDeviceID(), false) + "_" +
			 cardID;
	} else {
		// ...otherwise fall back to the CNTV2DeviceScanner method.
		cardID = CNTV2DeviceScanner::GetDeviceRefName(card);
	}
	return cardID;
}

RasterDefinition DetermineRasterDefinition(NTV2VideoFormat vf)
{
	RasterDefinition def = RasterDefinition::Unknown;
	if (NTV2_IS_SD_VIDEO_FORMAT(vf)) {
		def = RasterDefinition::SD;
	} else if (NTV2_IS_HD_VIDEO_FORMAT(vf)) {
		def = RasterDefinition::HD;
	} else if (NTV2_IS_QUAD_FRAME_FORMAT(vf)) {
		def = RasterDefinition::UHD_4K;
	} else if (NTV2_IS_QUAD_QUAD_FORMAT(vf)) {
		def = RasterDefinition::UHD2_8K;
	} else {
		def = RasterDefinition::Unknown;
	}
	return def;
}

inline bool IsStandard1080i(NTV2Standard standard)
{
	if (standard == NTV2_STANDARD_1080 ||
	    standard == NTV2_STANDARD_2Kx1080i) {
		return true;
	}
	return false;
}
inline bool IsStandard1080p(NTV2Standard standard)
{
	if (standard == NTV2_STANDARD_1080p || standard == NTV2_STANDARD_2K ||
	    standard == NTV2_STANDARD_2Kx1080p) {
		return true;
	}
	return false;
}

VPIDStandard DetermineVPIDStandard(IOSelection io, NTV2VideoFormat vf,
				   NTV2PixelFormat pf, SDITransport trx,
				   SDITransport4K t4k)
{
	VPIDStandard vpid = VPIDStandard_Unknown;
	auto rd = aja::DetermineRasterDefinition(vf);
	auto standard = GetNTV2StandardFromVideoFormat(vf);
	bool is_rgb = NTV2_IS_FBF_RGB(pf);
	bool is_hfr =
		NTV2_IS_HIGH_NTV2FrameRate(GetNTV2FrameRateFromVideoFormat(vf));
	if (rd == RasterDefinition::SD) {
		vpid = VPIDStandard_483_576;
	} else if (rd == RasterDefinition::HD) {
		vpid = VPIDStandard_1080;
		if (aja::IsSDIOneWireIOSelection(io)) {
			if (is_rgb) {
				if (standard == NTV2_STANDARD_720) {
					if (trx == SDITransport::SingleLink) {
						vpid = VPIDStandard_720;
					} else if (trx ==
						   SDITransport::SDI3Ga) {
						vpid = VPIDStandard_720_3Ga;
					} else if (trx ==
						   SDITransport::SDI3Gb) {
						vpid = VPIDStandard_720_3Gb;
					}
				} else if (IsStandard1080p(standard)) {
					if (trx == SDITransport::SingleLink) {
						vpid = VPIDStandard_1080;
					} else if (trx ==
						   SDITransport::SDI3Ga) {
						vpid = VPIDStandard_1080_3Ga;
					} else if (trx ==
						   SDITransport::SDI3Gb) {
						vpid = VPIDStandard_1080_3Gb;
					}
				}
			} else {
				if (standard == NTV2_STANDARD_720) {
					vpid = VPIDStandard_720;
				} else if (IsStandard1080i(standard)) {
					vpid = VPIDStandard_1080;
				} else if (IsStandard1080p(standard) &&
					   trx == SDITransport::SDI3Ga) {
					vpid = VPIDStandard_1080_3Ga;
				} else if (IsStandard1080p(standard) &&
					   trx == SDITransport::SDI3Gb) {
					vpid = VPIDStandard_1080_3Gb;
				}
			}
		} else if (aja::IsSDITwoWireIOSelection(io)) {
			if (is_rgb) {
				if (standard == NTV2_STANDARD_720) {
					if (trx == SDITransport::SDI3Ga)
						vpid = VPIDStandard_720_3Ga;
					else if (trx == SDITransport::SDI3Gb)
						vpid = VPIDStandard_720_3Gb;
				} else if (IsStandard1080p(standard)) {
					if (trx == SDITransport::HDDualLink) {
						vpid = VPIDStandard_1080_DualLink;
					} else if (trx == SDITransport::SDI3Ga)
						vpid = VPIDStandard_1080_Dual_3Ga;
					else if (trx == SDITransport::SDI3Gb)
						vpid = VPIDStandard_1080_Dual_3Gb;
				}
			} else {
				if (IsStandard1080p(standard) &&
				    trx == SDITransport::HDDualLink) {
					vpid = VPIDStandard_1080_DualLink;
				}
			}
		}
	} else if (rd == RasterDefinition::UHD_4K) {
		if (aja::IsSDIOneWireIOSelection(io)) {
			if (is_rgb) {
				if (trx == SDITransport::SDI6G) {
					vpid = VPIDStandard_2160_DualLink;
				} else if (trx == SDITransport::SDI12G) {
					vpid = VPIDStandard_2160_DualLink_12Gb;
				}
			} else {
				// YCbCr
				if (trx == SDITransport::SDI6G) {
					vpid = VPIDStandard_2160_Single_6Gb;
				} else if (trx == SDITransport::SDI12G) {
					vpid = VPIDStandard_2160_Single_12Gb;
				} else {
					vpid = VPIDStandard_2160_Single_6Gb; // fallback
				}
			}
		} else if (aja::IsSDITwoWireIOSelection(io)) {
			if (is_rgb) {
			} else {
				// YCbCr
				if (t4k == SDITransport4K::Squares) {
					vpid = VPIDStandard_1080;
				} else if (t4k ==
					   SDITransport4K::TwoSampleInterleave) {
					if (is_hfr &&
					    trx == SDITransport::SDI3Ga) {
						vpid = VPIDStandard_2160_QuadLink_3Ga;
					} else if (is_hfr &&
						   trx == SDITransport::SDI3Gb) {
						vpid = VPIDStandard_2160_QuadDualLink_3Gb;
					} else {
						vpid = VPIDStandard_2160_DualLink;
					}
				}
			}
		} else if (aja::IsSDIFourWireIOSelection(io)) {
			if (is_rgb) {
				if (t4k == SDITransport4K::Squares) {
					if (trx == SDITransport::SDI3Ga) {
						vpid = VPIDStandard_1080_3Ga;
					} else if (trx ==
						   SDITransport::SDI3Gb) {
						vpid = VPIDStandard_1080_DualLink_3Gb;
					}
				}
			} else {
				// YCbCr
				if (t4k == SDITransport4K::Squares) {
					if (trx == SDITransport::SDI3Ga) {
						vpid = VPIDStandard_1080_3Ga;
					} else if (trx ==
						   SDITransport::SDI3Gb) {
						vpid = VPIDStandard_1080_DualLink_3Gb;
					} else {
						vpid = VPIDStandard_1080;
					}
				} else if (t4k ==
					   SDITransport4K::TwoSampleInterleave) {
					if (trx == SDITransport::SDI3Ga) {
						vpid = VPIDStandard_2160_QuadLink_3Ga;
					} else if (trx ==
						   SDITransport::SDI3Gb) {
						vpid = VPIDStandard_2160_QuadDualLink_3Gb;
					}
				}
			}
		}
	}
	return vpid;
}

std::vector<NTV2DeviceID> MultiViewCards()
{
	return {DEVICE_ID_IOX3};
}

} // namespace aja
