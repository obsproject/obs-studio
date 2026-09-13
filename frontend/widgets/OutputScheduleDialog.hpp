#pragma once

#include <utility/OutputSchedule.hpp>
#include <QDialog>
#include <functional>

// The save callback commits immediately; a failed save leaves the model untouched.
void ShowOutputScheduleDialog(QWidget *parent, const QList<OutputSchedule> &entries,
			      const std::function<bool(const QList<OutputSchedule> &)> &save,
			      const std::function<QString(const char *)> &tr);
