#pragma once

#include <QStyledItemDelegate>

class ExtraBrowsersModel;

class ExtraBrowsersDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	inline ExtraBrowsersDelegate(ExtraBrowsersModel *model_) : QStyledItemDelegate(nullptr), model(model_) {}

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
			      const QModelIndex &index) const override;

	void setEditorData(QWidget *editor, const QModelIndex &index) const override;

	bool eventFilter(QObject *object, QEvent *event) override;
	void RevertText(QLineEdit *edit);
	bool UpdateText(QLineEdit *edit);
	bool ValidName(const QString &text) const;

	ExtraBrowsersModel *model;
};
