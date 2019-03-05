#include "window-dock.hpp"
#include "obs-app.hpp"

#include <QMessageBox>
#include <QCheckBox>

void OBSDock::closeEvent(QCloseEvent *event)
{
	bool warned = config_get_bool(App()->GlobalConfig(), "General",
			"WarnedAboutClosingDocks");
	if (!warned) {
		QMessageBox *msgbox = new QMessageBox(this);
		msgbox->setWindowTitle(QTStr("DockCloseWarning.Title"));
		msgbox->setText(QTStr("DockCloseWarning.Text"));
		msgbox->setIcon(QMessageBox::Icon::Information);
		msgbox->addButton(QMessageBox::Ok);

		QCheckBox *cb = new QCheckBox(QTStr("DoNotShowAgain"));
		msgbox->setCheckBox(cb);

		msgbox->exec();

		config_set_bool(App()->GlobalConfig(), "General",
				"WarnedAboutClosingDocks", cb->isChecked());
		config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	}

	QDockWidget::closeEvent(event);
}
