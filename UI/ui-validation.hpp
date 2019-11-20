#pragma once

#include <QObject>
#include <QWidget>

class UIValidation : public QObject {
	Q_OBJECT

public:
	// Shows alert box notifying there are no video sources
	// Returns true if user clicks "Yes"
	// Returns false if user clicks "No"
	// Blocks UI
	static bool NoSourcesConfirmation(QWidget *parent);
};
