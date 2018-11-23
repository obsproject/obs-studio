#include "window-twitch-login.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qt-wrappers.hpp>
#include <obs-app.hpp>

#include <browser-panel.hpp>
extern CREATE_BROWSER_WIDGET_PROC create_browser_widget;

#define TWITCH_AUTH_URL \
	"https://obsproject.com/app-auth/twitch?action=redirect"

TwitchLogin::TwitchLogin(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle("Auth");
	resize(700, 700);

	cefWidget = create_browser_widget(nullptr, TWITCH_AUTH_URL);
	if (!cefWidget) {
		fail = true;
		return;
	}

	connect(cefWidget, SIGNAL(titleChanged(const QString &)),
			this, SLOT(setWindowTitle(const QString &)));
	connect(cefWidget, SIGNAL(urlChanged(const QString &)),
			this, SLOT(urlChanged(const QString &)));

	QPushButton *close = new QPushButton(QTStr("Cancel"));
	connect(close, &QAbstractButton::clicked,
			this, &QDialog::reject);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	QVBoxLayout *topLayout = new QVBoxLayout(this);
	topLayout->addWidget(cefWidget);
	topLayout->addLayout(bottomLayout);
}

TwitchLogin::~TwitchLogin()
{
	delete cefWidget;
}

void TwitchLogin::urlChanged(const QString &url)
{
	int token_idx = url.indexOf("access_token=");
	if (token_idx == -1)
		return;

	token_idx += 13;

	int next_idx = url.indexOf("&", token_idx);
	if (next_idx != -1)
		token = url.mid(token_idx, next_idx - token_idx);
	else
		token = url.right(url.size() - token_idx);

	accept();
}
