#include "scene-wizard.hpp"
#include "obs-app.hpp"
#include "platform.hpp"
#include "qt-wrappers.hpp"
#include "display-helpers.hpp"

#include <QDir>

static void SaveSourceSettings(OBSSource source, const char *id)
{
	if (!source)
		return;

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_string(settings, "device_id", id);
	obs_source_update(source, settings);
}

void SceneWizard::SaveMicrophone()
{
	if (ui->disableMic->isChecked()) {
		obs_set_output_source(3, nullptr);
		return;
	}

	OBSSource source = obs_get_output_source(3);
	obs_source_release(source);

	if (!source)
		return;

	int index = ui->micCombo->currentIndex();
	QString id = ui->micCombo->itemData(index).toString();
	SaveSourceSettings(source, QT_TO_UTF8(id));
}

void SceneWizard::SaveDesktopAudio()
{
	if (ui->disableDesktopAudio->isChecked()) {
		obs_set_output_source(1, nullptr);
		return;
	}

	OBSSource source = obs_get_output_source(1);
	obs_source_release(source);

	if (!source)
		return;

	int index = ui->desktopAudioCombo->currentIndex();
	QString id = ui->desktopAudioCombo->itemData(index).toString();
	SaveSourceSettings(source, QT_TO_UTF8(id));
}

void SceneWizard::RemoveWebcam()
{
	if (ui->disableWebcam->isChecked())
		return;

	auto cb = [](void *, obs_source_t *source) {
		if (!source)
			return true;

		const char *id = obs_source_get_id(source);

		if (strcmp(id, App()->VideoCaptureSource()) == 0)
			obs_source_remove(source);

		return true;
	};

	obs_enum_sources(cb, nullptr);
}

void SceneWizard::SaveWebcam()
{
	if (ui->disableWebcam->isChecked()) {
		RemoveWebcam();
		return;
	}

	int index = ui->webcamCombo->currentIndex();

	if (index == 0)
		return;

	const char *id =
		QT_TO_UTF8(ui->webcamCombo->itemData(index).toString());

	auto cb = [](void *data, obs_source_t *source) {
		if (!source)
			return true;

		const char *id = obs_source_get_id(source);
		const char *deviceID = reinterpret_cast<char *>(data);

		if (strcmp(id, App()->VideoCaptureSource()) == 0)
			SaveSourceSettings(source, deviceID);

		return true;
	};

	obs_enum_sources(cb, &id);
}

void SceneWizard::on_micCombo_currentIndexChanged(int index)
{
	QString id = ui->desktopAudioCombo->itemData(index).toString();
	SaveSourceSettings(mic, QT_TO_UTF8(id));
}

void SceneWizard::on_desktopAudioCombo_currentIndexChanged(int index)
{
	QString id = ui->desktopAudioCombo->itemData(index).toString();
	SaveSourceSettings(desktopAudio, QT_TO_UTF8(id));
}

void SceneWizard::on_webcamCombo_currentIndexChanged(int index)
{
	QString id = ui->webcamCombo->itemData(index).toString();
	SaveSourceSettings(cam, QT_TO_UTF8(id));
}

static inline void LoadListValue(QComboBox *widget, const char *text,
				 const char *val)
{
	widget->addItem(QT_UTF8(text), QT_UTF8(val));
}

static void LoadListValues(QComboBox *widget, obs_property_t *prop)
{
	size_t count = obs_property_list_item_count(prop);

	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(prop, i);
		const char *val = obs_property_list_item_string(prop, i);
		LoadListValue(widget, name, val);
	}
}

void SceneWizard::LoadDevices()
{
	obs_properties_t *cam_props = obs_source_properties(cam);
	obs_properties_t *input_props = obs_source_properties(mic);
	obs_properties_t *output_props = obs_source_properties(desktopAudio);

	if (cam_props) {
		obs_property_t *cams =
			obs_properties_get(cam_props, "device_id");
		LoadListValues(ui->webcamCombo, cams);
		obs_properties_destroy(cam_props);
	}

	if (input_props) {
		obs_property_t *inputs =
			obs_properties_get(input_props, "device_id");
		LoadListValues(ui->micCombo, inputs);
		obs_properties_destroy(input_props);
	}

	if (output_props) {
		obs_property_t *outputs =
			obs_properties_get(output_props, "device_id");
		LoadListValues(ui->desktopAudioCombo, outputs);
		obs_properties_destroy(output_props);
	}
}

