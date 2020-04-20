/******************************************************************************
    Copyright (C) 2020 by Hugh Bailey <obs.jim@gmail.com>

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

#include "helpwidget-gamecapturefailed.hpp"
#include "obs-app.hpp"
#include "platform.hpp"
#include <QDesktopServices>
#include <QUrlQuery>
#include <QMessageBox>

using namespace std;

GameCaptureFailedHelpWidget::GameCaptureFailedHelpWidget(QWidget *parent)
	: GameCaptureFailedHelpWidget(parent,
				      "GameCaptureFailedHelpWidget.HelpText")
{
}

GameCaptureFailedHelpWidget::GameCaptureFailedHelpWidget(QWidget *parent,
							 const char *helpText)
	: HelpWidget(parent, helpText,
		     "GameCaptureFailedHelpWidget.FixButtonText")
{
#ifndef _WIN32
	ui->fixButton->hide();
#endif
}

void GameCaptureFailedHelpWidget::HelpButtonClicked()
{
	OpenManual();
}

void GameCaptureFailedHelpWidget::FixButtonClicked()
{
	TryFixAndDisplayResult();
}

bool GameCaptureFailedHelpWidget::ShouldDisplayHelp(int error)
{
	int gpuCount = 0;

	obs_enter_graphics();

	gs_enum_adapters(
		[](void *param, const char *name, uint32_t id) {
			int *counter = static_cast<int *>(param);
			(*counter)++;

			return true;
		},
		&gpuCount);

	obs_leave_graphics();

	return gpuCount > 1;
}

void GameCaptureFailedHelpWidget::OpenManual()
{
	QDesktopServices::openUrl(QUrl(
		"https://obsproject.com/forum/threads/laptop-black-screen-when-capturing-read-here-first.5965/",
		QUrl::TolerantMode));
}

void GameCaptureFailedHelpWidget::TryFixAndDisplayResult()
{
	const bool success = TryFix();

	if (success) {
		QMessageBox::information(
			this,
			QTStr("GameCaptureFailedHelpWidget.FixedSuccessTitle"),
			QTStr("GameCaptureFailedHelpWidget.FixedSuccessMessage"),
			QMessageBox::Ok);

		ui->fixButton->setEnabled(false);
	} else {
		const bool openManual =
			QMessageBox::warning(
				this,
				QTStr("GameCaptureFailedHelpWidget.FixedFailedTitle"),
				QTStr("GameCaptureFailedHelpWidget.FixedFailedMessage"),
				QMessageBox::Yes | QMessageBox::No) ==
			QMessageBox::Yes;

		if (openManual) {
			OpenManual();
		}
	}
}

bool GameCaptureFailedHelpWidget::TryFix()
{
#ifdef _WIN32
	return ToggleOBSGpuPowerProfile();
#else
	return false;
#endif
}
