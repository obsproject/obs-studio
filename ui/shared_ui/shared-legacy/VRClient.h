#pragma once

#include <QObject>
#include <QJsonObject>
#include <QLocalSocket>
#include <memory>

class VRClient : public QObject
{
    Q_OBJECT
      public:
    static VRClient &instance();

    bool connectToBackend();
    void sendCommand(const QString &command, const QJsonObject &args);
    void updateNode(const QString &uuid, const QJsonObject &data);

      signals:
    void responseReceived(const QJsonObject &response);

      private:
    VRClient();
    ~VRClient();

    // We use QLocalSocket, but VRProtocol uses unix domain sockets.
    // QLocalSocket on Linux uses unix domain sockets.
    // However, the socket path in VRProtocol is "/tmp/vrobs.sock".
    QLocalSocket m_socket;
};
