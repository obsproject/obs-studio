#pragma once

#include <QWizardPage>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

// Last Page
class CompletedPage : public QWizardPage {
	Q_OBJECT

public:
	CompletedPage(Destination destination, LaunchContext launchContext,
		      QWidget *parent = nullptr);

private:
	Destination destination_;
	LaunchContext launchContext_;

private slots:
	void didPushOpenWebsite();

}; // class CompletedPage

} // namespace StreamWizard
