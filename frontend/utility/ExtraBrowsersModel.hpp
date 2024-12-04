#pragma once

#include <QAbstractTableModel>
#include <QUuid>

enum class Column : int {
	Title,
	Url,
	Delete,

	Count,
};

#define OBJ_NAME_SUFFIX "_extraBrowser"

class ExtraBrowsersModel : public QAbstractTableModel {
	Q_OBJECT

public:
	inline ExtraBrowsersModel(QObject *parent = nullptr) : QAbstractTableModel(parent)
	{
		Reset();
		QMetaObject::invokeMethod(this, "Init", Qt::QueuedConnection);
	}

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	struct Item {
		int prevIdx;
		QString title;
		QString url;
	};

	void TabSelection(bool forward);

	void AddDeleteButton(int idx);
	void Reset();
	void CheckToAdd();
	void UpdateItem(Item &item);
	void DeleteItem();
	void Apply();

	QVector<Item> items;
	QVector<int> deleted;

	QString newTitle;
	QString newURL;

public slots:
	void Init();
};
