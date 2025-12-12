#include <QStatusBar>
#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include "VRProtocol.h"
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "NodeGraph.h"

class BlueprintWindow : public QMainWindow {
public:
	BlueprintWindow()
	{
		setWindowTitle("VR Blueprint Editor");
		resize(1024, 768);

		// Main Graph View
		nodeGraph = new NodeGraph(this);
		setCentralWidget(nodeGraph);

		statusLabel = new QLabel("Disconnected", this);
		statusBar()->addWidget(statusLabel);

		ConnectToEngine();
	}

	void ConnectToEngine()
	{
		int sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sock < 0)
			return;

		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, VRProtocol::SOCKET_PATH, sizeof(addr.sun_path) - 1);

		if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
			statusLabel->setText("Connected to libvr Engine!");

			// On connect, request scenes (Mock send for now)
			// In a real Qt app, we'd use QSocketNotifier to read asynchronously.
			// For this skeleton, we'll just mock adding some nodes to show it works.
			nodeGraph->addNode("Camera Source", 0, 0);
			nodeGraph->addNode("Filter: Color", 300, 50);
			nodeGraph->addNode("Output: Stream", 600, 0);

			// Close socket for now as we aren't maintaining persistent connection logic in this single-threaded stub
			::close(sock);
		} else {
			statusLabel->setText("Failed to connect to Engine daemon.");
		}
	}

private:
	QLabel *statusLabel;
	NodeGraph *nodeGraph;
};

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	BlueprintWindow win;
	win.show();

	return app.exec();
}
