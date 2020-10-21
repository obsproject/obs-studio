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
using SettingsMap = QMap<QString, QPair<QVariant, bool>>;
// where String = map key from kSettingsResponseKeys, and
// where QVariant is the Settings value, and
// where bool is if the user wants to apply the settings
// Keys for new settings QMap
struct SettingsResponseKeys {
	QString videoWidth;            //int
	QString videoHeight;           //int
	QString framerate;             // double (FPS)
	QString videoBitrate;          //int
	QString protocol;              // string
	QString videoCodec;            // string
	QString h264Profile;           // string ("High")
	QString h264Level;             // string ("4.1")
	QString gopSizeInFrames;       // int
	QString gopClosed;             // bool
	QString gopBFrames;            // int
	QString gopRefFrames;          // int
	QString streamRateControlMode; // string "CBR"
	QString streamBufferSize;      // int (5000 kb)
};

// Defined in pre-stream-current-settings.cpp
extern SettingsResponseKeys kSettingsResponseKeys;

} // namespace StreamWizard
