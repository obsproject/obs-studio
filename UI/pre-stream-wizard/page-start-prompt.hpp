#pragma once

#include <QWizardPage>

#include "pre-stream-current-settings.hpp"

class QRadioButton;
class QSignalMapper;
class QSize;

namespace StreamWizard {
// Prompt if the user wants to verify their settings or close.
class StartPage : public QWizardPage {
	Q_OBJECT

public:
	StartPage(Destination dest, LaunchContext launchContext,
		  QSize videoSize, QWidget *parent = nullptr);

signals:
	// emitted selected resolution from start page radio buttons
	void userSelectedResolution(QSize newVideoSize);

private:
	// Init information
	Destination destination_;
	LaunchContext launchContext_;
	QSize startVideoSize_;

	// Selected settings
	QSize selectedVideoSize_;
	QRadioButton *res720Button_;
	QRadioButton *res1080Button_;
	QRadioButton *resCurrentButton_;

	void createLayout();
	void connectRadioButtons();

private slots:
	void didPushOpenWebsiteHelp();

}; // class StartPage
} // namespace StreamWizard
