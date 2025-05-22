#include "SceneTreeItem.hpp"
#include <icon-label.hpp>
#include <qt-wrappers.hpp>
#include <widgets/OBSBasic.hpp>
#include <components/OBSSourceLabel.hpp>

#include <QHBoxLayout>
#include <QLineEdit>

#include "moc_SceneTreeItem.cpp"

SceneTreeItem::SceneTreeItem(OBSScene scene_, QWidget *parent) : QFrame(parent), scene(scene_)
{
	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	OBSSource source = obs_scene_get_source(GetScene());
	sceneLabel = new OBSSourceLabel(source);

	layout->addWidget(sceneLabel);
	layout->addStretch();
	setLayout(layout);

	UpdateIcons();

	connect(OBSBasic::Get(), &OBSBasic::StudioModeIndicatorsUpdated, this, &SceneTreeItem::UpdateIcons);
	connect(OBSBasic::Get(), &OBSBasic::PreviewProgramModeChanged, this, &SceneTreeItem::StudioModeEnabled);
}

void SceneTreeItem::UpdateIcons()
{
	OBSBasic *main = OBSBasic::Get();

	if (main->GetCurrentScene() == GetScene()) {
		previewIcon.reset(new IconLabel());
		previewIcon.data()->setIconSize(16);
		previewIcon.data()->setProperty("class", "icon-edit");

		layout->insertWidget(0, previewIcon.data());
	} else {
		previewIcon.reset();
	}

	if (obs_scene_from_source(main->GetProgramSource()) == GetScene()) {
		programIcon.reset(new IconLabel());
		programIcon.data()->setIconSize(16);
		programIcon.data()->setProperty("class", "icon-stream");

		layout->insertWidget(!!previewIcon ? 1 : 0, programIcon.data());
	} else {
		programIcon.reset();
	}
}

void SceneTreeItem::StudioModeEnabled(bool enabled)
{
	if (!enabled) {
		previewIcon.reset();
		programIcon.reset();
	}
}

OBSScene SceneTreeItem::GetScene()
{
	return scene;
}

bool SceneTreeItem::IsEditing()
{
	return editor != nullptr;
}

void SceneTreeItem::EnterEditMode()
{
	setFocusPolicy(Qt::StrongFocus);
	int index = layout->indexOf(sceneLabel);
	layout->removeWidget(sceneLabel);
	editor.reset(new QLineEdit(sceneLabel->text()));
	editor->setStyleSheet("background: none");
	editor->selectAll();
	editor->installEventFilter(this);
	layout->insertWidget(index, editor.data());
	setFocusProxy(editor.data());
}

void SceneTreeItem::ExitEditMode(bool save)
{
	std::string name = QT_TO_UTF8(editor->text().trimmed());

	setFocusProxy(nullptr);
	int index = layout->indexOf(editor.data());
	layout->removeWidget(editor.data());
	editor.reset();
	setFocusPolicy(Qt::NoFocus);
	layout->insertWidget(index, sceneLabel);
	setFocus();

	if (!save)
		return;

	OBSSource source = obs_scene_get_source(GetScene());

	const char *prevName = obs_source_get_name(source);
	if (name == prevName)
		return;

	OBSSourceAutoRelease foundSource = obs_get_source_by_name(name.c_str());

	if (foundSource || name.empty()) {
		sceneLabel->setText(QT_UTF8(prevName));

		if (foundSource) {
			OBSMessageBox::warning(OBSBasic::Get(), QTStr("NameExists.Title"), QTStr("NameExists.Text"));
		} else if (name.empty()) {
			OBSMessageBox::warning(OBSBasic::Get(), QTStr("NoNameEntered.Title"),
					       QTStr("NoNameEntered.Text"));
		}
	} else {
		auto undo = [prev = std::string(prevName)](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
			obs_source_set_name(source, prev.c_str());
		};

		auto redo = [name](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
			obs_source_set_name(source, name.c_str());
		};

		std::string source_uuid(obs_source_get_uuid(source));
		OBSBasic::Get()->undo_s.add_action(QTStr("Undo.Rename").arg(name.c_str()), undo, redo, source_uuid,
						   source_uuid);

		obs_source_set_name(source, name.c_str());
	}
}

bool SceneTreeItem::eventFilter(QObject *object, QEvent *event)
{
	if (editor.data() != object)
		return false;

	if (LineEditCanceled(event)) {
		QMetaObject::invokeMethod(this, "ExitEditMode", Qt::QueuedConnection, Q_ARG(bool, false));
		return true;
	}
	if (LineEditChanged(event)) {
		QMetaObject::invokeMethod(this, "ExitEditMode", Qt::QueuedConnection, Q_ARG(bool, true));
		return true;
	}

	return false;
}
