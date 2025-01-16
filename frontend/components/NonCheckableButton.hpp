#pragma once

#include <QPushButton>
#include <QString>
#include <QWidget>

/* Button with its checked property not changed when clicked.
 * Meant to be used in situations where manually changing the property
 * is always preferred. */
class NonCheckableButton : public QPushButton {
	Q_OBJECT

	inline void nextCheckState() override {}

public:
	inline NonCheckableButton(QWidget *parent = nullptr) : QPushButton(parent) {}
	inline NonCheckableButton(const QString &text, QWidget *parent = nullptr) : QPushButton(text, parent) {}
};
