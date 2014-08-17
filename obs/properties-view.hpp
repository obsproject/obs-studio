#pragma once

#include <QScrollArea>
#include <obs.hpp>
#include <vector>
#include <memory>

class QFormLayout;
class OBSPropertiesView;
class QLabel;

typedef void (*PropertiesUpdateCallback)(void *obj, obs_data_t settings);

/* ------------------------------------------------------------------------- */

class WidgetInfo : public QObject {
	Q_OBJECT

private:
	OBSPropertiesView *view;
	obs_property_t    property;
	QWidget           *widget;

	void BoolChanged(const char *setting);
	void IntChanged(const char *setting);
	void FloatChanged(const char *setting);
	void TextChanged(const char *setting);
	bool PathChanged(const char *setting);
	void ListChanged(const char *setting);
	bool ColorChanged(const char *setting);
	bool FontChanged(const char *setting);
	void ButtonClicked();

public:
	inline WidgetInfo(OBSPropertiesView *view_, obs_property_t prop,
			QWidget *widget_)
		: view(view_), property(prop), widget(widget_)
	{}

public slots:
	void ControlChanged();
};

/* ------------------------------------------------------------------------- */

class OBSPropertiesView : public QScrollArea {
	Q_OBJECT

	friend class WidgetInfo;

private:
	QWidget                                  *widget;
	obs_properties_t                         properties;
	OBSData                                  settings;
	void                                     *obj;
	PropertiesUpdateCallback                 callback;
	int                                      minSize;
	std::vector<std::unique_ptr<WidgetInfo>> children;
	std::string                              lastFocused;
	QWidget                                  *lastWidget;

	QWidget *NewWidget(obs_property_t prop, QWidget *widget,
			const char *signal);

	QWidget *AddCheckbox(obs_property_t prop);
	QWidget *AddText(obs_property_t prop);
	void AddPath(obs_property_t prop, QFormLayout *layout, QLabel **label);
	QWidget *AddInt(obs_property_t prop);
	QWidget *AddFloat(obs_property_t prop);
	QWidget *AddList(obs_property_t prop, bool &warning);
	QWidget *AddButton(obs_property_t prop);
	void AddColor(obs_property_t prop, QFormLayout *layout, QLabel *&label);
	void AddFont(obs_property_t prop, QFormLayout *layout, QLabel *&label);

	void AddProperty(obs_property_t property, QFormLayout *layout);

public slots:
	void RefreshProperties();

public:
	OBSPropertiesView(OBSData settings,
			obs_properties_t properties,
			void *obj, PropertiesUpdateCallback callback,
			int minSize = 0);

	inline ~OBSPropertiesView()
	{
		obs_properties_destroy(properties);
	}
};
