#pragma once

#include <string>
#include <math.h>
#include <QString>
#include <QPair>
#include <QVariant>
#include <QMap>

namespace StreamWizard {

enum class VideoType {
	live, // Streaming
	vod,  // Video On Demand, recording uploads
};

enum class StreamProtocol {
	rtmps,
};

enum class VideoCodec {
	h264,
};

enum class AudioCodec {
	aac,
};

/* There are two launch contexts for starting the wizard
	- PreStream: the wizard is triggered between pressing Start Streaming and a
	stream. So the user wizard should indicate when the encoder is ready to stream
	but also allow the user to abort. 

	- Settings: User start config workflow from the settings page or toolbar. 
		In this case, the wizard should not end with the stream starting but may end
		wutg saving the settings if given signal 
*/
enum class LaunchContext {
	PreStream,
	Settings,
};

/* 
	To make the wizard expandable we can have multiple destinations. 
	In the case Facebook, it will use Facebook's no-auth encoder API.
*/
enum class Destination {
	Facebook,
};

// Data to send to encoder config API
struct EncoderSettingsRequest {
	//// Stream
	StreamProtocol protocol; // Expandable but only supports RTMPS for now
	VideoType videoType;     // LIVE or VOD (but always live for OBS)

	///// Video Settings
	int videoWidth;
	int videoHeight;
	double framerate; // in frames per second
	int videoBitrate; // in kbps e.g., 4000kbps
	VideoCodec videoCodec;

	///// Audio Settings
	int audioChannels;
	int audioSamplerate; // in Hz, e.g., 48000Hz
	int audioBitrate;    // in kbps e.g., 128kbps
	AudioCodec audioCodec;

	// If the user picked a resolution in the wizard
	// Holds QSize or null
	QVariant userSelectedResolution;
};

// Map for the repsonse passed to UI and settings is:
using SettingsMap = QMap<const char *, QPair<QVariant, bool>>;
// where String = map key from kSettingsResponseKeys, and
// where QVariant is the Settings value, and
// where bool is if the user wants to apply the settings
// Keys for new settings QMap
static const struct {
	const char *videoWidth;            //int
	const char *videoHeight;           //int
	const char *framerate;             // double (FPS)
	const char *videoBitrate;          //int
	const char *protocol;              // string
	const char *videoCodec;            // string
	const char *h264Profile;           // string ("High")
	const char *h264Level;             // string ("4.1")
	const char *gopSizeInFrames;       // int
	const char *gopType;               // string ("fixed")
	const char *gopClosed;             // bool
	const char *gopBFrames;            // int
	const char *gopRefFrames;          // int
	const char *streamRateControlMode; // string "CBR"
	const char *streamBufferSize;      // int (5000 kb)
} SettingsResponseKeys = {
	"videoWidth",       "videoHeight",
	"framerate",        "videoBitrate",
	"protocol",         "videoCodec",
	"h264Profile",      "h264Level",
	"gopSizeInFrames",  "gopType",
	"gopClosed",        "gopBFrames",
	"gopRefFrames",     "streamRateControlMode",
	"streamBufferSize",
};
}
