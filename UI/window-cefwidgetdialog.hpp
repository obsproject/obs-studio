#pragma once

#include "obs-app.hpp"
#include <QDialog>
#include <QDockWidget>
#include <QInputDialog>
#include <QFormLayout>
#include <QUrl>
#include <QList>
#include <QPointer>
#include <QWidget>
#include <string>
#include <memory>

#include "ui_CefWidgetDialog.h"

class CefWidgetDialog : public QDialog
{
	Q_OBJECT
private:
	friend class QListWidget;
	std::unique_ptr<Ui::CefWidgetDialog> ui;
private slots:
	void on_addButton_clicked(bool clicked);
	void on_removeButton_clicked(bool clicked);
	void on_editButton_clicked(bool clicked);
public:
	explicit CefWidgetDialog(QList<QPointer<QDockWidget>> *inCefWidgets,
			QWidget *parent = 0);
	~CefWidgetDialog();

	static bool OpenDialog(QWidget *parent, const QString title,
			QList<QPointer<QDockWidget>> *customCefWidgets,
			QStringList *outUrls, QStringList *outTitles);
};
