#pragma once
#include "obs.hpp"

#include <QObject>

class AreaWindow : public QObject {
	Q_OBJECT

public:
	AreaWindow(QObject* parent = {})
		: QObject(parent) {}

public slots:
	void start(obs_sceneitem_t *item);
	void stop();
	void remove(obs_sceneitem_t *item);

	void update();
};
