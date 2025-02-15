#include "OBSExtraBrowsers.hpp"
#include "ui_OBSExtraBrowsers.h"

#include <utility/ExtraBrowsersDelegate.hpp>

#include "moc_OBSExtraBrowsers.cpp"

OBSExtraBrowsers::OBSExtraBrowsers(QWidget *parent) : QDialog(parent), ui(new Ui::OBSExtraBrowsers)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	model = new ExtraBrowsersModel(ui->table);

	ui->table->setModel(model);
	ui->table->setItemDelegateForColumn((int)Column::Title, new ExtraBrowsersDelegate(model));
	ui->table->setItemDelegateForColumn((int)Column::Url, new ExtraBrowsersDelegate(model));
	ui->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
	ui->table->horizontalHeader()->setSectionResizeMode((int)Column::Delete, QHeaderView::ResizeMode::Fixed);
	ui->table->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged);
}

OBSExtraBrowsers::~OBSExtraBrowsers() {}

void OBSExtraBrowsers::closeEvent(QCloseEvent *event)
{
	QDialog::closeEvent(event);
	model->Apply();
}

void OBSExtraBrowsers::on_apply_clicked()
{
	model->Apply();
}
