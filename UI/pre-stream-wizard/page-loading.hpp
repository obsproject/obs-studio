#pragma once

#include <QWizardPage>
#include <QLabel>
#include <QString>
#include <QTimer>
#include <QStringList>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

// Page shown while a provider computes best settings.

class LoadingPage : public QWizardPage {
	Q_OBJECT

public:
	LoadingPage(QWidget *parent = nullptr);
	void initializePage() override;
	void cleanupPage() override;

private:
	QLabel *loadingLabel_;
	QTimer *timer_;
	QStringList labels_;
	int count_ = 0;

private slots:
	void tick();

}; // class LoadingPage

} // namespace StreamWizard
