/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#include "OBSBasic.hpp"

#include <utility/display-helpers.hpp>
#include <widgets/OBSProjector.hpp>

#include <qt-wrappers.hpp>

#include <QColorDialog>

#include <sstream>

extern void undo_redo(const std::string &data);

using namespace std;

void OBSBasic::InitPrimitives()
{
	ProfileScope("OBSBasic::InitPrimitives");

	obs_enter_graphics();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	box = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	boxLeft = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(1.0f, 0.0f);
	boxTop = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxRight = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxBottom = gs_render_save();

	gs_render_start(true);
	for (int i = 0; i <= 360; i += (360 / 20)) {
		float pos = RAD(float(i));
		gs_vertex2f(cosf(pos), sinf(pos));
	}
	circle = gs_render_save();

	InitSafeAreas(&actionSafeMargin, &graphicsSafeMargin, &fourByThreeSafeMargin, &leftLine, &topLine, &rightLine);
	obs_leave_graphics();
}

void OBSBasic::UpdatePreviewScalingMenu()
{
	bool fixedScaling = ui->preview->IsFixedScaling();
	float scalingAmount = ui->preview->GetScalingAmount();
	if (!fixedScaling) {
		ui->actionScaleWindow->setChecked(true);
		ui->actionScaleCanvas->setChecked(false);
		ui->actionScaleOutput->setChecked(false);
		return;
	}

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	ui->actionScaleWindow->setChecked(false);
	ui->actionScaleCanvas->setChecked(scalingAmount == 1.0f);
	ui->actionScaleOutput->setChecked(scalingAmount == float(ovi.output_width) / float(ovi.base_width));
}

void OBSBasic::DrawBackdrop(float cx, float cy)
{
	if (!box)
		return;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawBackdrop");

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	vec4 colorVal;
	vec4_set(&colorVal, 0.0f, 0.0f, 0.0f, 1.0f);
	gs_effect_set_vec4(color, &colorVal);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_scale3f(float(cx), float(cy), 1.0f);

	gs_load_vertexbuffer(box);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_pop();
	gs_technique_end_pass(tech);
	gs_technique_end(tech);

	gs_load_vertexbuffer(nullptr);

	GS_DEBUG_MARKER_END();
}

void OBSBasic::RenderHorizontalMain(void *data, uint32_t, uint32_t)
{
	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderHorizontalMain");

	OBSBasic *window = static_cast<OBSBasic *>(data);
	obs_video_info ovi;

	const obs_video_info* h_ovi_ptr = App()->GetHorizontalVideoInfo();
	if (h_ovi_ptr && h_ovi_ptr->base_width > 0) {
		ovi = *h_ovi_ptr;
	} else {
		// Fallback if App's OVI isn't ready (should ideally not happen at render time)
		obs_get_video_info(&ovi);
		blog(LOG_WARNING, "RenderHorizontalMain: Using fallback OVI.");
	}

	// Assuming previewScale, previewX, previewY are for horizontal preview (ui->preview which is mainPreview_h)
	window->previewCX = int(window->previewScale * float(ovi.base_width));
	window->previewCY = int(window->previewScale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();

	obs_display_t *display = window->ui->preview->GetDisplay(); // This is mainPreview_h
	uint32_t width, height;
	obs_display_size(display, &width, &height);
	float right = float(width) - window->previewX;
	float bottom = float(height) - window->previewY;

	gs_ortho(-window->previewX, right, -window->previewY, bottom, -100.0f, 100.0f);

	window->ui->preview->DrawOverflow();

	/* --------------------------------------- */

	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	gs_set_viewport(window->previewX, window->previewY, window->previewCX, window->previewCY);

	if (window->IsPreviewProgramMode() && !App()->IsDualOutputActive()) { // Original logic if not dual output
		window->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));
		OBSScene scene = window->GetCurrentScene(); // This will need to be context aware
		obs_source_t *source = obs_scene_get_source(scene);
		if (source)
			obs_source_video_render(source);
	} else {
		// If Dual Output is active, or not in studio mode, render the current horizontal scene
		obs_source_t *h_scene = App()->GetCurrentHorizontalScene();
		if (h_scene) {
			obs_source_video_render(h_scene);
		} else {
			// Fallback: render main texture or backdrop if no specific horizontal scene
			obs_render_main_texture_src_color_only();
			// window->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));
		}
	}
	gs_load_vertexbuffer(nullptr);

	/* --------------------------------------- */

	gs_ortho(-window->previewX, right, -window->previewY, bottom, -100.0f, 100.0f);
	gs_reset_viewport();

	uint32_t targetCX = window->previewCX;
	uint32_t targetCY = window->previewCY;

	if (window->drawSafeAreas) {
		RenderSafeAreas(window->actionSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->graphicsSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->fourByThreeSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->leftLine, targetCX, targetCY);
		RenderSafeAreas(window->topLine, targetCX, targetCY);
		RenderSafeAreas(window->rightLine, targetCX, targetCY);
	}

	window->ui->preview->DrawSceneEditing();

	if (window->drawSpacingHelpers)
		window->ui->preview->DrawSpacingHelpers();

	/* --------------------------------------- */

	gs_projection_pop();
	gs_viewport_pop();

	GS_DEBUG_MARKER_END();
}

