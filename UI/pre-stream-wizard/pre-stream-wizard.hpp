#pragma once

#include <QObject>
#include <QWizard>
#include <QSharedPointer>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

class PreStreamWizard : public QWizard {
	Q_OBJECT

public:
	PreStreamWizard(Destination dest, LaunchContext launchContext,
			QSharedPointer<EncoderSettingsRequest> currentSettings,
			QWidget *parent = nullptr);
	~PreStreamWizard();

private:
	// External State
	Destination destination_;
	LaunchContext launchContext_;
	QSharedPointer<EncoderSettingsRequest> currentSettings_;
	QSize userSelectedNewRes_;

signals:
	// User left the wizard with intention to continue streaming
	void userSkippedWizard(void);

	// User canceled, also canceling intent to stream.
	void userAbortedStream(void);

	// User ready to start stream
	// If newSettings is not null, they should be applied before stream
	// If newSettings is null, user or wizard did not opt to apply changes.
	void startStreamWithSettings(
		QSharedPointer<EncoderSettingsResponse> newSettingsOrNull);

	// Apply settings, don't start stream. e.g., is configuring from settings
	void applySettings(QSharedPointer<EncoderSettingsResponse> newSettings);

private slots:
	void onPageChanged(int id);
	void onUserSelectResolution(QSize newSize);
};

} //namespace StreamWizard
