#pragma once

#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QWidget>

class AIControlDock : public QWidget {
	Q_OBJECT

public:
	explicit AIControlDock(QWidget *parent = nullptr);
	~AIControlDock() override;

private slots:
	void onStartStop();
	void onEngineChanged(int index);
	void onTranslateToggled(bool checked);
	void onCorrectToggled(bool checked);

private:
	void refreshUiState();
	void appendCaption(const QString &text);

	QComboBox *engineCombo_ = nullptr;
	QPushButton *startStopButton_ = nullptr;
	QPushButton *translateButton_ = nullptr;
	QPushButton *correctButton_ = nullptr;
	QPlainTextEdit *preview_ = nullptr;
	QLabel *statusLabel_ = nullptr;
};