void OBSBasic::RenderVerticalMain(void *data, uint32_t, uint32_t)
{
{
	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderVerticalMain");

	OBSBasic *window = static_cast<OBSBasic *>(data);
	obs_video_info ovi;

	if (!App()->IsDualOutputActive()) {
		// If dual output is not active, don't render anything in the vertical preview
		// or draw a clear backdrop.
		uint32_t display_cx, display_cy;
		obs_display_size(window->ui->mainPreview_v->GetDisplay(), &display_cx, &display_cy);
		window->DrawBackdrop(display_cx, display_cy); // Draw black backdrop
		GS_DEBUG_MARKER_END();
		return;
	}

	const obs_video_info* v_ovi_ptr = App()->GetVerticalVideoInfo();
	if (v_ovi_ptr && v_ovi_ptr->base_width > 0) {
		ovi = *v_ovi_ptr;
	} else {
		// Fallback, but should be configured if dual output is active
		blog(LOG_WARNING, "RenderVerticalMain: Vertical OVI not properly configured, using fallback.");
		obs_get_video_info(&ovi); // Fallback to main OVI, will look wrong
		// Or, set a default vertical OVI
		// ovi.base_width = 1080; ovi.base_height = 1920; // etc.
	}

	// TODO: Vertical preview needs its own scaling (previewScale_v, previewX_v etc.) and
	// viewport calculation (previewCX_v, previewCY_v) if it is to be independently pannable/zoomable.
	// For now, using horizontal's scaling properties but applying to vertical's base dimensions.
	// This will likely not be correct for independent scaling.
	// A new set of members (e.g., previewScale_v) and a new ResizePreview_V method would be needed.
	// int previewCX_v = int(window->previewScale * float(ovi.base_width)); // Calculated by ResizePreview_V
	// int previewCY_v = int(window->previewScale * float(ovi.base_height)); // Calculated by ResizePreview_V
	// int previewX_v = window->previewX; // Placeholder // Calculated by ResizePreview_V
	// int previewY_v = window->previewY; // Placeholder // Calculated by ResizePreview_V


	gs_viewport_push();
	gs_projection_push();

	obs_display_t *display = window->ui->mainPreview_v->GetDisplay();
	uint32_t width, height; // Display widget dimensions
	obs_display_size(display, &width, &height);

	// Use the _v members calculated by ResizePreview_V
	float right = float(width) - window->previewX_v;
	float bottom = float(height) - window->previewY_v;

	gs_ortho(-window->previewX_v, right, -window->previewY_v, bottom, -100.0f, 100.0f);

	window->ui->mainPreview_v->DrawOverflow();

	/* --------------------------------------- */

	// Set projection for the actual scene rendering based on vertical OVI base dimensions
	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	// Set viewport using the calculated _v dimensions for the scaled preview
	gs_set_viewport(window->previewX_v, window->previewY_v, window->previewCX_v, window->previewCY_v);

	obs_source_t *v_scene = App()->GetCurrentVerticalScene();
	if (v_scene) {
		obs_source_video_render(v_scene);
	} else {
		// Fallback: render a clear backdrop if no vertical scene
		window->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));
		blog(LOG_INFO, "RenderVerticalMain: No vertical scene set to render.");
	}
	gs_load_vertexbuffer(nullptr);

	/* --------------------------------------- */

	gs_ortho(-previewX_v, right, -previewY_v, bottom, -100.0f, 100.0f);
	gs_reset_viewport();

	uint32_t targetCX = previewCX_v;
	uint32_t targetCY = previewCY_v;

	// Assuming drawSafeAreas and drawSpacingHelpers are global toggles.
	// If they need to be per-preview, OBSBasic would need separate bools.
	if (window->drawSafeAreas) {
		RenderSafeAreas(window->actionSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->graphicsSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->fourByThreeSafeMargin, targetCX, targetCY);
		RenderSafeAreas(window->leftLine, targetCX, targetCY);
		RenderSafeAreas(window->topLine, targetCX, targetCY);
		RenderSafeAreas(window->rightLine, targetCX, targetCY);
	}

	window->ui->mainPreview_v->DrawSceneEditing();

	if (window->drawSpacingHelpers)
		window->ui->mainPreview_v->DrawSpacingHelpers();

	/* --------------------------------------- */

	gs_projection_pop();
	gs_viewport_pop();

	GS_DEBUG_MARKER_END();
}


