#pragma once

namespace VstProxy
{
	enum WM_USER_MSG
	{
		// Start at index user + 5 because some plugins were causing issues when sending invalid
		// messages to the main window
		WM_USER_SET_TITLE = WM_USER + 5,
		WM_USER_SHOW,
		WM_USER_CLOSE,
		WM_USER_CREATE_WINDOW,
		WM_USER_SETCHUNK,
		WM_USER_HIDE
	};
}
