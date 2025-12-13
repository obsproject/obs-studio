#include "AJAOutputUI.h"
#include "aja-ui-main.h"

#include "../../../plugins/aja/aja-common.hpp"
#include "../../../plugins/aja/aja-ui-props.hpp"
#include "../../../plugins/aja/aja-enums.hpp"
#include "../../../plugins/aja/aja-card-manager.hpp"

#include <ajantv2/includes/ntv2card.h>
#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2enums.h>
#include <ajantv2/includes/ntv2utils.h>

#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>

AJAOutputUI::AJAOutputUI(QWidget *parent) : QDialog(parent), ui(new Ui_Output)
{
	ui->setupUi(this);
	setSizeGripEnabled(true);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	propertiesView = nullptr;
	previewPropertiesView = nullptr;
	miscPropertiesView = nullptr;
}

void AJAOutputUI::ShowHideDialog()
{
	SetupPropertiesView();
	SetupPreviewPropertiesView();
	SetupMiscPropertiesView();

	setVisible(!isVisible());
}

void AJAOutputUI::SetupPropertiesView()
{
	if (propertiesView)
		delete propertiesView;

	obs_data_t *settings = obs_data_create();
	OBSData data = load_settings(kProgramPropsFilename);
	if (data) {
		obs_data_apply(settings, data);
	} else {
		// apply default settings
		obs_data_set_default_int(settings, kUIPropOutput.id, static_cast<long long>(IOSelection::Invalid));
		obs_data_set_default_int(settings, kUIPropVideoFormatSelect.id,
					 static_cast<long long>(kDefaultAJAVideoFormat));
		obs_data_set_default_int(settings, kUIPropPixelFormatSelect.id,
					 static_cast<long long>(kDefaultAJAPixelFormat));
		obs_data_set_default_int(settings, kUIPropSDITransport.id,
					 static_cast<long long>(kDefaultAJASDITransport));
		obs_data_set_default_int(settings, kUIPropSDITransport4K.id,
					 static_cast<long long>(kDefaultAJASDITransport4K));
	}

	// Assign an ID to the program output plugin instance for channel usage tracking
	obs_data_set_string(settings, kUIPropAJAOutputID.id, kProgramOutputID);

	propertiesView =
		new OBSPropertiesView(settings, "aja_output", (PropertiesReloadCallback)obs_get_output_properties, 170);

	ui->propertiesLayout->addWidget(propertiesView);
	obs_data_release(settings);

	connect(propertiesView, &OBSPropertiesView::Changed, this, &AJAOutputUI::PropertiesChanged);
}

void AJAOutputUI::SaveSettings(const char *filename, obs_data_t *settings)
{
	BPtr<char> modulePath = obs_module_get_config_path(obs_current_module(), "");

	os_mkdirs(modulePath);

	BPtr<char> path = obs_module_get_config_path(obs_current_module(), filename);

	if (settings)
		obs_data_save_json_safe(settings, path, "tmp", "bak");
}

void AJAOutputUI::SetupPreviewPropertiesView()
{
	if (previewPropertiesView)
		delete previewPropertiesView;

	obs_data_t *settings = obs_data_create();

	OBSData data = load_settings(kPreviewPropsFilename);
	if (data) {
		obs_data_apply(settings, data);
	} else {
		// apply default settings
		obs_data_set_default_int(settings, kUIPropOutput.id, static_cast<long long>(IOSelection::Invalid));
		obs_data_set_default_int(settings, kUIPropVideoFormatSelect.id,
					 static_cast<long long>(kDefaultAJAVideoFormat));
		obs_data_set_default_int(settings, kUIPropPixelFormatSelect.id,
					 static_cast<long long>(kDefaultAJAPixelFormat));
		obs_data_set_default_int(settings, kUIPropSDITransport.id,
					 static_cast<long long>(kDefaultAJASDITransport));
		obs_data_set_default_int(settings, kUIPropSDITransport4K.id,
					 static_cast<long long>(kDefaultAJASDITransport4K));
	}

	// Assign an ID to the program output plugin instance for channel usage tracking
	obs_data_set_string(settings, kUIPropAJAOutputID.id, kPreviewOutputID);

	previewPropertiesView =
		new OBSPropertiesView(settings, "aja_output", (PropertiesReloadCallback)obs_get_output_properties, 170);

	ui->previewPropertiesLayout->addWidget(previewPropertiesView);
	obs_data_release(settings);

	connect(previewPropertiesView, &OBSPropertiesView::Changed, this, &AJAOutputUI::PreviewPropertiesChanged);
}