void OBSBasic::ResizePreview(uint32_t cx, uint32_t cy)
{
	QSize targetSize;
	bool isFixedScaling;
	obs_video_info ovi;

	/* resize preview panel to fix to the top section of the window */
	targetSize = GetPixelSize(ui->preview);

	isFixedScaling = ui->preview->IsFixedScaling();
	obs_get_video_info(&ovi);

	if (isFixedScaling) {
		previewScale = ui->preview->GetScalingAmount();

		ui->preview->ClampScrollingOffsets();

		GetCenterPosFromFixedScale(int(cx), int(cy), targetSize.width() - PREVIEW_EDGE_SIZE * 2,
					   targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX, previewY,
					   previewScale);
		previewX += ui->preview->GetScrollX();
		previewY += ui->preview->GetScrollY();

	} else {
		GetScaleAndCenterPos(int(cx), int(cy), targetSize.width() - PREVIEW_EDGE_SIZE * 2,
				     targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX, previewY, previewScale);
	}

	ui->preview->SetScalingAmount(previewScale);

	previewX += float(PREVIEW_EDGE_SIZE);
	previewY += float(PREVIEW_EDGE_SIZE);
}

void OBSBasic::ResizePreview_V(uint32_t cx, uint32_t cy)
{
	QSize targetSize;
	obs_video_info ovi_v; // Used to get base dimensions for scaling calculations

	if (!ui || !ui->mainPreview_v || !App() || !App()->IsDualOutputActive()) {
		return;
	}

	const obs_video_info* v_ovi_ptr = App()->GetVerticalVideoInfo();
	if (!v_ovi_ptr || v_ovi_ptr->base_width == 0) {
		blog(LOG_WARNING, "ResizePreview_V: Vertical OVI not available or invalid.");
		return;
	}
	ovi_v = *v_ovi_ptr;

	// If cx or cy are 0, use the base dimensions from the vertical OVI
	if (cx == 0 || cy == 0) {
		cx = ovi_v.base_width;
		cy = ovi_v.base_height;
	}

	targetSize = GetPixelSize(ui->mainPreview_v);

	// Use the independent scaling mode for vertical preview, directly from its own UI state
	bool isFixedScaling_v = ui->mainPreview_v->IsFixedScaling();

	if (isFixedScaling_v) {
		// Get the scaling amount directly from the vertical preview widget
		previewScale_v = ui->mainPreview_v->GetScalingAmount();

		// Clamp scrolling offsets specific to the vertical preview
		ui->mainPreview_v->ClampScrollingOffsets();

		GetCenterPosFromFixedScale(int(cx), int(cy), targetSize.width() - PREVIEW_EDGE_SIZE * 2,
					   targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX_v, previewY_v,
					   previewScale_v);
		// Add scroll offsets specific to the vertical preview
		previewX_v += ui->mainPreview_v->GetScrollX();
		previewY_v += ui->mainPreview_v->GetScrollY();

	} else { // Not fixed scaling (fit to window) for vertical preview
		GetScaleAndCenterPos(int(cx), int(cy), targetSize.width() - PREVIEW_EDGE_SIZE * 2,
				     targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX_v, previewY_v, previewScale_v);
	}

	// Set the scaling amount back to the vertical preview widget (might be redundant if GetScalingAmount already reflects user choice)
	ui->mainPreview_v->SetScalingAmount(previewScale_v);

	previewX_v += float(PREVIEW_EDGE_SIZE);
	previewY_v += float(PREVIEW_EDGE_SIZE);

	if (ui->mainPreview_v->isVisible()) {
		ui->mainPreview_v->update();
	}
}


