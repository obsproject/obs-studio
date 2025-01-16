#pragma once

#include <QWizardPage>

class Ui_AutoConfigVideoPage;

class AutoConfigVideoPage : public QWizardPage {
	Q_OBJECT

	friend class AutoConfig;

	std::unique_ptr<Ui_AutoConfigVideoPage> ui;

public:
	AutoConfigVideoPage(QWidget *parent = nullptr);
	~AutoConfigVideoPage();

	virtual int nextId() const override;
	virtual bool validatePage() override;
};
