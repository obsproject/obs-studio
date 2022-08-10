#pragma once

#include <QPushButton>
#include <QBoxLayout>
#include <QScopedPointer>

class RecordButton : public QPushButton {
	Q_OBJECT

public:
	inline RecordButton(QWidget *parent = nullptr) : QPushButton(parent) {}

	virtual void resizeEvent(QResizeEvent *event) override;
};

class OBSBasic;

class ControlsSplitButton : public QHBoxLayout {
	Q_OBJECT

public:
	ControlsSplitButton(const QString &text, const QVariant &themeID,
			    void (OBSBasic::*clicked)());

	void addIcon(const QString &name, const QVariant &themeID,
		     void (OBSBasic::*clicked)());
	void removeIcon();
	void insert(int index);

	inline QPushButton *first() { return button.data(); }
	inline QPushButton *second() { return icon.data(); }

protected:
	virtual bool eventFilter(QObject *obj, QEvent *event) override;

private:
	QScopedPointer<QPushButton> button;
	QScopedPointer<QPushButton> icon;
};