void OBSBasic::on_preview_customContextMenuRequested()
{
	CreateSourcePopupMenu(GetTopSelectedSourceItem(), true);
}

void OBSBasic::on_previewDisabledWidget_customContextMenuRequested()
{
	QMenu popup(this);
	delete previewProjectorMain;

	QAction *action = popup.addAction(QTStr("Basic.Main.PreviewConextMenu.Enable"), this, &OBSBasic::TogglePreview);
	action->setCheckable(true);
	action->setChecked(obs_display_enabled(ui->preview->GetDisplay()));

	previewProjectorMain = new QMenu(QTStr("Projector.Open.Preview"));
	AddProjectorMenuMonitors(previewProjectorMain, this, &OBSBasic::OpenPreviewProjector);
	previewProjectorMain->addSeparator();
	previewProjectorMain->addAction(QTStr("Projector.Window"), this, &OBSBasic::OpenPreviewWindow);

	popup.addMenu(previewProjectorMain);
	popup.exec(QCursor::pos());
}

void OBSBasic::EnablePreviewDisplay(bool enable)
{
	obs_display_set_enabled(ui->preview->GetDisplay(), enable);
	ui->previewContainer->setVisible(enable);
	ui->previewDisabledWidget->setVisible(!enable);
}

void OBSBasic::TogglePreview()
{
	previewEnabled = !previewEnabled;
	EnablePreviewDisplay(previewEnabled);
}

void OBSBasic::EnablePreview()
{
	if (previewProgramMode)
		return;

	previewEnabled = true;
	EnablePreviewDisplay(true);
}

void OBSBasic::DisablePreview()
{
	if (previewProgramMode)
		return;

	previewEnabled = false;
	EnablePreviewDisplay(false);
}

static bool nudge_callback(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_locked(item))
		return true;

	struct vec2 &offset = *static_cast<struct vec2 *>(param);
	struct vec2 pos;

	if (!obs_sceneitem_selected(item)) {
		if (obs_sceneitem_is_group(item)) {
			struct vec3 offset3;
			vec3_set(&offset3, offset.x, offset.y, 0.0f);

			struct matrix4 matrix;
			obs_sceneitem_get_draw_transform(item, &matrix);
			vec4_set(&matrix.t, 0.0f, 0.0f, 0.0f, 1.0f);
			matrix4_inv(&matrix, &matrix);
			vec3_transform(&offset3, &offset3, &matrix);

			struct vec2 new_offset;
			vec2_set(&new_offset, offset3.x, offset3.y);
			obs_sceneitem_group_enum_items(item, nudge_callback, &new_offset);
		}

		return true;
	}

	obs_sceneitem_get_pos(item, &pos);
	vec2_add(&pos, &pos, &offset);
	obs_sceneitem_set_pos(item, &pos);
	return true;
}

void OBSBasic::Nudge(int dist, MoveDir dir)
{
	if (ui->preview->Locked())
		return;

	struct vec2 offset;
	vec2_set(&offset, 0.0f, 0.0f);

	switch (dir) {
	case MoveDir::Up:
		offset.y = (float)-dist;
		break;
	case MoveDir::Down:
		offset.y = (float)dist;
		break;
	case MoveDir::Left:
		offset.x = (float)-dist;
		break;
	case MoveDir::Right:
		offset.x = (float)dist;
		break;
	}

	if (!recent_nudge) {
		recent_nudge = true;
		OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), true);
		std::string undo_data(obs_data_get_json(wrapper));

		nudge_timer = new QTimer;
		QObject::connect(nudge_timer, &QTimer::timeout, [this, &recent_nudge = recent_nudge, undo_data]() {
			OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), true);
			std::string redo_data(obs_data_get_json(rwrapper));

			undo_s.add_action(QTStr("Undo.Transform").arg(obs_source_get_name(GetCurrentSceneSource())),
					  undo_redo, undo_redo, undo_data, redo_data);

			recent_nudge = false;
		});
		connect(nudge_timer, &QTimer::timeout, nudge_timer, &QTimer::deleteLater);
		nudge_timer->setSingleShot(true);
	}

	if (nudge_timer) {
		nudge_timer->stop();
		nudge_timer->start(1000);
	} else {
		blog(LOG_ERROR, "No nudge timer!");
	}

	obs_scene_enum_items(GetCurrentScene(), nudge_callback, &offset);
}

