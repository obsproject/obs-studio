// #include <QIsGuiApplication> removed
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QQmlContext>
#include "NodeModel.h"
#include "ConnectionModel.h"
#include "viewmodels/PropertiesViewModel.h"
#include "viewmodels/SettingsViewModel.h"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	QQuickStyle::setStyle("Material");
	app.setOrganizationName("NeuralStudio");
	app.setOrganizationDomain("neuralstudio.ai");
	app.setApplicationName("VR Blueprint QML");

	QQmlApplicationEngine engine;

	NodeModel model;
	ConnectionModel connModel;
	PropertiesViewModel propModel;
	SettingsViewModel settingsModel;

	engine.rootContext()->setContextProperty("nodeGraphModel", &model);
	engine.rootContext()->setContextProperty("connectionModel", &connModel);
	engine.rootContext()->setContextProperty("propertiesViewModel", &propModel);
	engine.rootContext()->setContextProperty("settingsViewModel", &settingsModel);

	// Attempt to locate QML file relative to current dir, or fallback to source path hack for dev
	QString qmlPath = "frontend/blueprint/qml/main.qml";
	if (!QFile::exists(qmlPath)) {
		// Try assuming we are in build/frontend/blueprint
		qmlPath = "../../../frontend/blueprint/qml/main.qml";
	}

	if (!QFile::exists(qmlPath)) {
		qWarning() << "Could not locate main.qml at:" << qmlPath << ". Run from project root.";
		return -1;
	}

	qDebug() << "Loading QML from:" << QFileInfo(qmlPath).absoluteFilePath();

	const QUrl url = QUrl::fromLocalFile(QFileInfo(qmlPath).absoluteFilePath());

	QObject::connect(
		&engine, &QQmlApplicationEngine::objectCreated, &app,
		[url](QObject *obj, const QUrl &objUrl) {
			if (!obj && url == objUrl)
				QCoreApplication::exit(-1);
		},
		Qt::QueuedConnection);

	engine.load(url);

	return app.exec();
}
