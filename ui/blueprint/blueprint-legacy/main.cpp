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
#include "NodeRegistry.h"
#include "NodeItem.h"

#include "PropertiesPanel.h"
#include <QDockWidget>

class BlueprintWindow : public QMainWindow {
	Q_OBJECT // Needs Q_OBJECT for slots if used, but lambda is fine
public:
	BlueprintWindow()
	{
		setWindowTitle("VR Blueprint Editor");
		resize(1280, 800);

		// Main Graph View
		nodeGraph = new NodeGraph(this);
		setCentralWidget(nodeGraph);

		// Properties Panel Dock
		QDockWidget *dock = new QDockWidget("Spatial Properties", this);
		dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
		propertiesPanel = new PropertiesPanel(dock);
		dock->setWidget(propertiesPanel);
		addDockWidget(Qt::RightDockWidgetArea, dock);

		// Connect Selection
		connect(nodeGraph->scene(), &QGraphicsScene::selectionChanged, this, [this]() {
			if (!nodeGraph || !nodeGraph->scene())
				return;
			QList<QGraphicsItem *> selected = nodeGraph->scene()->selectedItems();
			if (selected.isEmpty()) {
				propertiesPanel->setNode(nullptr);
			} else {
				// Assuming single selection logic for properties
				NodeItem *node = dynamic_cast<NodeItem *>(selected.first());
				propertiesPanel->setNode(node);
			}
		});

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
			const auto *camDef = NodeRegistry::instance().getNodeByName("PTZ Camera");
			if (camDef)
				nodeGraph->addNode(*camDef, 0, 0);

			const auto *filterDef = NodeRegistry::instance().getNodeByName("Color Correction");
			if (filterDef)
				nodeGraph->addNode(*filterDef, 300, 50);

			const auto *outDef = NodeRegistry::instance().getNodeByName("RTMP Stream");
			if (outDef)
				nodeGraph->addNode(*outDef, 600, 0);

			// Close socket for now as we aren't maintaining persistent connection logic in this single-threaded stub
			::close(sock);
		} else {
			statusLabel->setText("Failed to connect to Engine daemon.");
		}
	}

private:
	QLabel *statusLabel;
	NodeGraph *nodeGraph;
	PropertiesPanel *propertiesPanel;
};
#include "main.moc"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	BlueprintWindow win;
	win.show();

	return app.exec();
}