void OBSBasic::on_actionLockPreview_triggered()
{
	ui->preview->ToggleLocked();
	ui->actionLockPreview->setChecked(ui->preview->Locked());
}

void OBSBasic::on_scalingMenu_aboutToShow()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	QAction *action = ui->actionScaleCanvas;
	QString text = QTStr("Basic.MainMenu.Edit.Scale.Canvas");
	text = text.arg(QString::number(ovi.base_width), QString::number(ovi.base_height));
	action->setText(text);

	action = ui->actionScaleOutput;
	text = QTStr("Basic.MainMenu.Edit.Scale.Output");
	text = text.arg(QString::number(ovi.output_width), QString::number(ovi.output_height));
	action->setText(text);
	action->setVisible(!(ovi.output_width == ovi.base_width && ovi.output_height == ovi.base_height));

	UpdatePreviewScalingMenu();
}

void OBSBasic::setPreviewScalingWindow()
{
	ui->preview->SetFixedScaling(false);
	ui->preview->ResetScrollingOffset();

	emit ui->preview->DisplayResized();
}

void OBSBasic::setPreviewScalingCanvas()
{
	ui->preview->SetFixedScaling(true);
	ui->preview->SetScalingLevel(0);

	emit ui->preview->DisplayResized();
}

void OBSBasic::setPreviewScalingOutput()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	ui->preview->SetFixedScaling(true);
	float scalingAmount = float(ovi.output_width) / float(ovi.base_width);
	// log base ZOOM_SENSITIVITY of x = log(x) / log(ZOOM_SENSITIVITY)
	int32_t approxScalingLevel = int32_t(round(log(scalingAmount) / log(ZOOM_SENSITIVITY)));
	ui->preview->SetScalingLevelAndAmount(approxScalingLevel, scalingAmount);
	emit ui->preview->DisplayResized();
}

static void ConfirmColor(SourceTree *sources, const QColor &color, QModelIndexList selectedItems)
{
	for (int x = 0; x < selectedItems.count(); x++) {
		SourceTreeItem *treeItem = sources->GetItemWidget(selectedItems[x].row());
		treeItem->setStyleSheet("background: " + color.name(QColor::HexArgb));
		treeItem->style()->unpolish(treeItem);
		treeItem->style()->polish(treeItem);

		OBSSceneItem sceneItem = sources->Get(selectedItems[x].row());
		OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneItem);
		obs_data_set_int(privData, "color-preset", 1);
		obs_data_set_string(privData, "color", QT_TO_UTF8(color.name(QColor::HexArgb)));
	}
}

