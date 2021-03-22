#pragma once

#include <QString>

#include <deque>
#include <functional>
#include <string>
#include <memory>

#include "ui_OBSBasic.h"

typedef std::function<void(const std::string &data)> undo_redo_cb;
typedef std::function<void(bool is_undo)> func;
typedef std::unique_ptr<Ui::OBSBasic> &ui_ptr;

struct undo_redo_t {
	QString name;
	std::string undo_data;
	std::string redo_data;
	undo_redo_cb undo;
	undo_redo_cb redo;
	func d;
};

class undo_stack {
private:
	ui_ptr ui;
	std::deque<undo_redo_t> undo_items;
	std::deque<undo_redo_t> redo_items;
	bool disabled = false;

	void clear_redo();

public:
	undo_stack(ui_ptr ui);

	void enable_undo_redo();
	void disable_undo_redo();

	void release();
	void add_action(const QString &name, undo_redo_cb undo,
			undo_redo_cb redo, std::string undo_data,
			std::string redo_data, func d);
	void undo();
	void redo();
};
