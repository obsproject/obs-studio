#pragma once

#include <QAbstractNativeEventFilter>

namespace OBS {

class NativeEventFilter : public QAbstractNativeEventFilter {

public:
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result);
};
} // namespace OBS
