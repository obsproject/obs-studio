#include "OAuthLogin.hpp"

#include <widgets/OBSBasic.hpp>

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif
#include <ui-config.h>

#include "moc_OAuthLogin.cpp"

#ifdef BROWSER_AVAILABLE
extern QCef *cef;
extern QCefCookieManager *panel_cookies;
#endif

OAuthLogin::OAuthLogin(QWidget *parent, const std::string &url, bool token) : QDialog(parent), get_token(token)
{
#ifdef BROWSER_AVAILABLE
	if (!cef) {
		return;
	}

	setWindowTitle("Auth");
	setMinimumSize(400, 400);
	resize(700, 700);

	Qt::WindowFlags flags = windowFlags();
	Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags & (~helpFlag));

	OBSBasic::InitBrowserPanelSafeBlock();

	cefWidget = cef->create_widget(nullptr, url, panel_cookies);
	if (!cefWidget) {
		fail = true;
		return;
	}

	connect(cefWidget, &QCefWidget::titleChanged, this, &OAuthLogin::setWindowTitle);
	connect(cefWidget, &QCefWidget::urlChanged, this, &OAuthLogin::urlChanged);

	QPushButton *close = new QPushButton(QTStr("Cancel"));
	connect(close, &QAbstractButton::clicked, this, &QDialog::reject);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	QVBoxLayout *topLayout = new QVBoxLayout(this);
	topLayout->addWidget(cefWidget);
	topLayout->addLayout(bottomLayout);
#else
	UNUSED_PARAMETER(url);
#endif
}

OAuthLogin::~OAuthLogin() {}

int OAuthLogin::exec()
{
#ifdef BROWSER_AVAILABLE
	if (cefWidget) {
		return QDialog::exec();
	}
#endif
	return QDialog::Rejected;
}

void OAuthLogin::reject()
{
#ifdef BROWSER_AVAILABLE
	delete cefWidget;
#endif
	QDialog::reject();
}

void OAuthLogin::accept()
{
#ifdef BROWSER_AVAILABLE
	delete cefWidget;
#endif
	QDialog::accept();
}

void OAuthLogin::urlChanged(const QString &url)
{
	std::string uri = get_token ? "access_token=" : "code=";
	int code_idx = url.indexOf(uri.c_str());
	if (code_idx == -1)
		return;

	if (!url.startsWith(OAUTH_BASE_URL))
		return;

	code_idx += (int)uri.size();

	int next_idx = url.indexOf("&", code_idx);
	if (next_idx != -1)
		code = url.mid(code_idx, next_idx - code_idx);
	else
		code = url.right(url.size() - code_idx);

	accept();
}
