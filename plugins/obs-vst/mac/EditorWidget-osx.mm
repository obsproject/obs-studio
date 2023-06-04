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
#import "../headers/EditorWidget.h"
#import <Cocoa/Cocoa.h>
#include <QLayout>
#include <QWindow>

#import "../headers/VSTPlugin.h"

void EditorWidget::buildEffectContainer(AEffect *effect)
{
    NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 300, 300)];
    cocoaViewContainer = QWidget::createWindowContainer(QWindow::fromWinId(WId(view)));
    cocoaViewContainer->move(0, 0);
    cocoaViewContainer->resize(300, 300);
    cocoaViewContainer->show();

    QGridLayout *hblParams = new QGridLayout();
    hblParams->setContentsMargins(0, 0, 0, 0);
    hblParams->setSpacing(0);
    hblParams->addWidget(cocoaViewContainer);

    VstRect *vstRect = nullptr;
    effect->dispatcher(effect, effEditGetRect, 0, 0, &vstRect, 0);
    if (vstRect) {
        NSRect frame = NSMakeRect(vstRect->left, vstRect->top, vstRect->right, vstRect->bottom);

        [view setFrame:frame];

        cocoaViewContainer->resize(vstRect->right - vstRect->left, vstRect->bottom - vstRect->top);

        setFixedSize(vstRect->right - vstRect->left, vstRect->bottom - vstRect->top);
    }

    effect->dispatcher(effect, effEditOpen, 0, 0, view, 0);

    setLayout(hblParams);
}

void EditorWidget::handleResizeRequest(int width, int height)
{
    setFixedSize(width, height);
}
