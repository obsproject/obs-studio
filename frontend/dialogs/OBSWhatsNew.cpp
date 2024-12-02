#include "OBSWhatsNew.hpp"

#include <widgets/OBSBasic.hpp>

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
extern QCef *cef;
#endif

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "moc_OBSWhatsNew.cpp"

OBSWhatsNew::OBSWhatsNew(QWidget *parent, const std::string &url) : QDialog(parent)
{
#ifdef BROWSER_AVAILABLE
	if (!cef) {
		return;
	}

	setWindowTitle("What's New");
	setAttribute(Qt::WA_DeleteOnClose, true);
	resize(700, 600);

	Qt::WindowFlags flags = windowFlags();
	Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags & (~helpFlag));

	OBSBasic::InitBrowserPanelSafeBlock();

	cefWidget = cef->create_widget(nullptr, url);
	if (!cefWidget) {
		return;
	}

	connect(cefWidget, &QCefWidget::titleChanged, this, &OBSWhatsNew::setWindowTitle);

	QPushButton *close = new QPushButton(QTStr("Close"));
	connect(close, &QAbstractButton::clicked, this, &QDialog::accept);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	QVBoxLayout *topLayout = new QVBoxLayout(this);
	topLayout->addWidget(cefWidget);
	topLayout->addLayout(bottomLayout);

	show();
#else
	UNUSED_PARAMETER(url);
#endif
}

OBSWhatsNew::~OBSWhatsNew() {}

void OBSWhatsNew::reject()
{
#ifdef BROWSER_AVAILABLE
	delete cefWidget;
#endif
	QDialog::reject();
}

void OBSWhatsNew::accept()
{
#ifdef BROWSER_AVAILABLE
	delete cefWidget;
#endif
	QDialog::accept();
}
