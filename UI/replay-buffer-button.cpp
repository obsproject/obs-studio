#include "replay-buffer-button.hpp"
#include "window-basic-main.hpp"

void ReplayBufferButton::resizeEvent(QResizeEvent *event)
{
	OBSBasic *main = OBSBasic::Get();
	if (!main->saveReplay)
		return;

	QSize saveReplaySize = main->saveReplay->size();
	int height = main->ui->replayBufferButton->size().height();

	if (saveReplaySize.height() != height ||
	    saveReplaySize.width() != height) {
		main->saveReplay->setMinimumSize(height, height);
		main->saveReplay->setMaximumSize(height, height);
	}

	event->accept();
}
