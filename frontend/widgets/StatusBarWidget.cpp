#include "StatusBarWidget.hpp"
#include "ui_StatusBarWidget.h"
#include "moc_StatusBarWidget.cpp"

StatusBarWidget::StatusBarWidget(QWidget *parent) : QWidget(parent), ui(new Ui::StatusBarWidget)
{
	ui->setupUi(this);
}

StatusBarWidget::~StatusBarWidget() {}
