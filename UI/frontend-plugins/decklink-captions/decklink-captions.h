#include <QDialog>
#include <obs-module.h>
#include <util/platform.h>
#include <obs.hpp>
#include <memory>
#include "ui_captions.h"

class DecklinkCaptionsUI : public QDialog {
	Q_OBJECT
private:
public:
	std::unique_ptr<Ui_CaptionsDialog> ui;
	DecklinkCaptionsUI(QWidget *parent);

public slots:
	void on_source_currentIndexChanged(int idx);
};

static inline obs_weak_source_t *GetWeakSourceByName(const char *name)
{
	OBSWeakSourceAutoRelease weak;
	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source)
		weak = obs_source_get_weak_source(source);

	return weak;
}
