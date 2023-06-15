#include "simulcast-dock-widget.h"

#include "simulcast-output.h"

#include <QMainWindow>
#include <QDockWidget>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPushButton>
#include <QScrollArea>
#include <QGridLayout>
#include <QEvent>
#include <QThread>
#include <QLineEdit>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QAction>

#include <obs.h>
#include <obs-module.h>
#include <util/config-file.h>
#include <obs-frontend-api.h>

#define ConfigSection "simulcast"

SimulcastDockWidget::SimulcastDockWidget(QWidget *parent)
{
	QGridLayout *dockLayout = new QGridLayout(this);
	dockLayout->setAlignment(Qt::AlignmentFlag::AlignTop);

	// start all, stop all
	auto buttonContainer = new QWidget(this);
	auto buttonLayout = new QVBoxLayout();
	auto streamingButton = new QPushButton(
		obs_module_text("Btn.StartStreaming"), buttonContainer);
	buttonLayout->addWidget(streamingButton);
	buttonContainer->setLayout(buttonLayout);
	dockLayout->addWidget(buttonContainer);

	QObject::connect(
		streamingButton, &QPushButton::clicked,
		[this, streamingButton]() {
			if (this->Output().IsStreaming()) {
				this->Output().StopStreaming();
				streamingButton->setText(
					obs_module_text("Btn.StartStreaming"));
			} else {
				this->Output().StartStreaming();
				streamingButton->setText(
					obs_module_text("Btn.StopStreaming"));
			}
		});

	// load config
	LoadConfig();

	setLayout(dockLayout);

	resize(200, 400);
}

void SimulcastDockWidget::SaveConfig() {}

void SimulcastDockWidget::LoadConfig()
{
	auto profile_config = obs_frontend_get_profile_config();
}
