#pragma once

#include <QDialog>

class QCefWidget;

class TwitchLogin : public QDialog {
	Q_OBJECT

	QCefWidget *cefWidget = nullptr;
	QString token;
	bool fail = false;

public:
	TwitchLogin(QWidget *parent);
	~TwitchLogin();

	inline QString GetToken() const {return token;}
	inline bool LoadFail() const {return fail;}

public slots:
	void urlChanged(const QString &url);
};
