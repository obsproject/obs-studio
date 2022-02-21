#pragma once

#include <string>
#include <list>
#include <X11/Xlib.h>

#define blog(level, msg, ...) blog(level, "xcompcap: " msg, ##__VA_ARGS__)

class XErrorLock {
public:
	char curErrorText[200];
	XErrorHandler prevHandler;

	XErrorLock(const XErrorLock &) = delete;
	XErrorLock &operator=(const XErrorLock &) = delete;

	XErrorLock();
	~XErrorLock();

	bool gotError();
	std::string getErrorText();
	void resetError();
};

class XCompcapMain;

namespace XCompcap {
Display *disp();
void cleanupDisplay();

int getRootWindowScreen(Window root);
std::string getWindowAtom(Window win, const char *atom);
bool ewmhIsSupported();
std::list<Window> getTopLevelWindows();

inline std::string getWindowName(Window win)
{
	return getWindowAtom(win, "_NET_WM_NAME");
}

inline std::string getWindowClass(Window win)
{
	return getWindowAtom(win, "WM_CLASS");
}

void registerSource(XCompcapMain *source, Window win);
void unregisterSource(XCompcapMain *source);
void processEvents();
bool sourceWasReconfigured(XCompcapMain *source);
}

// Defer implementation from http://the-witness.net/news/2012/11/scopeexit-in-c11/
template<typename F> struct ScopeExit {
	ScopeExit(F f) : f(f) {}
	~ScopeExit() { f(); }
	F f;
};

template<typename F> ScopeExit<F> MakeScopeExit(F f)
{
	return ScopeExit<F>(f);
};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1##arg2
#define DEFER(code)                                \
	auto STRING_JOIN2(scope_exit_, __LINE__) = \
		MakeScopeExit([=]() { code; })
