#include "window-caffeine.hpp"
#include "window-basic-main.hpp"

#include "ui_CaffeinePanel.h"

#include <QDesktopServices>
#include <stdlib.h>

void CaffeineInfoPanel::registerDockWidget()
{
	this->setObjectName(QStringLiteral("caffeineDock"));
	this->setWindowTitle(QTStr("Caffeine.Dock"));
	this->setVisible(true);
	OBSBasic::Get()->AddDockWidget(this);
}

void CaffeineInfoPanel::updateClicked(bool)
{
	std::string title = std::string(ui->title->text().toUtf8().data());
	caff_Rating rating = ui->rating->itemData(ui->rating->currentIndex())
				     .value<caff_Rating>();

	setTitle(title);
	setRating(rating);

	obs_service_t *service = OBSBasic::Get()->GetService();
	if (service) {
		obs_data_t *data = obs_service_get_settings(service);
		obs_data_set_string(data, "broadcast_title",
				    getTitle().c_str());
		obs_data_set_int(data, "rating",
				 static_cast<int64_t>(getRating()));
		obs_service_update(service, data);
		obs_data_release(data);
	}
}

void CaffeineInfoPanel::viewOnWebClicked(bool)
{
	QUrl url;
	// Set the Caffeine URL default or staging
	const char *host = getenv("LIBCAFFEINE_DOMAIN") == NULL
				   ? "caffeine.tv"
				   : "www.staging.caffeine.tv";
	url.setHost(host);
	url.setPath(QString::fromStdString("/" + owner->GetUsername()));
	url.setScheme("https");
	QDesktopServices::openUrl(url);
}

CaffeineInfoPanel::CaffeineInfoPanel(CaffeineAuth *owner,
				     caff_InstanceHandle instance)
	: OBSDock(OBSBasic::Get()),
	  owner(owner),
	  ui(new Ui::CaffeinePanel),
	  caffeineInstance(instance)
{
	ui->setupUi(this);

	// Set up ratings.
	ui->rating->clear();
	ui->rating->addItem(QTStr("Caffeine.Rating.None"),
			    QVariant(caff_RatingNone));
	ui->rating->addItem(QTStr("Caffeine.Rating.SeventeenPlus"),
			    QVariant(caff_RatingSeventeenPlus));
	ui->rating->setCurrentIndex(static_cast<int>(getRating()));

	// Set up title
	ui->title->setText(QString::fromStdString(getTitle()));
	ui->title->setAttribute(Qt::WA_MacShowFocusRect, false);

	// Buttons
	connect(ui->updateButton, SIGNAL(clicked(bool)),
		SLOT(updateClicked(bool)));
	connect(ui->viewOnWebBtn, SIGNAL(clicked(bool)),
		SLOT(viewOnWebClicked(bool)));

	this->registerDockWidget();
}

CaffeineInfoPanel::~CaffeineInfoPanel()
{
	// Remove the Panel from OBS
	OBSBasic::Get()->RemoveCaffeineDockWidget(this);
}

std::string CaffeineInfoPanel::getTitle()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_default_string(main->Config(), "Caffeine", "Title",
				  tr("Caffeine.Title").toUtf8().data());
	const char *title =
		config_get_string(main->Config(), "Caffeine", "Title");
	if (!title) {
		return std::string(tr("Caffeine.Title").toUtf8().data());
	}
	return title;
}

void CaffeineInfoPanel::setTitle(std::string title)
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), "Caffeine", "Title", title.c_str());
}

caff_Rating CaffeineInfoPanel::getRating()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_default_int(main->Config(), "Caffeine", "Rating",
			       caff_RatingNone);
	return static_cast<caff_Rating>(
		config_get_int(main->Config(), "Caffeine", "Rating"));
}

void CaffeineInfoPanel::setRating(caff_Rating rating)
{
	OBSBasic *main = OBSBasic::Get();
	config_set_int(main->Config(), "Caffeine", "Rating",
		       static_cast<int64_t>(rating));
}
