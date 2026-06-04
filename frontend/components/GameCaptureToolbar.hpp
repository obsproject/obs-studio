#pragma once

#include "SourceToolbar.hpp"

class Ui_GameCaptureToolbar;

class GameCaptureToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_GameCaptureToolbar> ui;

	void UpdateWindowVisibility();

public:
	GameCaptureToolbar(QWidget *parent, OBSSource source);
	~GameCaptureToolbar();

public slots:
	void on_mode_currentIndexChanged(int idx);
	void on_window_currentIndexChanged(int idx);
};