void AJAOutputUI::on_outputButton_clicked()
{
	SaveSettings(kProgramPropsFilename, propertiesView->GetSettings());
	output_toggle();
}

void AJAOutputUI::PropertiesChanged()
{
	SaveSettings(kProgramPropsFilename, propertiesView->GetSettings());
}

void AJAOutputUI::OutputStateChanged(bool active)
{
	QString text;
	if (active) {
		text = QString(obs_module_text("Stop"));
	} else {
		text = QString(obs_module_text("Start"));
	}

	ui->outputButton->setChecked(active);
	ui->outputButton->setText(text);
}

void AJAOutputUI::on_previewOutputButton_clicked()
{
	SaveSettings(kPreviewPropsFilename, previewPropertiesView->GetSettings());
	preview_output_toggle();
}

void AJAOutputUI::PreviewPropertiesChanged()
{
	SaveSettings(kPreviewPropsFilename, previewPropertiesView->GetSettings());
}

void AJAOutputUI::PreviewOutputStateChanged(bool active)
{
	QString text;
	if (active) {
		text = QString(obs_module_text("Stop"));
	} else {
		text = QString(obs_module_text("Start"));
	}

	ui->previewOutputButton->setChecked(active);
	ui->previewOutputButton->setText(text);
}

static obs_properties_t *create_misc_props_ui(void *vp)
{
	AJAOutputUI *outputUI = (AJAOutputUI *)vp;
	if (!outputUI)
		return nullptr;
	aja::CardManager *cardManager = outputUI->GetCardManager();
	if (!cardManager)
		return nullptr;

	bool haveMultiView = false;
	for (auto &c : *cardManager) {
		auto deviceID = c.second->GetDeviceID();
		for (const auto &id : aja::MultiViewCards()) {
			if (deviceID == id) {
				haveMultiView = true;
				break;
			}
		}
	}

	obs_properties_t *props = obs_properties_create();
	obs_property_t *deviceList = obs_properties_add_list(props, kUIPropDevice.id,
							     obs_module_text(kUIPropDevice.text), OBS_COMBO_TYPE_LIST,
							     OBS_COMBO_FORMAT_STRING);
	obs_property_t *multiViewEnable =
		obs_properties_add_bool(props, kUIPropMultiViewEnable.id, obs_module_text(kUIPropMultiViewEnable.text));
	obs_property_t *multiViewAudioSources = obs_properties_add_list(
		props, kUIPropMultiViewAudioSource.id, obs_module_text(kUIPropMultiViewAudioSource.text),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_clear(deviceList);
	obs_property_list_clear(multiViewAudioSources);

	NTV2DeviceID firstDeviceID = DEVICE_ID_NOTFOUND;
	populate_misc_device_list(deviceList, cardManager, firstDeviceID);
	populate_multi_view_audio_sources(multiViewAudioSources, firstDeviceID);
	obs_property_set_modified_callback2(deviceList, on_misc_device_selected, cardManager);
	obs_property_set_modified_callback2(multiViewEnable, on_multi_view_toggle, cardManager);
	obs_property_set_modified_callback2(multiViewAudioSources, on_multi_view_toggle, cardManager);

	outputUI->ui->label_3->setVisible(haveMultiView);
	obs_property_set_visible(deviceList, haveMultiView);
	obs_property_set_visible(multiViewEnable, haveMultiView);
	obs_property_set_visible(multiViewAudioSources, haveMultiView);

	return props;
}

void AJAOutputUI::MiscPropertiesChanged()
{
	SaveSettings(kMiscPropsFilename, miscPropertiesView->GetSettings());
}

void AJAOutputUI::SetCardManager(aja::CardManager *cm)
{
	cardManager = cm;
}

aja::CardManager *AJAOutputUI::GetCardManager()
{
	return cardManager;
}

void AJAOutputUI::SetupMiscPropertiesView()
{
	if (miscPropertiesView)
		delete miscPropertiesView;

	obs_data_t *settings = obs_data_create();
	OBSData data = load_settings(kMiscPropsFilename);
	if (data) {
		obs_data_apply(settings, data);
	}

	miscPropertiesView = new OBSPropertiesView(settings, this, (PropertiesReloadCallback)create_misc_props_ui,
						   nullptr, nullptr, 170);

	ui->miscPropertiesLayout->addWidget(miscPropertiesView);
	obs_data_release(settings);
	connect(miscPropertiesView, &OBSPropertiesView::Changed, this, &AJAOutputUI::MiscPropertiesChanged);
}
