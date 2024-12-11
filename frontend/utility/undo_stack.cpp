#include "undo_stack.hpp"

#include <OBSApp.hpp>

#include "moc_undo_stack.cpp"

#define MAX_STACK_SIZE 5000

undo_stack::undo_stack(ui_ptr ui) : ui(ui)
{
	QObject::connect(&repeat_reset_timer, &QTimer::timeout, this, &undo_stack::reset_repeatable_state);
	repeat_reset_timer.setSingleShot(true);
	repeat_reset_timer.setInterval(3000);
}

void undo_stack::reset_repeatable_state()
{
	last_is_repeatable = false;
}

void undo_stack::clear()
{
	undo_items.clear();
	redo_items.clear();
	last_is_repeatable = false;

	ui->actionMainUndo->setText(QTStr("Undo.Undo"));
	ui->actionMainRedo->setText(QTStr("Undo.Redo"));

	ui->actionMainUndo->setDisabled(true);
	ui->actionMainRedo->setDisabled(true);
}

void undo_stack::add_action(const QString &name, const undo_redo_cb &undo, const undo_redo_cb &redo,
			    const std::string &undo_data, const std::string &redo_data, bool repeatable)
{
	if (!is_enabled())
		return;

	while (undo_items.size() >= MAX_STACK_SIZE) {
		undo_redo_t item = undo_items.back();
		undo_items.pop_back();
	}

	if (repeatable) {
		repeat_reset_timer.start();
	}

	if (last_is_repeatable && repeatable && name == undo_items[0].name) {
		undo_items[0].redo = redo;
		undo_items[0].redo_data = redo_data;
		return;
	}

	undo_redo_t n = {name, undo_data, redo_data, undo, redo};

	last_is_repeatable = repeatable;
	undo_items.push_front(n);
	clear_redo();

	ui->actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(name));
	ui->actionMainUndo->setEnabled(true);

	ui->actionMainRedo->setText(QTStr("Undo.Redo"));
	ui->actionMainRedo->setDisabled(true);
}

void undo_stack::undo()
{
	if (undo_items.size() == 0 || !is_enabled())
		return;

	last_is_repeatable = false;

	undo_redo_t temp = undo_items.front();
	temp.undo(temp.undo_data);
	redo_items.push_front(temp);
	undo_items.pop_front();

	ui->actionMainRedo->setText(QTStr("Undo.Item.Redo").arg(temp.name));
	ui->actionMainRedo->setEnabled(true);

	if (undo_items.size() == 0) {
		ui->actionMainUndo->setDisabled(true);
		ui->actionMainUndo->setText(QTStr("Undo.Undo"));
	} else {
		ui->actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(undo_items.front().name));
	}
}

void undo_stack::redo()
{
	if (redo_items.size() == 0 || !is_enabled())
		return;

	last_is_repeatable = false;

	undo_redo_t temp = redo_items.front();
	temp.redo(temp.redo_data);
	undo_items.push_front(temp);
	redo_items.pop_front();

	ui->actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(temp.name));
	ui->actionMainUndo->setEnabled(true);

	if (redo_items.size() == 0) {
		ui->actionMainRedo->setDisabled(true);
		ui->actionMainRedo->setText(QTStr("Undo.Redo"));
	} else {
		ui->actionMainRedo->setText(QTStr("Undo.Item.Redo").arg(redo_items.front().name));
	}
}

void undo_stack::enable_internal()
{
	last_is_repeatable = false;

	ui->actionMainUndo->setDisabled(false);
	if (redo_items.size() > 0)
		ui->actionMainRedo->setDisabled(false);
}

void undo_stack::disable_internal()
{
	last_is_repeatable = false;

	ui->actionMainUndo->setDisabled(true);
	ui->actionMainRedo->setDisabled(true);
}

void undo_stack::enable()
{
	enabled = true;
	if (is_enabled())
		enable_internal();
}

void undo_stack::disable()
{
	if (is_enabled())
		disable_internal();
	enabled = false;
}

void undo_stack::push_disabled()
{
	if (is_enabled())
		disable_internal();
	disable_refs++;
}

void undo_stack::pop_disabled()
{
	disable_refs--;
	if (is_enabled())
		enable_internal();
}

void undo_stack::clear_redo()
{
	redo_items.clear();
}
