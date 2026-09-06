#pragma once

#include <QWidget>
#include <QMenu>

class OBSMenu : public QMenu {
	Q_OBJECT

	QWidget* parent{nullptr};

public:
	OBSMenu(QWidget *parent);

	OBSMenu(QWidget *parent, const bool &deleteOnClose);

	OBSMenu(const QString &title, QWidget *parent, const bool &deleteOnClose);

	void showEvent(QShowEvent *event) override;

	void popupMenu();
};
