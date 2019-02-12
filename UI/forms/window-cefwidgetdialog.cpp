#include "window-cefwidgetdialog.hpp"
#include "ui_CefWidgetDialog.h"

CefWidgetDialog::CefWidgetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CefWidgetDialog)
{
    ui->setupUi(this);
}

CefWidgetDialog::~CefWidgetDialog()
{
    delete ui;
}

bool CefWidgetDialog::AskForName(QWidget *parent, const QString &title,
		const QString &text, string &str, const QString &placeHolder)
{
	CefWidgetDialog dialog(parent);
	dialog.setWindowTitle(title);
	dialog.ui->label->setText(text);
	dialog.ui->urlText->setMaxLength(INT_MAX);
	dialog.ui->urlText->setText(placeHolder);
	dialog.ui->urlText->selectAll();

	bool accepted = (dialog.exec() == DialogCode::Accepted);
	if (accepted)
		str = QT_TO_UTF8(dialog.ui->userText->text());

	return accepted;
}