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

#include <components/SceneTree.hpp>
#include <utility/mediators/CanvasMediator.hpp>

#include <QFrame>
#include <QPointer>

class SceneList : public QFrame {
	Q_OBJECT

public:
	explicit SceneList(QWidget *parent = nullptr);
	~SceneList();

protected:
	void createContextMenu();
	void createContextMenuForItem(QListWidgetItem *item);

private:
	std::vector<OBSSignal> signalHandlers;

	QPointer<QMenu> contextMenu;
	QPersistentModelIndex focusContextIndex{};

	SceneTree *sceneTree{};

	QToolBar *scenesToolbar{nullptr};
	QAction *actionAddScene{nullptr};
	QAction *actionSceneFilters{nullptr};
	QAction *actionMoveUp{nullptr};
	QAction *actionMoveDown{nullptr};

	QAction *actionRenameScene{nullptr};
	QAction *actionRemoveScene{nullptr};

	void recreateOptionsMenu();
	QPointer<QMenu> optionsMenu;
	QPushButton *optionsButton{nullptr};

	QPointer<OBS::CanvasMediator> mediator;

	std::vector<QMetaObject::Connection> connections;

	void applyGridMode(bool enable);

signals:
	void sceneSelected(QString sceneUuid);
	void sceneReorderFinished();

private slots:
	void onItemEditorClosed(QWidget *editor);
	void onSceneRenamed(const QString &uuid);

	void updateLayoutFromSettings();

public slots:
	void attachMediator(OBS::CanvasMediator *mediator);

	void onContextMenuRequested(const QPoint &pos);

	void triggerCreateScene();
	void triggerDeleteScene(const OBSScene &scene);
	void triggerDuplicateScene(const OBSScene &scene);

	void beginInlineRename();
	void onItemDoubleClicked(QListWidgetItem *item);

	void openFiltersForSelectedScene();
	void triggerDeleteForSelected();
	void triggerDuplicateForSelected();

	void triggerMoveSelectedToTop();
	void triggerMoveSelectedToBottom();
	void triggerMoveSelectedUp();
	void triggerMoveSelectedDown();

	void applySelectionFromMediator(const QString &uuid);
	void rebuildFromMediator();
	void onSceneSelectionChanged();
	void onSceneDropped();
};
