#include "VRClient.h"
#include <QJsonDocument>
#include <QDebug>
#include "../../shared/vr-protocol/VRProtocol.h" // Relative path until include dirs are fixed

VRClient &VRClient::instance()
{
	static VRClient s_instance;
	return s_instance;
}

VRClient::VRClient()
{
	// Optional: Connect signals for socket
	connect(&m_socket, &QLocalSocket::connected, this, []() { qDebug() << "Connected to VR Backend"; });
	connect(&m_socket, &QLocalSocket::errorOccurred, this,
		[](QLocalSocket::LocalSocketError error) { qDebug() << "VR Client Socket Error:" << error; });
}

VRClient::~VRClient()
{
	if (m_socket.isOpen())
		m_socket.close();
}

bool VRClient::connectToBackend()
{
	if (m_socket.state() == QLocalSocket::ConnectedState)
		return true;

	// QLocalSocket on Unix typically expects the name to be the path
	m_socket.connectToServer(QString::fromUtf8(VRProtocol::SOCKET_PATH));
	return m_socket.waitForConnected(1000);
}

void VRClient::sendCommand(const QString &command, const QJsonObject &args)
{
	if (m_socket.state() != QLocalSocket::ConnectedState) {
		if (!connectToBackend()) {
			qWarning() << "Cannot send command, backend not connected";
			return;
		}
	}

	// Construct Payload
	QJsonObject payload = args;
	payload["command"] = command;

	QByteArray jsonData = QJsonDocument(payload).toJson(QJsonDocument::Compact);

	// Construct Header
	VRProtocol::Header header;
	header.type = VRProtocol::MessageType::Command;
	header.payloadSize = jsonData.size();

	// Send Header
	m_socket.write((const char *)&header, sizeof(header));

	// Send Payload
	m_socket.write(jsonData);
	m_socket.flush();
}

void VRClient::updateNode(const QString &uuid, const QJsonObject &data)
{
	QJsonObject args;
	args["id"] = uuid;
	args["updates"] = data;
	sendCommand("updateNode", args);
}
