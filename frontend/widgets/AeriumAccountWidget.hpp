#pragma once

#include <QWidget>
#include <QByteArray>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QPointer>
#include <QThreadPool>
#include <QTimer>

#include <utility/AeriumCredentials.hpp>

#include <functional>

class QPushButton;
class QLabel;
class QMenu;
class QNetworkAccessManager;
class QDialog;

class AeriumAccountWidget : public QWidget {
	Q_OBJECT

public:
	explicit AeriumAccountWidget(QWidget *parent = nullptr, QNetworkAccessManager *network = nullptr,
				     QWidget *loginHost = nullptr);
	~AeriumAccountWidget() override;

protected:
	void resizeEvent(QResizeEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	using ResponseHandler = std::function<void(int, const QJsonObject &, int)>;
	using CredentialHandler = std::function<void(const AeriumCredentials::Result &)>;

	QPushButton *accountButton;
	QPushButton *cancelButton;
	QLabel *statusLabel;
	QMenu *accountMenu;
	QNetworkAccessManager *network;
	QPointer<QWidget> loginHost;
	QPointer<QDialog> loginWindow;
	QPushButton *loginButton = nullptr;
	QPushButton *loginCancelButton = nullptr;
	QLabel *loginStatus = nullptr;
	QThreadPool credentialWorker;
	QTimer pollTimer;
	QTimer sessionTimer;
	QElapsedTimer loginClock;
	QByteArray verifier;
	QByteArray accessToken;
	QByteArray refreshToken;
	QString requestId;
	QString username;
	quint64 generation = 0;
	bool busy = false;
	bool remembered = false;
	int pollDelay = 5000;

	void Request(const QString &path, const QJsonObject &body, const QByteArray &bearer, ResponseHandler callback);
	void CredentialTask(std::function<AeriumCredentials::Result()> work, CredentialHandler callback);
	void SetStatus(const QString &text);
	void SetBusy(const QString &text, bool cancellable);
	void CreateLoginWindow();
	void UpdateLoginWindow();
	void RestoreSession();
	void StartLogin();
	void PollLogin();
	void AcceptSession(const QJsonObject &tokens);
	void ValidateSession();
	void RefreshSession();
	void ShowAccount();
	void CancelLogin();
	void SignOut();
	void ForgetSession(const QString &message);
	void Revoke(const QByteArray &token);
	void RetryLater(const QString &message, int seconds);
};
