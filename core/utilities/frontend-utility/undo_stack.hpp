#pragma once

#include "ui_OBSBasic.h"

#include <QObject>
#include <QString>
#include <QTimer>

#include <deque>
#include <string>

class undo_stack : public QObject {
	Q_OBJECT

	typedef std::function<void(const std::string &data)> undo_redo_cb;
	typedef std::function<void(bool is_undo)> func;
	typedef std::unique_ptr<Ui::OBSBasic> &ui_ptr;

	struct undo_redo_t {
		QString name;
		std::string undo_data;
		std::string redo_data;
		undo_redo_cb undo;
		undo_redo_cb redo;
	};

	ui_ptr ui;
	std::deque<undo_redo_t> undo_items;
	std::deque<undo_redo_t> redo_items;
	int disable_refs = 0;
	bool enabled = true;
	bool last_is_repeatable = false;

	QTimer repeat_reset_timer;

	inline bool is_enabled() const { return !disable_refs && enabled; }

	void enable_internal();
	void disable_internal();
	void clear_redo();

private slots:
	void reset_repeatable_state();

public:
	undo_stack(ui_ptr ui);

	void enable();
	void disable();
	void push_disabled();
	void pop_disabled();

	void clear();
	void add_action(const QString &name, const undo_redo_cb &undo, const undo_redo_cb &redo,
			const std::string &undo_data, const std::string &redo_data, bool repeatable = false);
	void undo();
	void redo();
};
