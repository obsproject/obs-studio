#include "main-toolbar.hpp"
#include "window-basic-main.hpp"
#include <QMenu>

MainToolBar::MainToolBar(QWidget *parent)
	: QWidget(parent), ui(new Ui::MainToolBar)
{
	main = OBSBasic::Get();

	ui->setupUi(this);

	QMenu *copyMenu = new QMenu(this);
	copyMenu->addAction(main->ui->actionCopySource);
	copyMenu->addSeparator();
	copyMenu->addAction(main->ui->actionCopyTransform);
	copyMenu->addAction(main->ui->actionCopyFilters);

	ui->copyButton->setMenu(copyMenu);

	QMenu *pasteMenu = new QMenu(this);
	pasteMenu->addAction(main->ui->actionPasteRef);
	pasteMenu->addAction(main->ui->actionPasteDup);
	pasteMenu->addSeparator();
	pasteMenu->addAction(main->ui->actionPasteTransform);
	pasteMenu->addAction(main->ui->actionPasteFilters);

	ui->pasteButton->setMenu(pasteMenu);

	QMenu *screenshotMenu = new QMenu(this);
	screenshotMenu->addAction(QTStr("Screenshot.Preview"), main,
				  SLOT(ScreenshotScene()));
	screenshotMenu->addAction(QTStr("Screenshot.StudioProgram"), main,
				  SLOT(ScreenshotProgram()));
	screenshotMenu->addAction(QTStr("Screenshot.Scene"), main,
				  SLOT(ScreenshotScene()));
	screenshotMenu->addAction(QTStr("Screenshot.Source"), main,
				  SLOT(ScreenshotSelectedSource()));

	ui->screenshotButton->setMenu(screenshotMenu);

	connect(&main->undo_s, SIGNAL(changed()), this, SLOT(UpdateUndoRedo()));
	UpdateUndoRedo();
}

void MainToolBar::UpdateUndoRedo()
{
	ui->undoButton->setEnabled(main->ui->actionMainUndo->isEnabled());
	ui->redoButton->setEnabled(main->ui->actionMainRedo->isEnabled());
	ui->undoButton->setToolTip(main->ui->actionMainUndo->text());
	ui->redoButton->setToolTip(main->ui->actionMainRedo->text());
}

void MainToolBar::on_undoButton_clicked()
{
	main->on_actionMainUndo_triggered();
}

void MainToolBar::on_redoButton_clicked()
{
	main->on_actionMainRedo_triggered();
}

void MainToolBar::on_copyButton_clicked()
{
	main->on_actionCopySource_triggered();
}

void MainToolBar::on_pasteButton_clicked()
{
	main->on_actionPasteRef_triggered();
}

void MainToolBar::on_screenshotButton_clicked()
{
	main->ScreenshotScene();
}

void MainToolBar::on_previewVis_clicked()
{
	main->TogglePreview();
}

void MainToolBar::on_previewLock_clicked()
{
	main->on_actionLockPreview_triggered();
}

void MainToolBar::on_settingsButton_clicked()
{
	main->on_settingsButton_clicked();
}
