#pragma once

#include <QWizardPage>
#include <QString>
#include <QLabel>

#include "pre-stream-current-settings.hpp"

namespace StreamWizard {

// Page shown if there's an error emitted from provider.

class ErrorPage : public QWizardPage {
	Q_OBJECT

public:
	ErrorPage(QWidget *parent = nullptr);

	void setText(QString title, QString description);

private:
	QString title_;
	QString description_;
	QLabel *titleLabel_;
	QLabel *descriptionLabel_;
}; // class ErrorPage

} // namespace StreamWizard
