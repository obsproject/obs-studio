#pragma once

#include <QPointer>
#include <QDialog>
#include <string>

class QCefWidget;

class OBSWhatsNew : public QDialog {
	Q_OBJECT

	QCefWidget *cefWidget = nullptr;

public:
	OBSWhatsNew(QWidget *parent, const std::string &url);
	~OBSWhatsNew();

	virtual void reject() override;
	virtual void accept() override;
};
