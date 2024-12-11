#pragma once

#include <obs.hpp>

#include <QDialog>

class OBSAdvAudioCtrl;
class Ui_OBSAdvAudio;

// "Basic advanced audio"?  ...

class OBSBasicAdvAudio : public QDialog {
	Q_OBJECT

private:
	std::vector<OBSSignal> sigs;
	bool showInactive;
	bool showVisible;

	std::vector<OBSAdvAudioCtrl *> controls;

	inline void AddAudioSource(obs_source_t *source);

	static bool EnumSources(void *param, obs_source_t *source);

	static void OBSSourceAdded(void *param, calldata_t *calldata);
	static void OBSSourceRemoved(void *param, calldata_t *calldata);
	static void OBSSourceActivated(void *param, calldata_t *calldata);

	std::unique_ptr<Ui_OBSAdvAudio> ui;

public slots:
	void SourceAdded(OBSSource source);
	void SourceRemoved(OBSSource source);

	void on_usePercent_toggled(bool checked);
	void on_activeOnly_toggled(bool checked);

public:
	OBSBasicAdvAudio(QWidget *parent);
	~OBSBasicAdvAudio();
	void SetShowInactive(bool showInactive);
	void SetIconsVisible(bool visible);
};
