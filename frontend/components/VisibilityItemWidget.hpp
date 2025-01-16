#pragma once

#include <obs.hpp>

#include <QListWidget>
#include <QWidget>

class OBSSourceLabel;
class QCheckBox;

class VisibilityItemWidget : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	OBSSourceLabel *label = nullptr;
	QCheckBox *vis = nullptr;

	OBSSignal enabledSignal;

	bool active = false;
	bool selected = false;

	static void OBSSourceEnabled(void *param, calldata_t *data);

private slots:
	void SourceEnabled(bool enabled);

public:
	VisibilityItemWidget(obs_source_t *source);

	void SetColor(const QColor &color, bool active, bool selected);
};

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item, obs_source_t *source);
