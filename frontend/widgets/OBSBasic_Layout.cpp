/******************************************************************************
    Copyright (C) 2026 by Kryptographer

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
******************************************************************************/

#include "OBSBasic.hpp"

#include <utility/PreviewLayout.hpp>

#include <qt-wrappers.hpp>

#include <QLayout>
#include <QMenu>
#include <QResizeEvent>
#include <QTimer>
#include <QToolBar>

/* Keeps the preview usable on any screen size.
 *
 * Qt restores dock sizes in absolute pixels, so a layout saved on a large
 * monitor squeezes the preview when it is restored on a smaller one. The main
 * window therefore remembers the preview as a share of the window, re-applies
 * that share after the saved layout is restored, and trims the docks whenever
 * a resize or screen change would leave the preview below a minimum share.
 * The View > Preview Size menu lets the user grow or shrink the preview
 * directly; dragging the dock separators keeps working as before. */

void OBSBasic::SetupPreviewLayoutControls()
{
	QMenu *previewSizeMenu = new QMenu(QTStr("Basic.MainMenu.View.PreviewSize"), this);
	previewSizeMenu->setObjectName("previewSizeMenu");

	previewSizeMenu->addAction(QTStr("Basic.MainMenu.View.PreviewSize.Larger"), this,
				   [this] { AdjustPreviewShare(PreviewLayout::kStep); });
	previewSizeMenu->addAction(QTStr("Basic.MainMenu.View.PreviewSize.Smaller"), this,
				   [this] { AdjustPreviewShare(-PreviewLayout::kStep); });
	previewSizeMenu->addSeparator();

	auto addPreset = [this, previewSizeMenu](const char *text, double heightShare, double widthShare) {
		previewSizeMenu->addAction(QTStr(text), this, [this, heightShare, widthShare] {
			SetPreviewShare(heightShare, widthShare);
		});
	};
	addPreset("Basic.MainMenu.View.PreviewSize.Compact", PreviewLayout::kCompactHeightShare,
		  PreviewLayout::kCompactWidthShare);
	addPreset("Basic.MainMenu.View.PreviewSize.Balanced", PreviewLayout::kBalancedHeightShare,
		  PreviewLayout::kBalancedWidthShare);
	addPreset("Basic.MainMenu.View.PreviewSize.Large", PreviewLayout::kLargeHeightShare,
		  PreviewLayout::kLargeWidthShare);

	/* The studio header can be hidden to give the preview its full height.
	 * QMainWindow::saveState remembers toolbar visibility, so the choice
	 * persists together with the dock layout. */
	if (QToolBar *header = findChild<QToolBar *>("novaHeader")) {
		previewSizeMenu->addSeparator();
		QAction *headerAction = header->toggleViewAction();
		headerAction->setText(QTStr("Nova.Header"));
		previewSizeMenu->addAction(headerAction);
	}

	/* Place the submenu right after "Fullscreen Interface". */
	QAction *before = nullptr;
	const QList<QAction *> actions = ui->viewMenu->actions();
	const int index = actions.indexOf(ui->actionFullscreenInterface);
	if (index >= 0 && index + 1 < actions.size()) {
		before = actions[index + 1];
	}
	ui->viewMenu->insertMenu(before, previewSizeMenu);

	/* Moving to another monitor can change the window size and DPI. */
	if (QWindow *window = windowHandle()) {
		connect(window, &QWindow::screenChanged, this,
			[this](QScreen *) { SchedulePreviewLayoutUpdate(false); });
	}
}

void OBSBasic::resizeEvent(QResizeEvent *event)
{
	OBSMainWindow::resizeEvent(event);
	SchedulePreviewLayoutUpdate(false);
}

void OBSBasic::SchedulePreviewLayoutUpdate(bool restoreSavedShare)
{
	if (restoreSavedShare) {
		restorePreviewShare = true;
	}
	if (previewLayoutPending) {
		return;
	}

	previewLayoutPending = true;
	/* Deferred so the dock layout has settled before it is measured. */
	QTimer::singleShot(0, this, [this] {
		previewLayoutPending = false;
		UpdatePreviewLayout();
	});
}

void OBSBasic::UpdatePreviewLayout()
{
	if (!isVisible() || isMinimized() || !centralWidget()) {
		return;
	}

	/* Restored dock sizes are applied lazily; measure the settled layout. */
	if (QLayout *mainLayout = layout()) {
		mainLayout->activate();
	}

	if (restorePreviewShare) {
		restorePreviewShare = false;
		if (savedPreviewHeightShare > 0.0 && savedPreviewWidthShare > 0.0) {
			PreviewLayout::ApplyShare(this, savedPreviewHeightShare, savedPreviewWidthShare);
			/* Measure again once Qt has applied the new dock sizes. */
			SchedulePreviewLayoutUpdate(false);
			return;
		}
	}

	if (PreviewLayout::EnsureMinimumShare(this)) {
		blog(LOG_DEBUG, "Preview was squeezed to %.0f%% x %.0f%% of the window; trimmed docks",
		     PreviewLayout::HeightShare(this) * 100.0, PreviewLayout::WidthShare(this) * 100.0);
	}
}

void OBSBasic::SetPreviewShare(double heightShare, double widthShare)
{
	if (!centralWidget()) {
		return;
	}

	heightShare = PreviewLayout::Clamp(heightShare);
	widthShare = PreviewLayout::Clamp(widthShare);
	PreviewLayout::ApplyShare(this, heightShare, widthShare);
	savedPreviewHeightShare = heightShare;
	savedPreviewWidthShare = widthShare;
}

void OBSBasic::AdjustPreviewShare(double delta)
{
	const double heightShare = PreviewLayout::HeightShare(this);
	const double widthShare = PreviewLayout::WidthShare(this);
	if (heightShare <= 0.0 || widthShare <= 0.0) {
		return;
	}

	SetPreviewShare(heightShare + delta, widthShare + delta);
}

double OBSBasic::PreviewHeightShare() const
{
	return PreviewLayout::HeightShare(this);
}

double OBSBasic::PreviewWidthShare() const
{
	return PreviewLayout::WidthShare(this);
}

void OBSBasic::LoadPreviewShare()
{
	config_t *config = App()->GetUserConfig();
	savedPreviewHeightShare = 0.0;
	savedPreviewWidthShare = 0.0;

	if (config_has_user_value(config, "BasicWindow", "PreviewHeightShare") &&
	    config_has_user_value(config, "BasicWindow", "PreviewWidthShare")) {
		savedPreviewHeightShare = config_get_double(config, "BasicWindow", "PreviewHeightShare");
		savedPreviewWidthShare = config_get_double(config, "BasicWindow", "PreviewWidthShare");
	}
}

void OBSBasic::SavePreviewShare()
{
	if (!isVisible() || isMinimized() || !centralWidget()) {
		return;
	}

	const double heightShare = PreviewLayout::HeightShare(this);
	const double widthShare = PreviewLayout::WidthShare(this);
	if (heightShare <= 0.0 || widthShare <= 0.0) {
		return;
	}

	config_set_double(App()->GetUserConfig(), "BasicWindow", "PreviewHeightShare", heightShare);
	config_set_double(App()->GetUserConfig(), "BasicWindow", "PreviewWidthShare", widthShare);
}
