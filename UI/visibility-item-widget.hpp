#pragma once

#include <QWidget>
#include <QStyledItemDelegate>
#include <obs.hpp>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QCheckBox;
class OBSSourceLabel;

class VisibilityItemWidget : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	OBSSourceLabel *label = nullptr;
	QCheckBox *vis = nullptr;

	OBSSignal enabledSignal;

	bool active = false;
	bool selected = false;

	static void OBSSourceEnabled(void *param, calldata_t *data);

private slots:
	void SourceEnabled(bool enabled);

public:
	VisibilityItemWidget(obs_source_t *source);

	void SetColor(const QColor &color, bool active, bool selected);
};

class VisibilityItemDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	VisibilityItemDelegate(QObject *parent = nullptr);

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		   const QModelIndex &index) const override;

protected:
	bool eventFilter(QObject *object, QEvent *event) override;
};

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item,
			 obs_source_t *source);
