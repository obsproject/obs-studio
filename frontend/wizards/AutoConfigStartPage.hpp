#pragma once

#include <wizards/AutoConfig.hpp>

#include <QWizardPage>

class Ui_AutoConfigStartPage;

class AutoConfigStartPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	std::unique_ptr<Ui_AutoConfigStartPage> ui;

protected:
	AutoConfig *autoConfig() const { return qobject_cast<AutoConfig *>(wizard()); }

public:
	AutoConfigStartPage(QWidget *parent = nullptr);
	~AutoConfigStartPage();

	virtual int nextId() const override;

public slots:
	void on_prioritizeStreaming_clicked();
	void on_prioritizeRecording_clicked();
	void PrioritizeVCam();
};
