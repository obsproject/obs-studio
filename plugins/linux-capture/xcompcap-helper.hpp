#pragma once

#include <string>
#include <list>

#define blog(level, msg, ...) blog(level, "xcompcap: " msg, ##__VA_ARGS__)


class PLock
{
	pthread_mutex_t *m;
	bool islock;

	public:
	PLock(const PLock&) = delete;
	PLock& operator=(const PLock&) = delete;

	PLock(pthread_mutex_t* mtx, bool trylock = false);

	~PLock();

	bool isLocked();

	void unlock();
	void lock();
};

class XErrorLock
{
	bool islock;
	bool goterr;
	XErrorHandler prevhandler;

	public:
	XErrorLock(const XErrorLock&) = delete;
	XErrorLock& operator=(const XErrorLock&) = delete;

	XErrorLock();
	~XErrorLock();

	bool isLocked();

	void unlock();
	void lock();

	bool gotError();
	std::string getErrorText();
	void resetError();
};

class ObsGsContextHolder
{
	public:
	ObsGsContextHolder(const ObsGsContextHolder&) = delete;
	ObsGsContextHolder& operator=(const ObsGsContextHolder&) = delete;

	ObsGsContextHolder();
	~ObsGsContextHolder();
};

namespace XCompcap
{
	Display* disp();
	void cleanupDisplay();

	std::string getWindowCommand(Window win);
	int getRootWindowScreen(Window root);
	std::string getWindowName(Window win);
	int getWindowPid(Window win);
	bool ewmhIsSupported();
	std::list<Window> getTopLevelWindows();
	std::list<Window> getAllWindows();

	void processEvents();
	bool windowWasReconfigured(Window win);
}
