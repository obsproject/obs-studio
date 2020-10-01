#include "page-error.hpp"

#include <QSpacerItem>
#include <QVBoxLayout>

#include "obs-app.hpp"

namespace StreamWizard {

ErrorPage::ErrorPage(QWidget *parent) : QWizardPage(parent)
{
	setTitle(QTStr("PreLiveWizard.Error.Title"));
	setSubTitle(QTStr("PreLiveWizard.Error.Subtitle"));
	setFinalPage(true);

	QVBoxLayout *mainlayout = new QVBoxLayout(this);
	titleLabel_ = new QLabel(this);
	titleLabel_->setStyleSheet("font-weight: bold;");
	descriptionLabel_ = new QLabel(this);
	mainlayout->addWidget(titleLabel_);
	mainlayout->addSpacerItem(new QSpacerItem(12, 12));
	mainlayout->addWidget(descriptionLabel_);
	setText(QTStr("PreLiveWizard.Error.Generic.Headline"),
		QTStr("PreLiveWizard.Error.Generic.BodyText"));
}

void ErrorPage::setText(QString title, QString description)
{
	titleLabel_->setText(title);
	descriptionLabel_->setText(description);
}

} // namespace StreamWizard
