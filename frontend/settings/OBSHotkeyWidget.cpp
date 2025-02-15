/******************************************************************************
 Copyright (C) 2014-2015 by Ruwen Hahn <palana@stunned.de>
 
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

#include "OBSHotkeyWidget.hpp"
#include "OBSHotkeyLabel.hpp"

#include <OBSApp.hpp>

#include <QEnterEvent>

#include "moc_OBSHotkeyWidget.cpp"

void OBSHotkeyWidget::SetKeyCombinations(const std::vector<obs_key_combination_t> &combos)
{
	if (combos.empty())
		AddEdit({0, OBS_KEY_NONE});

	for (auto combo : combos)
		AddEdit(combo);
}

bool OBSHotkeyWidget::Changed() const
{
	return changed || std::any_of(begin(edits), end(edits), [](OBSHotkeyEdit *edit) { return edit->changed; });
}

void OBSHotkeyWidget::Apply()
{
	for (auto &edit : edits) {
		edit->original = edit->key;
		edit->changed = false;
	}

	changed = false;

	for (auto &revertButton : revertButtons)
		revertButton->setEnabled(false);
}

void OBSHotkeyWidget::GetCombinations(std::vector<obs_key_combination_t> &combinations) const
{
	combinations.clear();
	for (auto &edit : edits)
		if (!obs_key_combination_is_empty(edit->key))
			combinations.emplace_back(edit->key);
}

void OBSHotkeyWidget::Save()
{
	std::vector<obs_key_combination_t> combinations;
	Save(combinations);
}

void OBSHotkeyWidget::Save(std::vector<obs_key_combination_t> &combinations)
{
	GetCombinations(combinations);
	Apply();

	auto AtomicUpdate = [&]() {
		ignoreChangedBindings = true;

		obs_hotkey_load_bindings(id, combinations.data(), combinations.size());

		ignoreChangedBindings = false;
	};
	using AtomicUpdate_t = decltype(&AtomicUpdate);

	obs_hotkey_update_atomic([](void *d) { (*static_cast<AtomicUpdate_t>(d))(); },
				 static_cast<void *>(&AtomicUpdate));
}

void OBSHotkeyWidget::AddEdit(obs_key_combination combo, int idx)
{
	auto edit = new OBSHotkeyEdit(parentWidget(), combo, settings);
	edit->setToolTip(toolTip);

	auto revert = new QPushButton;
	revert->setProperty("class", "icon-revert");
	revert->setToolTip(QTStr("Revert"));
	revert->setEnabled(false);

	auto clear = new QPushButton;
	clear->setProperty("class", "icon-clear");
	clear->setToolTip(QTStr("Clear"));
	clear->setEnabled(!obs_key_combination_is_empty(combo));

	QObject::connect(edit, &OBSHotkeyEdit::KeyChanged, [=](obs_key_combination_t new_combo) {
		clear->setEnabled(!obs_key_combination_is_empty(new_combo));
		revert->setEnabled(edit->original != new_combo);
	});

	auto add = new QPushButton;
	add->setProperty("class", "icon-plus");
	add->setToolTip(QTStr("Add"));

	auto remove = new QPushButton;
	remove->setProperty("class", "icon-trash");
	remove->setToolTip(QTStr("Remove"));
	remove->setEnabled(removeButtons.size() > 0);

	auto CurrentIndex = [&, remove] {
		auto res = std::find(begin(removeButtons), end(removeButtons), remove);
		return std::distance(begin(removeButtons), res);
	};

	QObject::connect(add, &QPushButton::clicked, [&, CurrentIndex] {
		AddEdit({0, OBS_KEY_NONE}, CurrentIndex() + 1);
	});

	QObject::connect(remove, &QPushButton::clicked, [&, CurrentIndex] { RemoveEdit(CurrentIndex()); });

	QHBoxLayout *subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 2, 0, 2);
	subLayout->addWidget(edit);
	subLayout->addWidget(revert);
	subLayout->addWidget(clear);
	subLayout->addWidget(add);
	subLayout->addWidget(remove);

	if (removeButtons.size() == 1)
		removeButtons.front()->setEnabled(true);

	if (idx != -1) {
		revertButtons.insert(begin(revertButtons) + idx, revert);
		removeButtons.insert(begin(removeButtons) + idx, remove);
		edits.insert(begin(edits) + idx, edit);
	} else {
		revertButtons.emplace_back(revert);
		removeButtons.emplace_back(remove);
		edits.emplace_back(edit);
	}

	layout()->insertLayout(idx, subLayout);

	QObject::connect(revert, &QPushButton::clicked, edit, &OBSHotkeyEdit::ResetKey);
	QObject::connect(clear, &QPushButton::clicked, edit, &OBSHotkeyEdit::ClearKey);

	QObject::connect(edit, &OBSHotkeyEdit::KeyChanged, [&](obs_key_combination) { emit KeyChanged(); });
	QObject::connect(edit, &OBSHotkeyEdit::SearchKey, [=](obs_key_combination combo) { emit SearchKey(combo); });
}

void OBSHotkeyWidget::RemoveEdit(size_t idx, bool signal)
{
	auto &edit = *(begin(edits) + idx);
	if (!obs_key_combination_is_empty(edit->original) && signal) {
		changed = true;
	}

	revertButtons.erase(begin(revertButtons) + idx);
	removeButtons.erase(begin(removeButtons) + idx);
	edits.erase(begin(edits) + idx);

	auto item = layout()->takeAt(static_cast<int>(idx));
	QLayoutItem *child = nullptr;
	while ((child = item->layout()->takeAt(0))) {
		delete child->widget();
		delete child;
	}
	delete item;

	if (removeButtons.size() == 1)
		removeButtons.front()->setEnabled(false);

	emit KeyChanged();
}

void OBSHotkeyWidget::BindingsChanged(void *data, calldata_t *param)
{
	auto widget = static_cast<OBSHotkeyWidget *>(data);
	auto key = static_cast<obs_hotkey_t *>(calldata_ptr(param, "key"));

	QMetaObject::invokeMethod(widget, "HandleChangedBindings", Q_ARG(obs_hotkey_id, obs_hotkey_get_id(key)));
}

void OBSHotkeyWidget::HandleChangedBindings(obs_hotkey_id id_)
{
	if (ignoreChangedBindings || id != id_)
		return;

	std::vector<obs_key_combination_t> bindings;
	auto LoadBindings = [&](obs_hotkey_binding_t *binding) {
		if (obs_hotkey_binding_get_hotkey_id(binding) != id)
			return;

		auto get_combo = obs_hotkey_binding_get_key_combination;
		bindings.push_back(get_combo(binding));
	};
	using LoadBindings_t = decltype(&LoadBindings);

	obs_enum_hotkey_bindings(
		[](void *data, size_t, obs_hotkey_binding_t *binding) {
			auto LoadBindings = *static_cast<LoadBindings_t>(data);
			LoadBindings(binding);
			return true;
		},
		static_cast<void *>(&LoadBindings));

	while (edits.size() > 0)
		RemoveEdit(edits.size() - 1, false);

	SetKeyCombinations(bindings);
}

void OBSHotkeyWidget::enterEvent(QEnterEvent *event)
{
	if (!label)
		return;

	event->accept();
	label->highlightPair(true);
}

void OBSHotkeyWidget::leaveEvent(QEvent *event)
{
	if (!label)
		return;

	event->accept();
	label->highlightPair(false);
}
