#pragma once

#include <QWidget>

#include <util/util.hpp>

class TwitchBrowserWidget : public QWidget {
	Q_OBJECT

public:
	enum Addon : int { NONE = 0, FFZ = 0x1, BTTV = 0x2, BOTH = FFZ | BTTV };

private:
	std::string url;
	std::string startupScriptBase;
	std::string startupScript;
	BPtr<char *> forcePopupUrl = nullptr;
	BPtr<char *> popupWhitelistUrl = nullptr;

	QScopedPointer<QWidget> cefWidget;

	Addon addon;

public:
	TwitchBrowserWidget(const Addon &addon, const QString &url,
			    const QString &startupScriptBase,
			    const QStringList &forcePopupUrl,
			    const QStringList &popupWhitelistUrl);
	~TwitchBrowserWidget() {}
	void SetAddon(const Addon &newAddon);

	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
private slots:
	void UpdateCefWidget();
};
