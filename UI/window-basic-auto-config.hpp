#pragma once

#include <QWizard>
#include <QPointer>
#include <QFormLayout>
#include <QWizardPage>

#include <condition_variable>
#include <utility>
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <obs.hpp>

class Ui_AutoConfigStartPage;
class Ui_AutoConfigVideoPage;
class Ui_AutoConfigStreamPage;
class Ui_AutoConfigTestPage;

class AutoConfigStreamPage;
class OBSPropertiesView;

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

	static inline const char *GetEncoderId(Encoder enc);

	AutoConfigStreamPage *streamPage = nullptr;

	Quality recordingQuality = Quality::Stream;
	Encoder recordingEncoder = Encoder::Stream;
	Encoder streamingEncoder = Encoder::x264;
	Type type = Type::Streaming;
	FPSType fpsType = FPSType::PreferHighFPS;
	int idealBitrate = 2500;
	int baseResolutionCX = 1920;
	int baseResolutionCY = 1080;
	int idealResolutionCX = 1280;
	int idealResolutionCY = 720;
	int idealFPSNum = 60;
	int idealFPSDen = 1;
	std::string serviceName;
	std::string serverName;
	std::string server;
	OBSServiceAutoRelease service = nullptr;

	bool hardwareEncodingAvailable = false;
	bool nvencAvailable = false;
	bool qsvAvailable = false;
	bool vceAvailable = false;
	bool appleAvailable = false;

	int startingBitrate = 2500;
	bool bandwidthTest = false;
	bool preferHighFPS = false;
	bool preferHardware = false;
	int specificFPSNum = 0;
	int specificFPSDen = 0;

	void TestHardwareEncoding();

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

class AutoConfigStartPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	std::unique_ptr<Ui_AutoConfigStartPage> ui;

public:
	AutoConfigStartPage(QWidget *parent = nullptr);
	~AutoConfigStartPage();

	virtual int nextId() const override;

public slots:
	void on_prioritizeStreaming_clicked();
	void on_prioritizeRecording_clicked();
	void PrioritizeVCam();
};

class AutoConfigVideoPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	std::unique_ptr<Ui_AutoConfigVideoPage> ui;

public:
	AutoConfigVideoPage(QWidget *parent = nullptr);
	~AutoConfigVideoPage();

	virtual int nextId() const override;
	virtual bool validatePage() override;
};

class AutoConfigStreamPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	std::unique_ptr<Ui_AutoConfigStreamPage> ui;
	QString lastService;
	bool ready = false;

	OBSServiceAutoRelease tempService;
	OBSPropertiesView *streamServiceProps = nullptr;

	void LoadServices(bool showAll);

	OBSPropertiesView *CreateTempServicePropertyView(obs_data_t *settings);

private slots:
	void on_service_currentIndexChanged(int idx);

public:
	AutoConfigStreamPage(obs_service_t *service, QWidget *parent = nullptr);
	~AutoConfigStreamPage();

	virtual bool isComplete() const override;
	virtual int nextId() const override;
	virtual bool validatePage() override;

public slots:
	void ServiceChanged();
	void UpdateCompleted();
};

class AutoConfigTestPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	QPointer<QFormLayout> results;

	std::unique_ptr<Ui_AutoConfigTestPage> ui;
	std::thread testThread;
	std::condition_variable cv;
	std::mutex m;
	bool cancel = false;
	bool started = false;

	enum class Stage {
		Starting,
		BandwidthTest,
		StreamEncoder,
		RecordingEncoder,
		Finished,
	};

	Stage stage = Stage::Starting;
	bool softwareTested = false;

	void StartBandwidthStage();
	void StartStreamEncoderStage();
	void StartRecordingEncoderStage();

	void FindIdealHardwareResolution();
	bool TestSoftwareEncoding();

	void TestBandwidthThread();
	void TestStreamEncoderThread();
	void TestRecordingEncoderThread();

	void FinalizeResults();

	struct ServerInfo {
		std::string name;
		std::string address;
		int bitrate = 0;
		int ms = -1;

		inline ServerInfo() {}

		inline ServerInfo(const char *name_, const char *address_)
			: name(name_),
			  address(address_)
		{
		}

		inline ServerInfo(const char *address_)
			: name(address_),
			  address(address_)
		{
		}
	};

public:
	AutoConfigTestPage(QWidget *parent = nullptr);
	~AutoConfigTestPage();

	virtual void initializePage() override;
	virtual void cleanupPage() override;
	virtual bool isComplete() const override;
	virtual int nextId() const override;

public slots:
	void NextStage();
	void UpdateMessage(QString message);
	void Failure(QString message);
	void Progress(int percentage);
};
