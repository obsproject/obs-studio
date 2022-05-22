#include "streaming-helpers.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

#include "../plugins/rtmp-services/rtmp-format-ver.h"

#include <util/platform.h>
#include <util/util.hpp>
#include <obs.h>

using namespace json11;

static Json open_json_file(const char *path)
{
	BPtr<char> file_data = os_quick_read_utf8_file(path);
	if (!file_data)
		return Json();

	std::string err;
	Json json = Json::parse(file_data, err);
	if (json["format_version"].int_value() != RTMP_SERVICES_FORMAT_VERSION)
		return Json();
	return json;
}

static inline bool name_matches(const Json &service, const char *name)
{
	if (service["name"].string_value() == name)
		return true;

	auto &alt_names = service["alt_names"].array_items();
	for (const Json &alt_name : alt_names) {
		if (alt_name.string_value() == name) {
			return true;
		}
	}

	return false;
}

Json get_services_json()
{
	obs_module_t *mod = obs_get_module("rtmp-services");
	Json root;

	BPtr<char> file = obs_module_get_config_path(mod, "services.json");
	if (file)
		root = open_json_file(file);

	if (root.is_null()) {
		file = obs_find_module_file(mod, "services.json");
		if (file)
			root = open_json_file(file);
	}

	return root;
}

Json get_service_from_json(const Json &root, const char *name)
{
	auto &services = root["services"].array_items();
	for (const Json &service : services) {
		if (name_matches(service, name)) {
			return service;
		}
	}

	return Json();
}

bool StreamSettingsUI::IsServiceOutputHasNetworkFeatures()
{
	if (IsCustomService())
		return ui_customServer->text().startsWith("rtmp");

	Json service = get_service_from_json(
		GetServicesJson(), QT_TO_UTF8(ui_service->currentText()));

	if (!service["recommended"]["output"].is_string())
		return true;

	if (service["recommended"]["output"].string_value().compare(
		    "rtmp_output") == 0)
		return true;

	return false;
}

void StreamSettingsUI::UpdateMoreInfoLink()
{
	if (IsCustomService()) {
		ui_moreInfoButton->hide();
		return;
	}

	QString serviceName = ui_service->currentText();
	Json service = get_service_from_json(GetServicesJson(),
					     QT_TO_UTF8(serviceName));

	const std::string &more_info_link =
		service["more_info_link"].string_value();

	if (more_info_link.empty()) {
		ui_moreInfoButton->hide();
	} else {
		ui_moreInfoButton->setTargetUrl(QUrl(more_info_link.c_str()));
		ui_moreInfoButton->show();
	}
}

void StreamSettingsUI::UpdateKeyLink()
{
	QString serviceName = ui_service->currentText();
	QString customServer = ui_customServer->text().trimmed();

	Json service = get_service_from_json(GetServicesJson(),
					     QT_TO_UTF8(serviceName));

	std::string streamKeyLink = service["stream_key_link"].string_value();
	if (customServer.contains("fbcdn.net") && IsCustomService()) {
		streamKeyLink =
			"https://www.facebook.com/live/producer?ref=OBS";
	}

	if (serviceName == "Dacast") {
		ui_streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.EncoderKey"));
	} else {
		ui_streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.StreamKey"));
	}

	if (streamKeyLink.empty()) {
		ui_streamKeyButton->hide();
	} else {
		ui_streamKeyButton->setTargetUrl(QUrl(streamKeyLink.c_str()));
		ui_streamKeyButton->show();
	}
}

void StreamSettingsUI::LoadServices(bool showAll)
{
	auto &services = GetServicesJson()["services"].array_items();

	ui_service->blockSignals(true);
	ui_service->clear();

	QStringList names;

	for (const Json &service : services) {
		if (!showAll && !service["common"].bool_value())
			continue;
		names.push_back(service["name"].string_value().c_str());
	}

	if (showAll)
		names.sort(Qt::CaseInsensitive);

	for (QString &name : names)
		ui_service->addItem(name);

	if (!showAll) {
		ui_service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant((int)ListOpt::ShowAll));
	}

	ui_service->insertItem(
		0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"),
		QVariant((int)ListOpt::Custom));

	if (!lastService.isEmpty()) {
		int idx = ui_service->findText(lastService);
		if (idx != -1)
			ui_service->setCurrentIndex(idx);
	}

	ui_service->blockSignals(false);
}

void StreamSettingsUI::UpdateServerList()
{
	QString serviceName = ui_service->currentText();
	bool showMore = ui_service->currentData().toInt() ==
			(int)ListOpt::ShowAll;

	if (showMore) {
		LoadServices(true);
		ui_service->showPopup();
		return;
	} else {
		lastService = serviceName;
	}

	Json service = get_service_from_json(GetServicesJson(),
					     QT_TO_UTF8(serviceName));

	ui_server->clear();

	auto &servers = service["servers"].array_items();
	for (const Json &entry : servers) {
		ui_server->addItem(entry["name"].string_value().c_str(),
				   entry["url"].string_value().c_str());
	}
}
