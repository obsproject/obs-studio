#pragma once

#include <functional>

class GlobalService {
public:
	~GlobalService() {}
	virtual bool RunInUIThread(std::function<void()> task) = 0;
	virtual void setCurrentThreadAsDefault() = 0;
};

GlobalService &GetGlobalService();
