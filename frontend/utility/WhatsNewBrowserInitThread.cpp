#include "WhatsNewBrowserInitThread.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

#include "moc_WhatsNewBrowserInitThread.cpp"

#ifdef BROWSER_AVAILABLE
struct QCef;
extern QCef *cef;
#endif

void WhatsNewBrowserInitThread::run()
{
#ifdef BROWSER_AVAILABLE
	cef->wait_for_browser_init();
#endif
	emit Result(url);
}
