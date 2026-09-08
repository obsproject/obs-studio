#include <components/OBSMenu.hpp>

#include <Qt>
#include <QWindow>
#include <QCursor>
#include <QString>

OBSMenu::OBSMenu(QWidget* parent) : parent(parent) {
	connect(parent, &QObject::destroyed, this, &QMenu::deleteLater);
}

// This context menu will automatically delete itself on close.
// No need to manually delete this.
OBSMenu::OBSMenu(QWidget *parent, const bool &deleteOnClose) : parent(parent)
{
	if (deleteOnClose) {
		setAttribute(Qt::WA_DeleteOnClose);
	} else {
		connect(parent, &QObject::destroyed, this, &QMenu::deleteLater);
	}
}

OBSMenu::OBSMenu(const QString &title, QWidget *parent, const bool &deleteOnClose) : parent(parent)
{
	setTitle(title);

	if (deleteOnClose) {
		setAttribute(Qt::WA_DeleteOnClose);
	} else {
		connect(parent, &QObject::destroyed, this, &QMenu::deleteLater);
	}
}

void OBSMenu::showEvent([[maybe_unused]] QShowEvent *event)
{
	// TODO: implement this custom showEvent handler when hiding OBS Studio from
	//       capture is supported on bsd
}

void OBSMenu::popupMenu() {
	popup(QCursor::pos());
}
