#include "page-loading.hpp"

#include <QSpacerItem>
#include <QVBoxLayout>
#include <QTimer>

#include "obs-app.hpp"

namespace StreamWizard {

LoadingPage::LoadingPage(QWidget *parent) : QWizardPage(parent)
{
	setTitle(QTStr("PreLiveWizard.Loading.Title"));
	setCommitPage(true);

	labels_.append(QTStr("Loading"));
	labels_.append(QTStr("Loading") + ".");
	labels_.append(QTStr("Loading") + "." + ".");
	labels_.append(QTStr("Loading") + "." + "." + ".");

	QVBoxLayout *mainlayout = new QVBoxLayout(this);
	loadingLabel_ = new QLabel(this);
	loadingLabel_->setText(labels_.at(0));
	mainlayout->addWidget(loadingLabel_);
	setLayout(mainlayout);
}

void LoadingPage::initializePage()
{
	if (timer_ == nullptr) {
		timer_ = new QTimer(this);
		connect(timer_, &QTimer::timeout, this, &LoadingPage::tick);
	}
	timer_->setSingleShot(false);
	timer_->start(500);
}
void LoadingPage::cleanupPage()
{
	timer_->stop();
}

void LoadingPage::tick()
{
	count_++;
	if (count_ > 3)
		count_ = 0;

	loadingLabel_->setText(labels_.at(count_));
}

} // namespace StreamWizard
