#pragma once
#if __has_include(<obs-frontend-api.h>)
#include <obs-frontend-api.h>
#else
#include <obs-frontend-api/obs-frontend-api.h>
#endif
#include <util/config-file.h>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>
#include <obs.hpp>
#include <QWidget>
#include <QPointer>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <vector>
using std::vector;
EXPORT typedef struct Mapping
{
	QString input;
	QString inputAction;
	QString output;
	QString outputAction;
}Mapping;
class ControlMapper : public QObject {
	Q_OBJECT

public:

	ControlMapper();
	~ControlMapper();
	Mapping MakeMapping(QString inType, QString inAct, QString outType,
			    QString OutAct);
	bool SaveMapping(Mapping entry);
	vector <Mapping> map;
	bool LoadMapping();
	config_t* GetMappingStore();
	QStringList MappingList;
	QString CurrentInputString;
	QString CurrentOutputString;
	QString PreviousInputString;
	QString PreviousOutputString;
signals:
	QString EventCall(QString input, QString inputAction, QString output,
			  QString outputAction);

public slots:
	void updateInputString(QString inputstring);
	void updateOutputString(QString outputstring);
	QString BroadcastControlEvent(QString input, QString inputAction,
				      QString output, QString outputAction);

private:
	static void OnFrontendEvent(enum obs_frontend_event event, void *param);
};
