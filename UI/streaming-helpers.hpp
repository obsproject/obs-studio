#pragma once

#include "url-push-button.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

#include <json11.hpp>

extern json11::Json get_services_json();
extern json11::Json get_service_from_json(const json11::Json &root,
					  const char *name);

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

class StreamSettingsUI : public QObject {
	Q_OBJECT

	QLabel *ui_streamKeyLabel;
	QComboBox *ui_service;
	QComboBox *ui_server;
	QLineEdit *ui_customServer;
	UrlPushButton *ui_moreInfoButton;
	UrlPushButton *ui_streamKeyButton;

	json11::Json servicesRoot;
	bool servicesLoaded = false;
	QString lastService;

public:
	inline void Setup(QLabel *streamKeyLabel, QComboBox *service,
			  QComboBox *server, QLineEdit *customServer,
			  UrlPushButton *moreInfoButton,
			  UrlPushButton *streamKeyButton)
	{
		ui_streamKeyLabel = streamKeyLabel;
		ui_service = service;
		ui_server = server;
		ui_customServer = customServer;
		ui_moreInfoButton = moreInfoButton;
		ui_streamKeyButton = streamKeyButton;
	}

	inline bool IsCustomService() const
	{
		return ui_service->currentData().toInt() ==
		       (int)ListOpt::Custom;
	}

	inline void ClearLastService() { lastService.clear(); }

	inline json11::Json GetServicesJson()
	{
		if (!servicesLoaded && servicesRoot.is_null()) {
			servicesRoot = get_services_json();
			servicesLoaded = true;
		}
		return servicesRoot;
	}

	inline const QString &LastService() const { return lastService; }

public slots:
	void UpdateMoreInfoLink();
	void UpdateKeyLink();
	void LoadServices(bool showAll);
	void UpdateServerList();
};
