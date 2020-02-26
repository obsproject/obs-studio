#pragma once

#include <memory>

#include "window-dock.hpp"
#include "auth-caffeine.hpp"

#include <caffeine.h>

Q_DECLARE_METATYPE(caff_Rating);

namespace Ui {
class CaffeinePanel;
}

class CaffeineInfoPanel : public OBSDock {
	Q_OBJECT

private:
	CaffeineAuth *owner;
	std::shared_ptr<Ui::CaffeinePanel> ui;
	caff_InstanceHandle caffeineInstance;

	void registerDockWidget();

public slots:
	void updateClicked(bool);

	void viewOnWebClicked(bool);

public:
	CaffeineInfoPanel(CaffeineAuth *owner, caff_InstanceHandle instance);
	virtual ~CaffeineInfoPanel() override;

	std::string getTitle();
	void setTitle(std::string title);

	caff_Rating getRating();
	void setRating(caff_Rating rating);
};