void SceneWizard::OBSRender(void *data, uint32_t cx, uint32_t cy)
{
	SceneWizard *window = static_cast<SceneWizard *>(data);

	if (!window->cam)
		return;

	uint32_t sourceCX = std::max(obs_source_get_width(window->cam), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(window->cam), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(window->cam);

	gs_projection_pop();
	gs_viewport_pop();
}

SceneWizard::SceneWizard(QWidget *parent)
	: QDialog(parent), ui(new Ui::SceneWizard)
{
	ui->setupUi(this);
	setFixedSize(750, 750);

	ui->back->setText("<a href='#' style='text-decoration:none;'>" +
			  QTStr("Back") + "</a>");
	ui->next->setText("<a href='#' style='text-decoration:none;'>" +
			  QTStr("Next") + "</a>");
	ui->bottomWidget->setProperty("themeID", "aboutHLayout");
	ui->back->setProperty("themeID", "aboutHLayout");
	ui->next->setProperty("themeID", "aboutHLayout");
	ui->templateList->setProperty("themeID", "templateList");

	style()->unpolish(ui->templateList);
	style()->polish(ui->templateList);

	ui->noTemplates->hide();

	InitTemplates();

	mic = obs_source_create(App()->InputAudioSource(), "microphone",
				nullptr, nullptr);
	desktopAudio = obs_source_create(App()->OutputAudioSource(),
					 "desktop_audio", nullptr, nullptr);
	cam = obs_source_create_private(App()->VideoCaptureSource(), nullptr,
					nullptr);

	obs_source_release(mic);
	obs_source_release(desktopAudio);
	obs_source_release(cam);

	preview = new OBSQTDisplay(ui->devicePage);
	preview->setMinimumSize(20, 150);
	preview->setSizePolicy(
		QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(preview->GetDisplay(),
					      SceneWizard::OBSRender, this);
	};

	preview->show();
	connect(preview.data(), &OBSQTDisplay::DisplayCreated, addDrawCallback);

	LoadDevices();

	micVol = new VolControl(mic, true, false);
	desktopVol = new VolControl(desktopAudio, true, false);

	micVol->slider->hide();
	micVol->config->hide();
	micVol->nameLabel->hide();
	micVol->volLabel->hide();
	micVol->mute->hide();

	desktopVol->slider->hide();
	desktopVol->config->hide();
	desktopVol->nameLabel->hide();
	desktopVol->volLabel->hide();
	desktopVol->mute->hide();

	ui->deviceLayout->insertWidget(2, preview);
	ui->deviceLayout->insertWidget(6, micVol);
	ui->deviceLayout->insertWidget(10, desktopVol);

	ui->templateList->setCurrentRow(0);
	GoToOptionsPage();
}

SceneWizard::~SceneWizard()
{
	obs_display_remove_draw_callback(preview->GetDisplay(),
					 SceneWizard::OBSRender, this);
}

void SceneWizard::UpdateSceneCollection()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	QString templatePath;

	QListWidgetItem *item = ui->templateList->currentItem();

	if (item)
		templatePath = item->data(Qt::UserRole).toString();

	if (!templatePath.isEmpty()) {
		main->SaveProjectNow();
		main->Load(QT_TO_UTF8(templatePath));
		main->RefreshSceneCollections();

		const char *newName = config_get_string(
			App()->GlobalConfig(), "Basic", "SceneCollection");
		const char *newFile = config_get_string(
			App()->GlobalConfig(), "Basic", "SceneCollectionFile");

		blog(LOG_INFO, "Switched to scene collection '%s' (%s.json)",
		     newName, newFile);
		blog(LOG_INFO,
		     "------------------------------------------------");

		main->UpdateTitleBar();

		if (main->api)
			main->api->on_event(
				OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
	}

	SaveMicrophone();
	SaveDesktopAudio();
	SaveWebcam();

	close();
}

void SceneWizard::on_importButton_clicked()
{
	close();

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	main->on_actionImportSceneCollection_triggered();
}

void SceneWizard::on_templateButton_clicked()
{
	GoToTemplatePage();
}

void SceneWizard::on_setupOnOwnButton_clicked()
{
	close();
}

void SceneWizard::on_disableWebcam_toggled(bool checked)
{
	ui->webcamCombo->setEnabled(!checked);
}

void SceneWizard::on_disableMic_toggled(bool checked)
{
	ui->micCombo->setEnabled(!checked);
}

void SceneWizard::on_disableDesktopAudio_toggled(bool checked)
{
	ui->desktopAudioCombo->setEnabled(!checked);
}

void SceneWizard::GoToOptionsPage()
{
	ui->stackedWidget->setCurrentWidget(ui->optionPage);
	ui->bottomWidget->hide();
}

void SceneWizard::GoToTemplatePage()
{
	ui->stackedWidget->setCurrentWidget(ui->templatePage);
	ui->bottomWidget->show();
}

void SceneWizard::GoToDevicesPage()
{
	ui->stackedWidget->setCurrentWidget(ui->devicePage);
	ui->bottomWidget->show();
	ui->next->setText("<a href='#' style='text-decoration:none;'>" +
			  QTStr("Finish") + "</a>");
}

void SceneWizard::on_back_clicked()
{
	QWidget *widget = ui->stackedWidget->currentWidget();

	if (widget == ui->devicePage)
		GoToTemplatePage();
	else if (widget == ui->templatePage)
		GoToOptionsPage();
}

void SceneWizard::on_next_clicked()
{
	QWidget *widget = ui->stackedWidget->currentWidget();

	if (widget == ui->devicePage) {
		UpdateSceneCollection();
		close();
	} else if (widget == ui->templatePage) {
		GoToDevicesPage();
	}
}

void SceneWizard::closeEvent(QCloseEvent *event)
{
	deleteLater();
	QDialog::closeEvent(event);
}

#ifdef __APPLE__
#define TEMPLATE_FOLDER "mac"
#elif _WIN32
#define TEMPLATE_FOLDER "windows"
#else
#define TEMPLATE_FOLDER "nix"
#endif

void SceneWizard::InitTemplates()
{
	std::string path;

	if (!GetDataFilePath("templates/", path))
		return;

	QDir folder(QT_UTF8(path.c_str()) + TEMPLATE_FOLDER + "/");

	QStringList files =
		folder.entryList(QStringList() << "*.json", QDir::Files);

	if (files.count() == 0) {
		ui->noTemplates->show();
		return;
	}

	for (auto file : files) {
		QString filename;
		filename += QT_UTF8(path.c_str()) + TEMPLATE_FOLDER + "/";
		filename += file;

		QListWidgetItem *item = new QListWidgetItem();
		item->setData(Qt::UserRole, filename);
		item->setText(file.replace(".json", ""));

		QPixmap pixmap(filename.replace(".json", ".png"));

		if (!pixmap)
			pixmap.load(QString(":res/images/no-image.png"));

		QIcon icon;
		icon.addPixmap(pixmap, QIcon::Normal);
		icon.addPixmap(pixmap, QIcon::Selected);
		item->setIcon(icon);

		ui->templateList->addItem(item);
	}
}
