#pragma once
#if __has_include(<obs-frontend-api.h>)
#include <obs-frontend-api.h>
#else
#include <obs-frontend-api/obs-frontend-api.h>
#endif
#include <QWidget>
#include <QIcon>
#include <QString>
#include <QDialog>
class OBSControlWidget : public QDialog
{

public:
	inline OBSControlWidget(QIcon *Icon = nullptr,
			 QString *Name = nullptr, QWidget *Page = nullptr);

};
