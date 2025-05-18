#pragma once

#include <QFrame>
#include <QScopedPointer>
#include <QPointer>

#include <obs.hpp>

class IconLabel;
class OBSSourceLabel;
class QHBoxLayout;
class QLineEdit;

class SceneTreeItem : public QFrame {
	Q_OBJECT

	friend class SceneTree;

private:
	QScopedPointer<IconLabel> previewIcon;
	QScopedPointer<IconLabel> programIcon;
	QScopedPointer<QLineEdit> editor;

	QPointer<OBSSourceLabel> sceneLabel;
	QPointer<QHBoxLayout> layout;

	OBSScene scene;

	bool IsEditing();

private slots:
	void UpdateIcons();
	void StudioModeEnabled(bool enabled);

	void EnterEditMode();
	void ExitEditMode(bool save);

public:
	explicit SceneTreeItem(OBSScene scene_, QWidget *parent = nullptr);

	OBSScene GetScene();

protected:
	virtual bool eventFilter(QObject *object, QEvent *event) override;
};
