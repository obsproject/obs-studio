#include "window-basic-vcam-config.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#include <util/util.hpp>
#include <util/platform.h>

OBSBasicVCamConfig::OBSBasicVCamConfig(const VCamConfig &_config,
				       QWidget *parent)
	: config(_config), QDialog(parent), ui(new Ui::OBSBasicVCamConfig)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->outputType->setCurrentIndex(config.type);
	OutputTypeChanged(config.type);
	connect(ui->outputType, SIGNAL(currentIndexChanged(int)), this,
		SLOT(OutputTypeChanged(int)));

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
		&OBSBasicVCamConfig::UpdateConfig);
}

void OBSBasicVCamConfig::OutputTypeChanged(int type)
{
	auto list = ui->outputSelection;
	list->clear();

	switch ((VCamOutputType)type) {
	case VCamOutputType::InternalOutput:
		list->addItem(QTStr("Basic.VCam.InternalDefault"));
		list->addItem(QTStr("Basic.VCam.InternalPreview"));
		list->setCurrentIndex(config.internal);
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

			if (!(obs_source_get_output_flags(source) &
			      OBS_SOURCE_VIDEO))
				return;

			sources.push_back(name);
		};
		using AddSource_t = decltype(AddSource);

		obs_enum_sources(
			[](void *data, obs_source_t *source) {
				auto &AddSource =
					*static_cast<AddSource_t *>(data);
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
}

void OBSBasicVCamConfig::UpdateConfig()
{
	VCamOutputType type = (VCamOutputType)ui->outputType->currentIndex();
	switch (type) {
	case VCamOutputType::InternalOutput:
		config.internal =
			(VCamInternalType)ui->outputSelection->currentIndex();
		break;
	case VCamOutputType::SceneOutput:
		config.scene = ui->outputSelection->currentText().toStdString();
		break;
	case VCamOutputType::SourceOutput:
		config.source =
			ui->outputSelection->currentText().toStdString();
		break;
	default:
		// unknown value, don't save type
		return;
	}

	config.type = type;

	emit Accepted(config);
}
