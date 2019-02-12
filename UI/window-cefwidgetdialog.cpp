#include "window-cefwidgetdialog.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

#include "ui_CefWidgetDialog.h"

using namespace std;

CefWidgetDialog::CefWidgetDialog(QList<QPointer<QDockWidget>> *inCefWidgets,
		QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CefWidgetDialog)
{
	ui->setupUi(this);
	setMinimumWidth(400);
	QStringList strings;
	for (QPointer<QDockWidget> widget : *inCefWidgets) {
		QDockWidget *w = widget.data();
		QVariant v = w->property("url");
		if (w->windowTitle().length()) {
			QListWidgetItem *item = new QListWidgetItem();
			item->setData(Qt::UserRole, v.toString());
			item->setData(Qt::DisplayRole, w->windowTitle());
			ui->listWidget->addItem(item);
		}
	}
}

CefWidgetDialog::~CefWidgetDialog()
{

}

void CefWidgetDialog::on_addButton_clicked(bool clicked)
{
	if (clicked)
		return;

	QDialog dialog(this);
	QFormLayout form(&dialog);
	dialog.setMinimumWidth(400);
	dialog.setWindowTitle(QTStr("CefWidgetDialog.Add.Title"));

	QLineEdit *title = new QLineEdit(&dialog);
	form.addRow(new QLabel(QTStr("CefWidgetDialog.Add.Title.Label")), title);
	QLineEdit *url = new QLineEdit(&dialog);
	form.addRow(new QLabel(QTStr("CefWidgetDialog.Add.Url.Label")), url);

	QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		Qt::Horizontal, &dialog);

	QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
	QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
	form.addRow(&buttonBox);

	if (dialog.exec() == QDialog::Accepted) {
		QString u = url->text();
		QString t = title->text();
		if (t.isEmpty() || u.isEmpty())
			return;
		QList<QListWidgetItem *>items = ui->listWidget->findItems(t, Qt::MatchFlag::MatchCaseSensitive);
		if (items.count())
			return;
		QListWidgetItem *item = new QListWidgetItem();
		item->setData(Qt::DisplayRole, t);
		item->setData(Qt::UserRole, u);
		ui->listWidget->addItem(item);
	}
}

void CefWidgetDialog::on_removeButton_clicked(bool clicked)
{
	if (clicked)
		return;

	QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
	for (QListWidgetItem *item : items)
		delete item;
}

void CefWidgetDialog::on_editButton_clicked(bool clicked)
{
	if (clicked)
		return;

	QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
	bool success = false;
	for (QListWidgetItem *item : items) {
		QDialog dialog(this);
		QFormLayout form(&dialog);
		dialog.setMinimumWidth(400);

		QString dialogTitle = QTStr("CefWidgetDialog.Edit.Title").arg(
			item->data(Qt::DisplayRole).toString());
		dialog.setWindowTitle(dialogTitle);
		
		QLineEdit *title = new QLineEdit(&dialog);
		title->setText(item->data(Qt::DisplayRole).toString());
		form.addRow(new QLabel(QTStr("CefWidgetDialog.Edit.Title.Label")), title);
		QLineEdit *url = new QLineEdit(&dialog);
		url->setText(item->data(Qt::UserRole).toString());
		form.addRow(new QLabel(QTStr("CefWidgetDialog.Edit.Url.Label")), url);

		QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			Qt::Horizontal, &dialog);

		QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
		QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
		form.addRow(&buttonBox);

		if (dialog.exec() == QDialog::Accepted) {
			QString u = url->text();
			QString t = title->text();
			if (t.isEmpty() || u.isEmpty())
				return;

			if (t != item->data(Qt::DisplayRole).toString()) {
				QList<QListWidgetItem *>items = ui->listWidget->findItems(t, Qt::MatchFlag::MatchCaseSensitive);
				if (items.count())
					return;
				item->setData(Qt::DisplayRole, title->text());
				item->setData(Qt::UserRole, url->text());
			} else {
				item->setData(Qt::UserRole, url->text());
			}
		}
	}
}

bool CefWidgetDialog::OpenDialog(QWidget *parent, const QString title,
		QList<QPointer<QDockWidget>> *inCefWidgets, QStringList *outUrls,
		QStringList *outTitles = nullptr)
{
	CefWidgetDialog dialog(inCefWidgets, parent);
	dialog.setWindowTitle(title);

	bool success = dialog.exec() == DialogCode::Accepted;

	int count = dialog.ui->listWidget->count();
	if (outUrls) {
		outUrls->clear();
		outUrls->reserve(count);
	}
	if (outTitles) {
		outTitles->clear();
		outTitles->reserve(count);
	}
	for (int i = 0; i < count; i++) {
		QListWidgetItem *item = dialog.ui->listWidget->item(i);
		QVariant u = item->data(Qt::UserRole);
		blog(LOG_INFO, "%s", item->text().toStdString().c_str());
		blog(LOG_INFO, "%s", u.toString().toStdString().c_str());
		if (outUrls)
			outUrls->append(u.toString());
		if (outTitles)
			outTitles->append(item->text());
	}

	return success;
}