void OBSBasic::ColorChange()
{
	QModelIndexList selectedItems = ui->sources->selectionModel()->selectedIndexes();
	QAction *action = qobject_cast<QAction *>(sender());
	QPushButton *colorButton = qobject_cast<QPushButton *>(sender());

	if (selectedItems.count() == 0)
		return;

	if (colorButton) {
		int preset = colorButton->property("bgColor").value<int>();

		for (int x = 0; x < selectedItems.count(); x++) {
			SourceTreeItem *treeItem = ui->sources->GetItemWidget(selectedItems[x].row());
			treeItem->setStyleSheet("");
			treeItem->setProperty("bgColor", preset);
			treeItem->style()->unpolish(treeItem);
			treeItem->style()->polish(treeItem);

			OBSSceneItem sceneItem = ui->sources->Get(selectedItems[x].row());
			OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneItem);
			obs_data_set_int(privData, "color-preset", preset + 1);
			obs_data_set_string(privData, "color", "");
		}

		for (int i = 1; i < 9; i++) {
			stringstream button;
			button << "preset" << i;
			QPushButton *cButton =
				colorButton->parentWidget()->findChild<QPushButton *>(button.str().c_str());
			cButton->setStyleSheet("border: 1px solid black");
		}

		colorButton->setStyleSheet("border: 2px solid black");
	} else if (action) {
		int preset = action->property("bgColor").value<int>();

		if (preset == 1) {
			OBSSceneItem curSceneItem = GetCurrentSceneItem();
			SourceTreeItem *curTreeItem = GetItemWidgetFromSceneItem(curSceneItem);
			OBSDataAutoRelease curPrivData = obs_sceneitem_get_private_settings(curSceneItem);

			int oldPreset = obs_data_get_int(curPrivData, "color-preset");
			const QString oldSheet = curTreeItem->styleSheet();

			auto liveChangeColor = [=](const QColor &color) {
				if (color.isValid()) {
					curTreeItem->setStyleSheet("background: " + color.name(QColor::HexArgb));
				}
			};

			auto changedColor = [=](const QColor &color) {
				if (color.isValid()) {
					ConfirmColor(ui->sources, color, selectedItems);
				}
			};

			auto rejected = [=]() {
				if (oldPreset == 1) {
					curTreeItem->setStyleSheet(oldSheet);
					curTreeItem->setProperty("bgColor", 0);
				} else if (oldPreset == 0) {
					curTreeItem->setStyleSheet("background: none");
					curTreeItem->setProperty("bgColor", 0);
				} else {
					curTreeItem->setStyleSheet("");
					curTreeItem->setProperty("bgColor", oldPreset - 1);
				}

				curTreeItem->style()->unpolish(curTreeItem);
				curTreeItem->style()->polish(curTreeItem);
			};

			QColorDialog::ColorDialogOptions options = QColorDialog::ShowAlphaChannel;

			const char *oldColor = obs_data_get_string(curPrivData, "color");
			const char *customColor = *oldColor != 0 ? oldColor : "#55FF0000";
#ifdef __linux__
			// TODO: Revisit hang on Ubuntu with native dialog
			options |= QColorDialog::DontUseNativeDialog;
#endif

			QColorDialog *colorDialog = new QColorDialog(this);
			colorDialog->setOptions(options);
			colorDialog->setCurrentColor(QColor(customColor));
			connect(colorDialog, &QColorDialog::currentColorChanged, liveChangeColor);
			connect(colorDialog, &QColorDialog::colorSelected, changedColor);
			connect(colorDialog, &QColorDialog::rejected, rejected);
			colorDialog->open();
		} else {
			for (int x = 0; x < selectedItems.count(); x++) {
				SourceTreeItem *treeItem = ui->sources->GetItemWidget(selectedItems[x].row());
				treeItem->setStyleSheet("background: none");
				treeItem->setProperty("bgColor", preset);
				treeItem->style()->unpolish(treeItem);
				treeItem->style()->polish(treeItem);

				OBSSceneItem sceneItem = ui->sources->Get(selectedItems[x].row());
				OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneItem);
				obs_data_set_int(privData, "color-preset", preset);
				obs_data_set_string(privData, "color", "");
			}
		}
	}
}

void OBSBasic::UpdateProjectorHideCursor()
{
	for (size_t i = 0; i < projectors.size(); i++)
		projectors[i]->SetHideCursor();
}

void OBSBasic::UpdateProjectorAlwaysOnTop(bool top)
{
	for (size_t i = 0; i < projectors.size(); i++)
		SetAlwaysOnTop(projectors[i], top);
}

void OBSBasic::ResetProjectors()
{
	OBSDataArrayAutoRelease savedProjectorList = SaveProjectors();
	ClearProjectors();
	LoadSavedProjectors(savedProjectorList);
}

void OBSBasic::UpdatePreviewSafeAreas()
{
	drawSafeAreas = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ShowSafeAreas");
}

