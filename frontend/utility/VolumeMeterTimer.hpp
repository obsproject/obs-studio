#pragma once

#include <QTimer>

class VolumeMeter;

class VolumeMeterTimer : public QTimer {
	Q_OBJECT

public:
	inline VolumeMeterTimer() : QTimer() {}

	void AddVolControl(VolumeMeter *meter);
	void RemoveVolControl(VolumeMeter *meter);

protected:
	void timerEvent(QTimerEvent *event) override;
	QList<VolumeMeter *> volumeMeters;
};
