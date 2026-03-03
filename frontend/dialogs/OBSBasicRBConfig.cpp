#include "OBSBasicRBConfig.hpp"

#include <OBSApp.hpp>

#include "moc_OBSBasicRBConfig.cpp"

OBSBasicRBConfig::OBSBasicRBConfig(const ReplayBufferConfig &_config, bool _rbActive, QWidget *parent)
	: config(_config),
	  rbActive(_rbActive),
	  QDialog(parent),
	  ui(new Ui::OBSBasicRBConfig)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->outputType->addItem(QTStr("Basic.VCam.OutputType.Program"), (int)RBOutputProgramView);
	ui->outputType->addItem(QTStr("Basic.Scene"), (int)RBOutputSceneView);

	ui->outputType->setCurrentIndex(ui->outputType->findData((int)config.type));
	OutputTypeChanged();
	connect(ui->outputType, &QComboBox::currentIndexChanged, this, &OBSBasicRBConfig::OutputTypeChanged);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &OBSBasicRBConfig::UpdateConfig);
}

void OBSBasicRBConfig::OutputTypeChanged()
{
	ReplayBufferOutputType type = (ReplayBufferOutputType)ui->outputType->currentData().toInt();

	ui->sceneSelection->clear();

	if (type == RBOutputProgramView) {
		ui->sceneSelection->setDisabled(true);
		ui->sceneSelection->addItem(QTStr("Basic.VCam.OutputSelection.NoSelection"));
	} else {
		ui->sceneSelection->setDisabled(false);
		BPtr<char *> scenes = obs_frontend_get_scene_names();
		for (char **temp = scenes; *temp; temp++) {
			ui->sceneSelection->addItem(*temp);

			if (config.scene.compare(*temp) == 0)
				ui->sceneSelection->setCurrentIndex(ui->sceneSelection->count() - 1);
		}
	}
}

void OBSBasicRBConfig::UpdateConfig()
{
	ReplayBufferOutputType type = (ReplayBufferOutputType)ui->outputType->currentData().toInt();

	config.type = type;
	if (type == RBOutputSceneView)
		config.scene = ui->sceneSelection->currentText().toStdString();

	emit Accepted(config);
}
