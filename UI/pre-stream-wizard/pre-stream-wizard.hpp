#pragma once

#include <QObject>
#include <QWizard>
#include <QSharedPointer>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

/* 
	To make the wizard expandable we can have multiple destinations. 
	In the case Facebook, it will use Facebook's no-auth encoder setup API.
	Other platforms can use APIs, static JSON files hosted, etc. 
*/
enum class Destination {
	Facebook,
};

/* There are two contexts for starting a stream 
	- PreStream: the wizard is triggered between pressing Start Streaming and a
	stream. So the user wizard should indicate when the encoder is ready to stream
	but also allow the user to abort. 

	- Settings: User start config workflow from the settings page or toolbar. 
		In this case, the wizard should not end with the stream starting but may end
		wutg saving the settings if given signal 
*/
enum class Context {
	PreStream,
	Settings,
};

class PreStreamWizard : public QWizard {
	Q_OBJECT

public:
	PreStreamWizard(Destination dest,
			QSharedPointer<EncoderSettingsRequest> currentSettings,
			QWidget *parent = nullptr);
	~PreStreamWizard();

private:
	QWizardPage *makeDebugPage();

	Destination destination_;
	QSharedPointer<EncoderSettingsRequest> currentSettings_;

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
};

} //namespace StreamWizard
