#pragma once

#include <QWizardPage>
#include <QScrollArea>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "pre-stream-current-settings.hpp"

/*
Shows user encoder configuration options and allows them to select and apply 
each. For exmaple can apply a resolution limit but opt-out of using b-frames 
even if recommended. 
*/

namespace StreamWizard {

class SelectionPage : public QWizardPage {
	Q_OBJECT

public:
	SelectionPage(QWidget *parent = nullptr);

	// Sets data to layout scrollbox
	void setSettingsMap(QSharedPointer<SettingsMap> settingsMap);

private:
	// Main UI setup, setSettingsMap finished layout with data
	void setupLayout();

	void mapContainsVariant();
	// Adds a row only if map contains values for it
	void addRow(QString title, QString value, const char *mapKey);

	// Data
	QSharedPointer<SettingsMap> settingsMap_;

	// Layouts and subwidgets
	QLabel *descriptionLabel_;
	QScrollArea *scrollArea_;
	QWidget *scrollBoxWidget_;
	QVBoxLayout *scrollVBoxLayout_;

private slots:
	void checkboxRowChanged(const char *propertyKey, bool selected);

}; // class SelectionPage

} // namespace StreamWizard
