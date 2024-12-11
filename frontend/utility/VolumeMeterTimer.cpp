#include "VolumeMeterTimer.hpp"

#include <widgets/VolumeMeter.hpp>

#include "moc_VolumeMeterTimer.cpp"

void VolumeMeterTimer::AddVolControl(VolumeMeter *meter)
{
	volumeMeters.push_back(meter);
}

void VolumeMeterTimer::RemoveVolControl(VolumeMeter *meter)
{
	volumeMeters.removeOne(meter);
}

void VolumeMeterTimer::timerEvent(QTimerEvent *)
{
	for (VolumeMeter *meter : volumeMeters) {
		if (meter->needLayoutChange()) {
			// Tell paintEvent to update layout and paint everything
			meter->update();
		} else {
			// Tell paintEvent to paint only the bars
			meter->update(meter->getBarRect());
		}
	}
}
