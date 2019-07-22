/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QSplitter>
#include <QTabBar>
#include <QStackedWidget>
#include "qt-display.hpp"
#include <obs.hpp>
#include "window-basic-filters.hpp"
#include "window-basic-transform.hpp"

enum PropertiesType { Scene, Source, Transition };

class OBSPropertiesView;
class OBSBasic;

class OBSBasicProperties : public QDialog {
	Q_OBJECT

private:
	QPointer<OBSQTDisplay> preview;

	OBSBasic *main;
	bool acceptClicked;

	OBSSource source;
	OBSSceneItem item;
	OBSSignal removedSignal;
	OBSSignal renamedSignal;
	OBSSignal updatePropertiesSignal;
	OBSData oldSettings;
	OBSPropertiesView *view;
	OBSBasicFilters *filters;
	OBSBasicTransform *transformWindow;
	QDialogButtonBox *buttonBox;
	QSplitter *windowSplitter;
	QStackedWidget *stackedWidget;
	QTabBar *tabsLeft;
	QTabBar *tabsRight;
	QWidget *PerSceneTransitionWidget(QWidget *parent);
	QWidget *AdvancedItemTab(QWidget *parent);
	QWidget *AdvancedGlobalTab(QWidget *parent);
	QComboBox *combo;
	QSpinBox *duration;
	QComboBox *deinterlace;
	QComboBox *sf;
	QComboBox *order;
	QWidget *trOverride;
	QWidget *itemAdvanced;
	QWidget *globalAdvanced;

	enum PropertiesType propType;

	OBSSource sourceA;
	OBSSource sourceB;
	OBSSource sourceClone;
	bool direction = true;

	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceRenamed(void *data, calldata_t *params);
	static void UpdateProperties(void *data, calldata_t *params);
	static void DrawPreview(void *data, uint32_t cx, uint32_t cy);
	static void DrawTransitionPreview(void *data, uint32_t cx, uint32_t cy);
	void UpdateCallback(void *obj, obs_data_t *settings);
	bool ConfirmQuit();
	int CheckSettings();
	void Cleanup();

	void ShowGeneral();
	void ShowFilters();
	void ShowTransform();
	void ShowGlobalAdvanced();
	void ShowItemAdvanced();
	void ShowPerSceneTransition();
	void HideLeft();
	void HideRight();

	void LoadOriginalSettings();
	void SaveOriginalSettings();

	void ResetSourcesDialog();
	void ResetScenesDialog();
	void ResetTransitionsDialog();

	obs_transform_info originalTransform;
	obs_sceneitem_crop originalCrop;

	obs_deinterlace_mode originalDeinterlaceMode;
	obs_deinterlace_field_order originalDeinterlaceOrder;
	obs_scale_type originalScaleFilter;

	const char *originalTransition;
	int originalDuration;

private slots:
	void on_buttonBox_clicked(QAbstractButton *button);
	void AddPreviewButton();
	void SetDeinterlacingMode(int index);
	void SetDeinterlacingOrder(int index);
	void SetScaleFilter(int index);
	void TabsLeftClicked(int index);
	void TabsRightClicked(int index);

public:
	OBSBasicProperties(QWidget *parent, OBSSource source_,
			   PropertiesType type_);
	~OBSBasicProperties();

	void Init();
	void OpenTransformTab();

	OBSSource GetSource();

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual void reject() override;
};
