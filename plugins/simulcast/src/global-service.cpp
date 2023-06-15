#include "global-service.h"

#include <QThread>

static class GlobalServiceImpl : public GlobalService {
public:
	bool RunInUIThread(std::function<void()> task) override
	{
		if (uiThread_ == nullptr)
			return false;
		QMetaObject::invokeMethod(
			uiThread_, [func = std::move(task)]() { func(); });
		return true;
	}

	void setCurrentThreadAsDefault() override
	{
		uiThread_ = QThread::currentThread();
	}

	QThread *uiThread_ = nullptr;
	std::function<void()> saveConfig_;
} s_service;

GlobalService &GetGlobalService()
{
	return s_service;
}
