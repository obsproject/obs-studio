#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>

#include "lineedit-autoresize.hpp"
#include "youtube-oauth.hpp"

class YoutubeChat : public QWidget {
	Q_OBJECT

	YouTubeApi::ServiceOAuth *apiYouTube;
	QScopedPointer<QWidget> cefWidget;

	LineEditAutoResize *lineEdit;
	QPushButton *sendButton;
	QHBoxLayout *chatLayout;

	std::string url;
	std::string chatId;

	QScopedPointer<QThread> sendingMessage;

public:
	YoutubeChat(YouTubeApi::ServiceOAuth *api);
	~YoutubeChat();
	void SetChatIds(const std::string &broadcastId,
			const std::string &chatId);
	void ResetIds();

	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
private slots:
	void SendChatMessage();
	void ShowErrorMessage(const QString &error);
	void UpdateCefWidget();
	void EnableChatInput(bool enable);
};
