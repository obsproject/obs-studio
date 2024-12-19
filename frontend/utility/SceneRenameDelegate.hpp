#pragma once

#include <QStyledItemDelegate>

class SceneRenameDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	SceneRenameDelegate(QObject *parent);
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;

protected:
	virtual bool eventFilter(QObject *editor, QEvent *event) override;
};
