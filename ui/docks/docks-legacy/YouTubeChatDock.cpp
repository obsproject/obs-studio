#include "YouTubeChatDock.hpp"

#include <docks/YouTubeAppDock.hpp>
#include <utility/YoutubeApiWrappers.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QHBoxLayout>
#include <QPushButton>

#include "moc_YouTubeChatDock.cpp"

#ifdef BROWSER_AVAILABLE
void YoutubeChatDock::YoutubeCookieCheck()
{
	QPointer<YoutubeChatDock> this_ = this;
	auto cb = [this_](bool currentlyLoggedIn) {
		bool previouslyLoggedIn = this_->isLoggedIn;
		this_->isLoggedIn = currentlyLoggedIn;
		bool loginStateChanged = (currentlyLoggedIn && !previouslyLoggedIn) ||
					 (!currentlyLoggedIn && previouslyLoggedIn);
		if (loginStateChanged) {
			OBSBasic *main = OBSBasic::Get();
			if (main->GetYouTubeAppDock() != nullptr) {
				QMetaObject::invokeMethod(main->GetYouTubeAppDock(), "SettingsUpdated",
							  Qt::QueuedConnection, Q_ARG(bool, !currentlyLoggedIn));
			}
		}
	};
	if (panel_cookies) {
		panel_cookies->CheckForCookie("https://www.youtube.com", "SID", cb);
	}
}
#endif
