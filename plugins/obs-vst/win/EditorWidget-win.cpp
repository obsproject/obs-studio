/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include <QGridLayout>
#include "../headers/EditorWidget.h"

void EditorWidget::buildEffectContainer(AEffect *effect)
{
	WNDCLASSEXW wcex{sizeof(wcex)};

	wcex.lpfnWndProc = DefWindowProcW;
	wcex.hInstance = GetModuleHandleW(nullptr);
	wcex.lpszClassName = L"Minimal VST host - Guest VST Window Frame";
	RegisterClassExW(&wcex);

	const auto style = WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPEDWINDOW;
	windowHandle = CreateWindowW(wcex.lpszClassName, TEXT(""), style, 0, 0,
				     0, 0, nullptr, nullptr, nullptr, nullptr);

	// set pointer to vst effect for window long
	LONG_PTR wndPtr = (LONG_PTR)effect;
	SetWindowLongPtr(windowHandle, -21 /*GWLP_USERDATA*/, wndPtr);

	QWidget *widget = QWidget::createWindowContainer(
		QWindow::fromWinId((WId)windowHandle), nullptr);
	widget->move(0, 0);
	QGridLayout *layout = new QGridLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setLayout(layout);
	layout->addWidget(widget);

	effect->dispatcher(effect, effEditOpen, 0, 0, windowHandle, 0);

	VstRect *vstRect = nullptr;
	effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0);
	if (vstRect) {
		widget->resize(vstRect->right - vstRect->left,
			       vstRect->bottom - vstRect->top);
		resize(vstRect->right - vstRect->left,
		       vstRect->bottom - vstRect->top);
	} else {
		widget->resize(300, 300);
	}
}

void EditorWidget::handleResizeRequest(int, int)
{
	// Some plugins can't resize automatically (like SPAN by Voxengo),
	// so we must resize window manually

	// get pointer to vst effect from window long
	LONG_PTR wndPtr = (LONG_PTR)GetWindowLongPtrW(windowHandle,
						      -21 /*GWLP_USERDATA*/);
	AEffect *effect = (AEffect *)(wndPtr);
	VstRect *rec = nullptr;

	effect->dispatcher(effect, effEditGetRect, 0, 0, &rec, 0);

	if (rec) {
		resize(rec->right - rec->left, rec->bottom - rec->top);
	}
}
