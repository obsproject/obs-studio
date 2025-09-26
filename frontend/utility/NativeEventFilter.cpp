

#include <utility/NativeEventFilter.hpp>

#include <widgets/OBSBasic.hpp>

#ifdef _WIN32
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

namespace OBS {

bool NativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
	if (eventType == "windows_generic_MSG") {
#ifdef _WIN32
		MSG *msg = static_cast<MSG *>(message);

		OBSBasic *main = OBSBasic::Get();
		if (!main) {
			return false;
		}

		switch (msg->message) {
		case WM_QUERYENDSESSION:
			main->saveAll();
			if (msg->lParam == ENDSESSION_CRITICAL) {
				break;
			}

			if (main->shouldPromptForClose()) {
				*result = FALSE;
				return true;
			}

			return false;
		case WM_ENDSESSION:
			if (msg->wParam == TRUE) {
				// Session is ending, start closing the main window now with no checks or prompts.
				main->closeWindow();
			} else {
				/* Session is no longer ending. If OBS is still open, odds are it is what held
				 * up the session end due to it's higher than default priority. We call the
				 * close method to trigger the confirmation window flow. We do this after the fact
				 * to avoid blocking the main window event loop prior to this message.
				 * Otherwise OBS is already gone and invoking this does nothing */
				main->close();
			}

			return true;
		}
#else
		UNUSED_PARAMETER(message);
		UNUSED_PARAMETER(result);
#endif
	}

	return false;
}
} // namespace OBS