void OBSBasic::UpdatePreviewOverflowSettings()
{
	bool hidden = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowHidden");
	bool select = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowSelectionHidden");
	bool always = config_get_bool(App()->GetUserConfig(), "BasicWindow", "OverflowAlwaysVisible");

	ui->preview->SetOverflowHidden(hidden);
	ui->preview->SetOverflowSelectionHidden(select);
	ui->preview->SetOverflowAlwaysVisible(always);
}

static inline QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff);
}

QColor OBSBasic::GetSelectionColor() const
{
	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		return color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "SelectRed"));
	} else {
		return QColor::fromRgb(255, 0, 0);
	}
}

QColor OBSBasic::GetCropColor() const
{
	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		return color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "SelectGreen"));
	} else {
		return QColor::fromRgb(0, 255, 0);
	}
}

QColor OBSBasic::GetHoverColor() const
{
	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		return color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "SelectBlue"));
	} else {
		return QColor::fromRgb(0, 127, 255);
	}
}

void OBSBasic::UpdatePreviewSpacingHelpers()
{
	drawSpacingHelpers = config_get_bool(App()->GetUserConfig(), "BasicWindow", "SpacingHelpersEnabled");
}

float OBSBasic::GetDevicePixelRatio()
{
	return dpi;
}

void OBSBasic::UpdatePreviewControls()
{
	const int scalingLevel = ui->preview->GetScalingLevel();

	if (!ui->preview->IsFixedScaling()) {
		ui->previewXScrollBar->setRange(0, 0);
		ui->previewYScrollBar->setRange(0, 0);

		ui->actionPreviewResetZoom->setEnabled(false);

		return;
	}

	const bool minZoom = scalingLevel == MAX_SCALING_LEVEL;
	const bool maxZoom = scalingLevel == -MAX_SCALING_LEVEL;

	ui->actionPreviewZoomIn->setEnabled(!minZoom);
	ui->previewZoomInButton->setEnabled(!minZoom);

	ui->actionPreviewZoomOut->setEnabled(!maxZoom);
	ui->previewZoomOutButton->setEnabled(!maxZoom);

	ui->actionPreviewResetZoom->setEnabled(scalingLevel != 0);
}

void OBSBasic::UpdatePreviewControls_V()
{
	if (!ui || !ui->mainPreview_v) return;

	const int scalingLevel_v = ui->mainPreview_v->GetScalingLevel();

	if (!ui->mainPreview_v->IsFixedScaling()) {
		ui->previewXScrollBar_v->setRange(0, 0);
		ui->previewYScrollBar_v->setRange(0, 0);
		// ui->actionPreviewResetZoom_v->setEnabled(false); // If a separate action exists
		return;
	}

	const bool minZoom_v = scalingLevel_v == MAX_SCALING_LEVEL;
	const bool maxZoom_v = scalingLevel_v == -MAX_SCALING_LEVEL;

	// Assuming actions like ui->actionPreviewZoomIn_v exist or these are handled contextually
	// ui->actionPreviewZoomIn_v->setEnabled(!minZoom_v);
	ui->previewZoomInButton_v->setEnabled(!minZoom_v);

	// ui->actionPreviewZoomOut_v->setEnabled(!maxZoom_v);
	ui->previewZoomOutButton_v->setEnabled(!maxZoom_v);

	// ui->actionPreviewResetZoom_v->setEnabled(scalingLevel_v != 0);
}


void OBSBasic::PreviewScalingModeChanged(int value)
{
	switch (value) {
	case 0:
		setPreviewScalingWindow();
		break;
	case 1:
		setPreviewScalingCanvas();
		break;
	case 2:
		setPreviewScalingOutput();
		break;
	};
}

void OBSBasic::PreviewScalingModeChanged_V(int value)
{
	if (!ui || !ui->mainPreview_v || loading) return;

	QString scaleType;
	switch (value) {
	case 0: // Scale to Window
		setPreviewScalingWindow_V();
		scaleType = "Window";
		break;
	case 1: // Scale to Canvas
		setPreviewScalingCanvas_V();
		scaleType = "Canvas";
		break;
	case 2: // Scale to Output
		setPreviewScalingOutput_V();
		scaleType = "Output";
		break;
	default: // Scale to Window (same as case 0)
		setPreviewScalingWindow_V();
		scaleType = "Window";
	}
	config_set_string(App()->GetUserConfig(), "BasicWindow", "PreviewVScaleType", scaleType.toUtf8().constData());
	SetWidgetChanged(ui->previewScalingMode_v);
	UpdatePreviewControls_V();
}

