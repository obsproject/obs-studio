#include "record-button.hpp"
#include "window-basic-main.hpp"

void RecordButton::resizeEvent(QResizeEvent *event)
{
	OBSBasic *main = OBSBasic::Get();
	if (!main->pause && !main->splitFile)
		return;

	int height = main->ui->recordButton->size().height();

	if (main->pause) {
		QSize pauseSize = main->pause->size();
		if (pauseSize.height() != height ||
		    pauseSize.width() != height) {
			main->pause->setMinimumSize(height, height);
			main->pause->setMaximumSize(height, height);
		}
	}

	if (main->splitFile) {
		QSize splitFileSize = main->splitFile->size();
		if (splitFileSize.height() != height ||
		    splitFileSize.width() != height) {
			main->splitFile->setMinimumSize(height, height);
			main->splitFile->setMaximumSize(height, height);
		}
	}

	event->accept();
}

void ReplayBufferButton::resizeEvent(QResizeEvent *event)
{
	OBSBasic *main = OBSBasic::Get();
	if (!main->replay)
		return;

	QSize replaySize = main->replay->size();
	int height = main->ui->recordButton->size().height();

	if (replaySize.height() != height || replaySize.width() != height) {
		main->replay->setMinimumSize(height, height);
		main->replay->setMaximumSize(height, height);
	}

	event->accept();
}
