#pragma once

#include <obs.hpp>
#include <QDialog>
#include <vector>

class OBSAdvAudioCtrl;
class QGridLayout;

// "Basic advanced audio"?  ...

class OBSBasicAdvAudio : public QDialog {
	Q_OBJECT

private:
	QWidget *controlArea;
	QGridLayout *mainLayout;
	OBSSignal sourceAddedSignal;
	OBSSignal sourceRemovedSignal;

	std::vector<OBSAdvAudioCtrl*> controls;

	inline void AddAudioSource(obs_source_t *source);

	static bool EnumSources(void *param, obs_source_t *source);

	static void OBSSourceAdded(void *param, calldata_t *calldata);
	static void OBSSourceRemoved(void *param, calldata_t *calldata);

public slots:
	void SourceAdded(OBSSource source);
	void SourceRemoved(OBSSource source);

public:
	OBSBasicAdvAudio(QWidget *parent);
	~OBSBasicAdvAudio();
};
