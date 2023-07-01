#include "restream-browser-widget.hpp"

#include <QVBoxLayout>

#include <obs-module.h>
#include <obs-frontend-api.h>

RestreamBrowserWidget::RestreamBrowserWidget(const QString &url_)
	: url(url_.toStdString()),
	  QWidget()
{
	setMinimumSize(200, 300);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	setLayout(layout);
}

void RestreamBrowserWidget::showEvent(QShowEvent *event)
{
	UpdateCefWidget();

	QWidget::showEvent(event);
}

void RestreamBrowserWidget::hideEvent(QHideEvent *event)
{
	cefWidget.reset(nullptr);
	QWidget::hideEvent(event);
}

void RestreamBrowserWidget::UpdateCefWidget()
{
	obs_frontend_browser_params params = {0};

	params.url = url.c_str();
	params.enable_cookie = true;

	cefWidget.reset((QWidget *)obs_frontend_get_browser_widget(&params));

	QVBoxLayout *layout = (QVBoxLayout *)this->layout();
	layout->addWidget(cefWidget.get());
	cefWidget->setParent(this);
}
