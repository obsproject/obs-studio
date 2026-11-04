#pragma once

#include <obs-frontend-api.h>
#include <obs.hpp>
#include <util/platform.h>

#include <QFrame>
#include <QPointer>
#include <QTimer>

class QLabel;
class QGridLayout;

class OBSBasicStats : public QFrame {
	Q_OBJECT

	QLabel *fps = nullptr;
	QLabel *cpuUsage = nullptr;
	QLabel *hddSpace = nullptr;
	QLabel *recordTimeLeft = nullptr;
	QLabel *memUsage = nullptr;

	QLabel *renderTime = nullptr;
	QLabel *skippedFrames = nullptr;
	QLabel *missedFrames = nullptr;
	QLabel *previewMode = nullptr;
	QLabel *previewRefresh = nullptr;
	QLabel *previewProduced = nullptr;
	QLabel *previewSubmitted = nullptr;
	QLabel *previewSubmittedCount = nullptr;
	QLabel *previewQueued = nullptr;
	QLabel *previewDiscarded = nullptr;
	QLabel *previewNotReady = nullptr;
	QLabel *previewNoCandidate = nullptr;
	QLabel *previewGpuPending = nullptr;
	QLabel *previewBusy = nullptr;
	QLabel *previewGap = nullptr;
	QLabel *previewAge = nullptr;
	QLabel *previewPresentTime = nullptr;
	QLabel *previewLockWait = nullptr;
	QLabel *previewDrawAttempts = nullptr;
	obs_display_stats previousPreviewStats = {};
	bool previewStatsValid = false;

	QGridLayout *outputLayout = nullptr;

	os_cpu_usage_info_t *cpu_info = nullptr;

	QTimer timer;
	QTimer recTimeLeft;
	uint64_t num_bytes = 0;
	std::vector<long double> bitrates;

	struct OutputLabels {
		QPointer<QLabel> name;
		QPointer<QLabel> status;
		QPointer<QLabel> droppedFrames;
		QPointer<QLabel> megabytesSent;
		QPointer<QLabel> bitrate;

		uint64_t lastBytesSent = 0;
		uint64_t lastBytesSentTime = 0;

		int first_total = 0;
		int first_dropped = 0;

		void Update(obs_output_t *output, bool rec);
		void Reset(obs_output_t *output);

		long double kbps = 0.0l;
	};

	QList<OutputLabels> outputLabels;

	void AddOutputLabels(QString name);
	void Update();
	void UpdatePreview();

	virtual void closeEvent(QCloseEvent *event) override;

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	OBSBasicStats(QWidget *parent = nullptr, bool closable = true);
	~OBSBasicStats();

	static void InitializeValues();

	void StartRecTimeLeft();
	void ResetRecTimeLeft();

private:
	QPointer<QObject> shortcutFilter;

private slots:
	void RecordingTimeLeft();

public slots:
	void Reset();

protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;
};