void OBSBasic::PreviewScalePercentChanged_V(int value)
{
	if (!ui || !ui->mainPreview_v || loading) return;

	// This slot is intended if ui->previewScalePercent_v were a QSpinBox.
	// Currently, it's a QLabel updated by OBSBasicPreview::scalingChanged signal.
	// If it were a QSpinBox, the logic would be:
	// float scaleValue = static_cast<float>(value) / 100.0f;
	// ui->mainPreview_v->SetScalingAmount(scaleValue); // This might trigger scalingChanged from OBSBasicPreview
	// config_set_double(App()->GetUserConfig(), "BasicWindow", "PreviewVFixedScale", scaleValue);
	// SetWidgetChanged(ui->previewScalePercent_v); // Assuming previewScalePercent_v is the QSpinBox
	// ResizePreview_V(); // Trigger a resize to apply the new scale
	// UpdatePreviewControls_V();

	UNUSED_PARAMETER(value); // Keep if it's a label
}

void OBSBasic::setPreviewScalingWindow_V()
{
	if (!ui || !ui->mainPreview_v) return;
	ui->mainPreview_v->SetFixedScaling(false);
	ui->mainPreview_v->ResetScrollingOffset();
	emit ui->mainPreview_v->DisplayResized(); // Force redraw and recalculation
	config_set_string(App()->GetUserConfig(), "BasicWindow", "PreviewVScaleType", "Window");
}

void OBSBasic::setPreviewScalingCanvas_V()
{
	if (!ui || !ui->mainPreview_v) return;
	ui->mainPreview_v->SetFixedScaling(true);
	ui->mainPreview_v->SetScalingLevel(0); // Resets to 100% of canvas
	emit ui->mainPreview_v->DisplayResized();
	config_set_string(App()->GetUserConfig(), "BasicWindow", "PreviewVScaleType", "Canvas");
}

void OBSBasic::setPreviewScalingOutput_V()
{
	if (!ui || !ui->mainPreview_v || !App()) return;
	const obs_video_info* v_ovi = App()->GetVerticalVideoInfo();
	if (!v_ovi || v_ovi->base_width == 0) return;

	ui->mainPreview_v->SetFixedScaling(true);
	float scalingAmount = float(v_ovi->output_width) / float(v_ovi->base_width);
	int32_t approxScalingLevel = int32_t(round(log(scalingAmount) / log(ZOOM_SENSITIVITY)));
	ui->mainPreview_v->SetScalingLevelAndAmount(approxScalingLevel, scalingAmount);
	emit ui->mainPreview_v->DisplayResized();
	config_set_string(App()->GetUserConfig(), "BasicWindow", "PreviewVScaleType", "Output");
}

void OBSBasic::UpdatePreviewScalingMenu_V()
{
    if (!ui || !ui->mainPreview_v) return;

    bool fixedScaling = ui->mainPreview_v->IsFixedScaling();
    float scalingAmount = ui->mainPreview_v->GetScalingAmount();

    // Assuming similar QActions (actionScaleWindow_v, etc.) would exist for a vertical preview context menu
    // For now, this function might not be directly used if there's no separate context menu for vertical scaling.
    // If ui->previewScalingMode_v (the QComboBox) is the primary control, this menu update logic might be less critical.

    // Example if actions existed:
    // if (!fixedScaling) {
    //     ui->actionScaleWindow_v->setChecked(true);
    //     ui->actionScaleCanvas_v->setChecked(false);
    //     ui->actionScaleOutput_v->setChecked(false);
    //     return;
    // }
    // const obs_video_info* ovi_v = App()->GetVerticalVideoInfo();
    // if (!ovi_v || ovi_v->base_width == 0) return;
    // ui->actionScaleWindow_v->setChecked(false);
    // ui->actionScaleCanvas_v->setChecked(scalingAmount == 1.0f);
    // ui->actionScaleOutput_v->setChecked(scalingAmount == float(ovi_v->output_width) / float(ovi_v->base_width));
	UNUSED_PARAMETER(fixedScaling);
	UNUSED_PARAMETER(scalingAmount);
}
