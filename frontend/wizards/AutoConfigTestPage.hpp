#pragma once

#include <QPointer>
#include <QWizardPage>

#include <condition_variable>
#include <mutex>
#include <thread>

class QFormLayout;
class Ui_AutoConfigTestPage;

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

		inline ServerInfo(const char *name_, const char *address_) : name(name_), address(address_) {}
	};

	void GetServers(std::vector<ServerInfo> &servers);

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
