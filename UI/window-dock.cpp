#include "window-dock.hpp"
#include "obs-app.hpp"

#include <QMessageBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>

OBSDock::OBSDock(QWidget *parent) : QDockWidget(parent)
{
	QFrame *titleBar = new QFrame(this);

	titleBar->setObjectName("dockTitleBar");

	title = new QLabel(titleBar);
	title->setAlignment(Qt::AlignVCenter);

	layout = new QHBoxLayout(titleBar);
	layout->setContentsMargins(4, 2, 4, 2);
	layout->setSpacing(4);

	layout->addWidget(title);
	layout->addStretch();

	titleBar->setLayout(layout);

	setTitleBarWidget(titleBar);
}

void OBSDock::closeEvent(QCloseEvent *event)
{
	auto msgBox = []() {
		QMessageBox msgbox(App()->GetMainWindow());
		msgbox.setWindowTitle(QTStr("DockCloseWarning.Title"));
		msgbox.setText(QTStr("DockCloseWarning.Text"));
		msgbox.setIcon(QMessageBox::Icon::Information);
		msgbox.addButton(QMessageBox::Ok);

		QCheckBox *cb = new QCheckBox(QTStr("DoNotShowAgain"));
		msgbox.setCheckBox(cb);

		msgbox.exec();

		if (cb->isChecked()) {
			config_set_bool(App()->GlobalConfig(), "General",
					"WarnedAboutClosingDocks", true);
			config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
		}
	};

	bool warned = config_get_bool(App()->GlobalConfig(), "General",
				      "WarnedAboutClosingDocks");
	if (!warned) {
		QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection,
					  Q_ARG(VoidFunc, msgBox));
	}

	QDockWidget::closeEvent(event);
}

void OBSDock::ToggleFloating()
{
	if (isFloating())
		setFloating(false);
	else
		setFloating(true);
}

void OBSDock::ShowControlMenu()
{
	QMenu popup;

	QDockWidget::DockWidgetFeatures dockFeatures = features();

	if ((QDockWidget::DockWidgetClosable & dockFeatures) != 0)
		popup.addAction(QTStr("Hide"), this, SLOT(close()));

	popup.addAction(isFloating() ? QTStr("SetDocked")
				     : QTStr("SetFloating"),
			this, SLOT(ToggleFloating()));

	popup.exec(QCursor::pos());
}

void OBSDock::EnableControlMenu(bool lock)
{
	menuButton->setVisible(!lock);
}

void OBSDock::AddControlMenu()
{
	menuButton = new QPushButton(this);
	menuButton->setFlat(true);
	menuButton->setObjectName("dockTitleBarButton");
	menuButton->setFixedSize(16, 16);
	menuButton->setIconSize(QSize(12, 12));
	menuButton->setProperty("themeID", "dotsIcon");

	connect(menuButton, SIGNAL(clicked()), this, SLOT(ShowControlMenu()));

	layout->addWidget(menuButton);
}

void OBSDock::SetTitle(const QString &newTitle)
{
	title->setText(newTitle);
}

void OBSDock::AddButton(const QString &themeID, const QString &toolTip,
			const QObject *receiver, const char *slot)
{
	QPushButton *button = new QPushButton(this);
	button->setFlat(true);
	button->setObjectName("dockTitleBarButton");
	button->setFixedSize(16, 16);
	button->setIconSize(QSize(12, 12));
	button->setProperty("themeID", themeID);
	button->setToolTip(toolTip);

	connect(button, SIGNAL(clicked()), receiver, slot);

	layout->addWidget(button);
}

void OBSDock::AddSeparator()
{
	QFrame *line = new QFrame(this);
	line->setFrameShape(QFrame::VLine);
	line->setFixedWidth(1);
	layout->addWidget(line);
}
