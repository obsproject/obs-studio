#pragma once

#include <obs.hpp>
#include <string>
#include <QThread>

class DeviceToolbarPropertiesThread : public QThread {
	Q_OBJECT

	OBSSource source;
	obs_properties_t *props;

	void run() override;

public:
	inline DeviceToolbarPropertiesThread(OBSSource source_)
		: source(source_)
	{
	}

	~DeviceToolbarPropertiesThread() override;

public slots:
	void Ready();
};
