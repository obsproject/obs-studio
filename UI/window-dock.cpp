#include "window-dock.hpp"
#include "obs-app.hpp"

#include <QMessageBox>

void OBSDock::closeEvent(QCloseEvent *event)
{
	auto msgBox = [] ()
	{
		QMessageBox::information(App()->GetMainWindow(),
				QTStr("DockCloseWarning.Title"),
				QTStr("DockCloseWarning.Text"));
	};

	bool warned = config_get_bool(App()->GlobalConfig(), "General",
			"WarnedAboutClosingDocks");
	if (!warned) {
		QMetaObject::invokeMethod(App(), "Exec",
				Qt::QueuedConnection,
				Q_ARG(VoidFunc, msgBox));
		config_set_bool(App()->GlobalConfig(), "General",
				"WarnedAboutClosingDocks", true);
		config_save_safe(App()->GlobalConfig(), "tmp", nullptr);
	}

	QDockWidget::closeEvent(event);
}
