#include "undo-stack-obs.hpp"

#include <util/util.hpp>

undo_stack::undo_stack(ui_ptr ui) : ui(ui) {}

void undo_stack::release()
{
	for (auto f : undo_items)
		if (f.d)
			f.d(true);

	for (auto f : redo_items)
		if (f.d)
			f.d(false);
}

void undo_stack::add_action(const QString &name, undo_redo_cb undo,
			    undo_redo_cb redo, std::string undo_data,
			    std::string redo_data, func d)
{
	undo_redo_t n = {name, undo_data, redo_data, undo, redo, d};

	undo_items.push_front(n);
	clear_redo();

	ui->actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(name));
	ui->actionMainUndo->setEnabled(true);

	ui->actionMainRedo->setText(QTStr("Undo.Redo"));
	ui->actionMainRedo->setDisabled(true);
}

void undo_stack::undo()
{
	if (undo_items.size() == 0 || disabled)
		return;

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
		ui->actionMainUndo->setText(
			QTStr("Undo.Item.Undo").arg(undo_items.front().name));
	}
}

void undo_stack::redo()
{
	if (redo_items.size() == 0 || disabled)
		return;

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
		ui->actionMainRedo->setText(
			QTStr("Undo.Item.Redo").arg(redo_items.front().name));
	}
}

void undo_stack::enable_undo_redo()
{
	disabled = false;

	ui->actionMainUndo->setDisabled(false);
	ui->actionMainRedo->setDisabled(false);
}

void undo_stack::disable_undo_redo()
{
	disabled = true;

	ui->actionMainUndo->setDisabled(true);
	ui->actionMainRedo->setDisabled(true);
}

void undo_stack::clear_redo()
{
	for (auto f : redo_items)
		if (f.d)
			f.d(false);

	redo_items.clear();
}
