#pragma once

#include "ui_OBSBasicTransitions.h"

#include <QFrame>

#include <memory>

class OBSBasic;

class OBSBasicTransitions : public QFrame {
	Q_OBJECT

	std::unique_ptr<Ui::OBSBasicTransitions> ui;

	bool transitionConfigurable;

private slots:
	void transitionAdded(const QString &name, const QString &uuid);
	void transitionRenamed(const QString &uuid, const QString &newName);
	void transitionRemoved(const QString &uuid);
	void transitionsCleared();

	void currentTransitionChanged(const QString &uuid, bool fixed, bool configurable);

	void durationChanged(const int &duration);

	void transitionsControlChanged(bool locked);

public:
	OBSBasicTransitions(OBSBasic *main);
	inline ~OBSBasicTransitions() {}

signals:
	void transitionChanged(const QString &uuid);
	void transitionDurationChanged(int duration);

	void addTransitionClicked();
	void removeCurrentTransitionClicked();
	void currentTransitionPropertiesMenuClicked();
};
