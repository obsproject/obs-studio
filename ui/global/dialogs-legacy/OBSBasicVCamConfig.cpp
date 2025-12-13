#include "OBSBasicVCamConfig.hpp"

#include <OBSApp.hpp>

#include "moc_OBSBasicVCamConfig.cpp"

OBSBasicVCamConfig::OBSBasicVCamConfig(const VCamConfig &_config, bool _vcamActive, QWidget *parent)
	: config(_config),
	  vcamActive(_vcamActive),
	  activeType(_config.type),
	  QDialog(parent),
	  ui(new Ui::OBSBasicVCamConfig)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->outputType->addItem(QTStr("Basic.VCam.OutputType.Program"), (int)VCamOutputType::ProgramView);
	ui->outputType->addItem(QTStr("StudioMode.Preview"), (int)VCamOutputType::PreviewOutput);
	ui->outputType->addItem(QTStr("Basic.Scene"), (int)VCamOutputType::SceneOutput);
	ui->outputType->addItem(QTStr("Basic.Main.Source"), (int)VCamOutputType::SourceOutput);

	ui->outputType->setCurrentIndex(ui->outputType->findData((int)config.type));
	OutputTypeChanged();
	connect(ui->outputType, &QComboBox::currentIndexChanged, this, &OBSBasicVCamConfig::OutputTypeChanged);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &OBSBasicVCamConfig::UpdateConfig);
}

void OBSBasicVCamConfig::OutputTypeChanged()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentData().toInt();
	ui->outputSelection->setDisabled(false);

	auto list = ui->outputSelection;
	list->clear();

	switch (type) {
	case VCamOutputType::Invalid:
	case VCamOutputType::ProgramView:
	case VCamOutputType::PreviewOutput:
		ui->outputSelection->setDisabled(true);
		list->addItem(QTStr("Basic.VCam.OutputSelection.NoSelection"));
		break;
	case VCamOutputType::SceneOutput: {
		// Scenes in default order
		BPtr<char *> scenes = obs_frontend_get_scene_names();
		for (char **temp = scenes; *temp; temp++) {
			list->addItem(*temp);

			if (config.scene.compare(*temp) == 0)
				list->setCurrentIndex(list->count() - 1);
		}
		break;
	}
	case VCamOutputType::SourceOutput: {
		// Sources in alphabetical order
		std::vector<std::string> sources;
		auto AddSource = [&](obs_source_t *source) {
			auto name = obs_source_get_name(source);

			if (!(obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO))
				return;

			sources.push_back(name);
		};
		using AddSource_t = decltype(AddSource);

		obs_enum_sources(
			[](void *data, obs_source_t *source) {
				auto &AddSource = *static_cast<AddSource_t *>(data);
				if (!obs_source_removed(source))
					AddSource(source);
				return true;
			},
			static_cast<void *>(&AddSource));

		// Sort and select current item
		sort(sources.begin(), sources.end());
		for (auto &&source : sources) {
			list->addItem(source.c_str());

			if (config.source == source)
				list->setCurrentIndex(list->count() - 1);
		}
		break;
	}
	}

	if (!vcamActive)
		return;

	requireRestart = (activeType == VCamOutputType::ProgramView && type != VCamOutputType::ProgramView) ||
			 (activeType != VCamOutputType::ProgramView && type == VCamOutputType::ProgramView);

	ui->warningLabel->setVisible(requireRestart);
}

void OBSBasicVCamConfig::UpdateConfig()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentData().toInt();
	switch (type) {
	case VCamOutputType::ProgramView:
	case VCamOutputType::PreviewOutput:
		break;
	case VCamOutputType::SceneOutput:
		config.scene = ui->outputSelection->currentText().toStdString();
		break;
	case VCamOutputType::SourceOutput:
		config.source = ui->outputSelection->currentText().toStdString();
		break;
	default:
		// unknown value, don't save type
		return;
	}

	config.type = type;

	if (requireRestart) {
		emit AcceptedAndRestart(config);
	} else {
		emit Accepted(config);
	}
}
