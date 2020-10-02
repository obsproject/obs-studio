#pragma once

#include <QObject>
#include <QWizard>
#include <QSharedPointer>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

class StartPage;
class SelectionPage;
class LoadingPage;
class ErrorPage;

/*
	** The pre-stream wizard is a workflow focused on delivering encoder settings
	** specfic for the streaming destination before going Live. 
	** There is a launch context to know if launched from settings, otherwise 
	** we'll later add a wizard page and button for going live after new settings.
	*/

class PreStreamWizard : public QWizard {
	Q_OBJECT

public:
	PreStreamWizard(Destination dest, LaunchContext launchContext,
			QSharedPointer<EncoderSettingsRequest> currentSettings,
			QWidget *parent = nullptr);

	int nextId() const override;

private:
	// Pages
	StartPage *startPage_;
	LoadingPage *loadingPage_;
	SelectionPage *selectionPage_;
	ErrorPage *errorPage_;

	// External State
	Destination destination_;
	LaunchContext launchContext_;
	QSharedPointer<EncoderSettingsRequest> currentSettings_;
	QSharedPointer<SettingsMap> newSettingsMap_;
	bool sendToErrorPage_ = false;

	void requestSettings();

	// Wizard uses custom button layouts to manage user flow & aborts.
	enum ButtonLayout {
		NextStep,
		CancelOnly,
		FinishOnly,
		CommitStep,
	};
	void setButtons(ButtonLayout layout);

signals:
	// User left the wizard with intention to continue streaming
	void userSkippedWizard(void);

	// User canceled, also canceling intent to stream.
	void userAbortedStream(void);

	// User ready to start stream
	// If newSettings is not null, they should be applied before stream
	// If newSettings is null, user or wizard did not opt to apply changes.
	void
	startStreamWithSettings(QSharedPointer<SettingsMap> newSettingsOrNull);

	// Apply settings, don't start stream. e.g., is configuring from settings
	void applySettings(QSharedPointer<SettingsMap> newSettings);

	// All configuration providers must call one of these exclusively when done to
	// continue the wizard. This is the contract any servicer's
	// configuration provider must fulfill
public slots:
	// On success, returns a EncoderSettingsResponse object
	void providerEncoderSettings(QSharedPointer<SettingsMap> response);
	// Title and description to show the user.
	void providerError(QString title, QString description);

private slots:
	void onPageChanged(int id);
	void onUserSelectResolution(QSize newSize);
};

} //namespace StreamWizard
