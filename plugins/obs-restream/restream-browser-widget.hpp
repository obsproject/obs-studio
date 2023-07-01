#pragma once

#include <QWidget>

#include <util/util.hpp>

class RestreamBrowserWidget : public QWidget {
	Q_OBJECT

	std::string url;

	QScopedPointer<QWidget> cefWidget;

public:
	RestreamBrowserWidget(const QString &url);
	~RestreamBrowserWidget() {}

	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
private slots:
	void UpdateCefWidget();
};
