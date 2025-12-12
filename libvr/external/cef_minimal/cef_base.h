#pragma once

// Minimal Stub for CEF Headers needed for compilation

#include <string>
#include <vector>

namespace Cef {
    typedef std::string String;
}

class CefBaseRefCounted {
public:
    virtual void AddRef() const = 0;
    virtual void Release() const = 0;
    virtual bool HasOneRef() const = 0;
protected:
    virtual ~CefBaseRefCounted() {}
};

template <class T>
class CefRefPtr {
public:
    T* p;
    CefRefPtr() : p(nullptr) {}
    CefRefPtr(T* p) : p(p) { if(p) p->AddRef(); }
    ~CefRefPtr() { if(p) p->Release(); }
    T* get() const { return p; }
    operator T*() const { return p; }
    T* operator->() const { return p; }
};

class CefApp : public CefBaseRefCounted {};
class CefClient : public CefBaseRefCounted {};

class CefBrowser : public CefBaseRefCounted {
public:
    virtual void Reload() = 0;
    virtual void LoadURL(const Cef::String& url) = 0;
};

class CefBrowserHost : public CefBaseRefCounted {
public:
    virtual void SendMouseMoveEvent(const void* event, bool mouseLeave) = 0;
    virtual void SendMouseClickEvent(const void* event, int type, bool mouseUp, int clickCount) = 0;
    virtual void WasResized() = 0;
    virtual void CloseBrowser(bool force_close) = 0;
};

// ... Add more valid Stubs as needed during implementation
