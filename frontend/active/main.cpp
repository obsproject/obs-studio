#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QLabel>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QLabel>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "MonitorGrid.h"
#include "AudioMixer.h"
#include "SceneButtons.h"
#include "VideoModule.h"
#include "CamModule.h"
#include "ThreeDModule.h"
#include "ExtraModules.h"
#include "MoreModules.h"

class ActiveWindow : public QMainWindow {
public:
	ActiveWindow(const QString &moduleName)
	{
		setWindowTitle("VR Active: " + moduleName);
		resize(400, 300); // Default small size for tools

		statusLabel = new QLabel("Disconnected", this);
		statusBar()->addWidget(statusLabel);

		QWidget *central = nullptr;
		if (moduleName == "monitor") {
			resize(1280, 720);
			MonitorGrid *grid = new MonitorGrid(this);
			central = grid;
			// Mock data
			grid->addMonitor("Scene 1");
			grid->addMonitor("Webcam");
		} else if (moduleName == "audio") {
			central = new AudioMixer(this);
		} else if (moduleName == "scenes") {
			central = new SceneButtons(this);
		} else if (moduleName == "video") {
			central = new VideoModule(this);
		} else if (moduleName == "cam") {
			central = new CamModule(this);
		} else if (moduleName == "3d") {
			central = new ThreeDModule(this);
		} else if (moduleName == "photo") {
			central = new PhotoModule(this);
		} else if (moduleName == "stream") {
			central = new StreamModule(this);
		} else if (moduleName == "script") {
			central = new ScriptModule(this);
		} else if (moduleName == "mixer") {
			central = new LayerMixerModule(this);
		} else if (moduleName == "macro") {
			central = new MacroModule(this);
		} else {
			central = new QLabel("Unknown Module: " + moduleName, this);
		}

		setCentralWidget(central);
		ConnectToEngine();
	}

	void ConnectToEngine()
	{
		const char *SOCKET_PATH = "/tmp/vrobs.sock";
		int sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sock < 0)
			return;

		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

		if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
			statusLabel->setText("Connected to libvr Engine!");
			::close(sock);
		} else {
			statusLabel->setText("Failed to connect to Engine daemon.");
		}
	}

private:
	QLabel *statusLabel;
};

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QString module = "monitor"; // Default
	QStringList args = app.arguments();
	for (const QString &arg : args) {
		if (arg.startsWith("--module=")) {
			module = arg.mid(9);
		}
	}

	ActiveWindow win(module);
	win.show();

	return app.exec();
}
