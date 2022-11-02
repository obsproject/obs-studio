#pragma once

#include <QVBoxLayout>
#include <QMap>
#include <QLabel>

class OBSWarningBox : public QVBoxLayout {
	Q_OBJECT

	QMap<QString, QString> errors;
	QMap<QString, QString> warnings;

	QStringList restartIds;

	QList<QSharedPointer<QLabel>> labels;

	void AddLabel(bool error, const QString &msg);
	void UpdateLayout();

public:
	inline OBSWarningBox() : QVBoxLayout(){};
	inline explicit OBSWarningBox(QWidget *parent) : QVBoxLayout(parent){};
	inline ~OBSWarningBox(){};

	void AddWarning(const QString &id, const QString &message);
	void AddError(const QString &id, const QString &message);

	void RemoveWarning(const QString &id);
	void RemoveError(const QString &id);

	void AddRestartWarning(const QString &id);
	void RemoveRestartWarning(const QString &id);

	inline bool HasErrors() { return errors.size(); };
	QString GetHTMLErrorsList();
};
