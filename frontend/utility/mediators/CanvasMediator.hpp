/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <obs.hpp>
#include <qt-wrappers.hpp>

#include <QInputDialog>
#include <QObject>
#include <QPointer>

#include <unordered_map>

class OBSApp;
namespace OBS {

class CanvasMediator : public QObject {
	Q_OBJECT

	struct State {
		std::string previewSceneUuid;
		std::string programSceneUuid;

		struct SceneOrderInfo {
			std::string uuid;
			int order{0};
		};

		std::vector<std::string> orderedSceneUuids;
		std::unordered_map<std::string, SceneOrderInfo> sceneOrderData;

		int getSceneOrder(const std::string &uuid)
		{
			auto it = sceneOrderData.find(uuid);
			if (it != sceneOrderData.end()) {
				return it->second.order;
			}

			return 0;
		};
	} canvas_state_;

	/* -------------------------------------
	 * MARK: - Canvas Mediator
	 * -------------------------------------
	 */
public:
	static CanvasMediator *create(obs_canvas_t *canvas);
	~CanvasMediator();

	std::string uuid;
	[[nodiscard]] OBSCanvas getCanvas() const;

	// Helpers
	[[nodiscard]] std::vector<std::string> getScenes() const;

	// Requests
	void requestCreateScene();
	void requestDuplicateScene(const OBSScene &scene);
	void requestDeleteScene(const OBSScene &scene);
	void requestRenameScene(const OBSScene &scene, const QString &name);

	// Data Handling
	[[nodiscard]] OBSDataAutoRelease serializeStateToOBSData() const;
	void loadStateFromOBSData(obs_data_t *data);

private:
	CanvasMediator(OBSApp *parent, obs_canvas_t *canvas);

	QPointer<QDialog> activeDialog;
	QInputDialog *createInputDialog();
	void handleCreateScene(const QString &name);
	void handleDuplicateName(const OBSScene &scene, const QString &name);
	void handleDeleteScene(const OBSScene &scene);

	std::vector<OBSSignal> signalHandlers;

	static void obsCanvasSourceCreated(void *data, calldata_t *params);
	static void obsCanvasSourceRemoved(void *data, calldata_t *params);
	static void obsSourceRenamed(void *data, calldata_t *params);

	// FIXME: Temporary ownership, should probably be handled by a SceneMediator class
	std::unordered_map<std::string, std::vector<OBSSignal>> sourceSignals;

	std::unordered_map<obs_hotkey_id, std::string> hotkeyScenes;
	void registerSceneHotkey(const std::string &uuid);
	void unregisterSceneHotkey(const std::string &uuid);
	std::optional<std::string> getSceneForHotkeyId(obs_hotkey_id id);

	/* -------------------------------------
	 * MARK: - Canvas State
	 * -------------------------------------
	 */
public:
	void reorderSceneUp(const OBSScene &scene);
	void reorderSceneDown(const OBSScene &scene);
	void reorderSceneToTop(const OBSScene &scene);
	void reorderSceneToBottom(const OBSScene &scene);
	bool reorderSceneToIndex(const OBSScene &scene, int index);

	void setPreviewScene(const OBSScene &scene);
	[[nodiscard]] OBSScene getPreviewScene() const;
	void setProgramScene(const OBSScene &scene);
	[[nodiscard]] OBSScene getProgramScene() const;

	[[nodiscard]] std::unordered_map<std::string, State::SceneOrderInfo> getSceneOrderData() const;
	void rebuildSceneOrderFromData(const OBSDataArray &data);

private:
	void normalizeSceneOrder();

	void ensureValidPreviewScene();

signals:
	void sceneAdded(QString uuid);
	void sceneRemoved(QString uuid);
	void sceneRenamed(QString uuid);

	void sceneOrderChanged();

	void previewChanged(QString uuid);
	void programChanged(QString uuid);

private slots:
	void onSceneRemoved(const QString &uuid);
	void onSourceRenamed(const QString &uuid, const QString &newName);

public slots:
	void onSceneAdded(const QString &uuid);
	void commitSceneOrderChanges();
};
} // namespace OBS
