#pragma once

#include <QMenu>

class OBSContextMenu : public QMenu {
	Q_OBJECT

	QWindow* parentWindow;

public:
	OBSContextMenu(QWindow*);

	void showEvent(QShowEvent *event) override;
};
