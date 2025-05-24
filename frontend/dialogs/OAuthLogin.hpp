#pragma once

#include <QDialog>

class QCefWidget;

class OAuthLogin : public QDialog {
	Q_OBJECT

	QCefWidget *cefWidget = nullptr;
	QString code;
	bool get_token = false;
	bool fail = false;

public:
	OAuthLogin(QWidget *parent, const std::string &url, bool token);
	~OAuthLogin();

	inline QString GetCode() const { return code; }
	inline bool LoadFail() const { return fail; }

	virtual int exec() override;
	virtual void reject() override;
	virtual void accept() override;

public slots:
	void urlChanged(const QString &url);
};
