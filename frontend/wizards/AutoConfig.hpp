#pragma once

#include <QWizard>

class AutoConfigStreamPage;

class AutoConfig : public QWizard {
	Q_OBJECT

	friend class AutoConfigStartPage;
	friend class AutoConfigVideoPage;
	friend class AutoConfigStreamPage;
	friend class AutoConfigTestPage;

	enum class Type {
		Invalid,
		Streaming,
		Recording,
		VirtualCam,
	};

	enum class Service {
		Twitch,
		YouTube,
		AmazonIVS,
		Other,
	};

	enum class Encoder {
		x264,
		NVENC,
		QSV,
		AMD,
		Apple,
		Stream,
	};

	enum class Quality {
		Stream,
		High,
	};

	enum class FPSType : int {
		PreferHighFPS,
		PreferHighRes,
		UseCurrent,
		fps30,
		fps60,
	};

	struct StreamServer {
		std::string name;
		std::string address;
	};

	static inline const char *GetEncoderId(Encoder enc);

	AutoConfigStreamPage *streamPage = nullptr;

	Service service = Service::Other;
	Quality recordingQuality = Quality::Stream;
	Encoder recordingEncoder = Encoder::Stream;
	Encoder streamingEncoder = Encoder::x264;
	Type type = Type::Streaming;
	FPSType fpsType = FPSType::PreferHighFPS;
	int idealBitrate = 2500;
	struct {
		std::optional<int> targetBitrate;
		std::optional<int> bitrate;
		bool testSuccessful = false;
	} multitrackVideo;
	int baseResolutionCX = 1920;
	int baseResolutionCY = 1080;
	int idealResolutionCX = 1280;
	int idealResolutionCY = 720;
	int idealFPSNum = 60;
	int idealFPSDen = 1;
	std::string serviceName;
	std::string serverName;
	std::string server;
	std::vector<StreamServer> serviceConfigServers;
	std::string key;

	bool hardwareEncodingAvailable = false;
	bool nvencAvailable = false;
	bool qsvAvailable = false;
	bool vceAvailable = false;
	bool appleAvailable = false;

	int startingBitrate = 2500;
	bool customServer = false;
	bool bandwidthTest = false;
	bool testMultitrackVideo = false;
	bool testRegions = true;
	bool twitchAuto = false;
	bool amazonIVSAuto = false;
	bool regionUS = true;
	bool regionEU = true;
	bool regionAsia = true;
	bool regionOther = true;
	bool preferHighFPS = false;
	bool preferHardware = false;
	int specificFPSNum = 0;
	int specificFPSDen = 0;

	void TestHardwareEncoding();
	bool CanTestServer(const char *server);

	virtual void done(int result) override;

	void SaveStreamSettings();
	void SaveSettings();

public:
	AutoConfig(QWidget *parent);
	~AutoConfig();

	enum Page {
		StartPage,
		VideoPage,
		StreamPage,
		TestPage,
	};
};
