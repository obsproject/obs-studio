#pragma once

#include <ajantv2/includes/ntv2enums.h>

#include <string>

using OutputXpt = NTV2OutputCrosspointID; // src
using InputXpt = NTV2InputCrosspointID;   // dest

// Firmware widget input socket connector
struct WidgetInputSocket {
	InputXpt id;
	NTV2WidgetID widget_id;
	NTV2Channel channel;
	const char *name;
	int32_t datastream_index;

	static bool Find(const std::string &route, NTV2Channel channel,
			 int32_t datastream, WidgetInputSocket &inp);
	static bool GetWidgetInputSocketByXpt(InputXpt id,
					      WidgetInputSocket &inp);
	static int32_t InputXptDatastreamIndex(InputXpt xpt);
	static NTV2Channel InputXptChannel(InputXpt xpt);
	static const char *InputXptName(InputXpt xpt);
};

// Firmware widget output socket connector
struct WidgetOutputSocket {
	OutputXpt id;
	NTV2WidgetID widget_id;
	NTV2Channel channel;
	const char *name;
	int32_t datastream_index;

	static bool Find(const std::string &route, NTV2Channel channel,
			 int32_t datastream, WidgetOutputSocket &out);
	static bool GetWidgetOutputSocketByXpt(OutputXpt id,
					       WidgetOutputSocket &out);
	static int32_t OutputXptDatastreamIndex(OutputXpt xpt);
	static NTV2Channel OutputXptChannel(OutputXpt xpt);
	static const char *OutputXptName(OutputXpt xpt);
};
