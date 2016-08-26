#include <QMenu>
#include <QKeyEvent>
#include <QMouseEvent>
#include "menu-button.hpp"

void MenuButton::keyPressEvent(QKeyEvent *event)
{
	if (menu()) {
		switch (event->key()) {
		case Qt::Key_Enter:
		case Qt::Key_Return:
			emit clicked();
			return;
		case Qt::Key_Down:
			showMenu();
			return;
		}
	}

	QPushButton::keyPressEvent(event);
}

void MenuButton::mousePressEvent(QMouseEvent *event)
{
	if (menu()) {
		if (width() - event->x() <= 15)
			showMenu();
		else
			setDown(true);
	} else {
		QPushButton::mousePressEvent(event);
	}
}
