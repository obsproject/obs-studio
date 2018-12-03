#pragma once

#include "vertical-scroll-area.hpp"
#include <obs.hpp>
#include <vector>
#include <memory>
#include <unordered_set>

class QFormLayout;
class OBSPropertiesView;
class QLabel;

typedef obs_properties_t *(*PropertiesReloadCallback)(void *obj);
typedef void              (*PropertiesUpdateCallback)(void *obj,
							obs_data_t *settings);

/* ------------------------------------------------------------------------- */

class WidgetInfo : public QObject {
	Q_OBJECT

	friend class OBSPropertiesView;

private:
	OBSPropertiesView *view;
	obs_property_t    *property;
	QWidget           *widget;

	void BoolChanged(const char *setting);
	void IntChanged(const char *setting);
	void FloatChanged(const char *setting);
	void TextChanged(const char *setting);
	bool PathChanged(const char *setting);
	void ListChanged(const char *setting);
	bool ColorChanged(const char *setting);
	bool FontChanged(const char *setting);
	void EditableListChanged();
	void ButtonClicked();

	void TogglePasswordText(bool checked);

public:
	inline WidgetInfo(OBSPropertiesView *view_, obs_property_t *prop,
			QWidget *widget_)
		: view(view_), property(prop), widget(widget_)
	{}

public slots:

	void ControlChanged();

	/* editable list */
	void EditListAdd();
	void EditListAddText();
	void EditListAddFiles();
	void EditListAddDir();
	void EditListRemove();
	void EditListEdit();
	void EditListUp();
	void EditListDown();
};

class GroupInfo : public QObject {
	Q_OBJECT

	friend class OBSPropertiesView;

private:
	OBSPropertiesView    *view;
	obs_property_group_t *group;
	QWidget              *widget;

public:
	inline GroupInfo(OBSPropertiesView *view_,
			obs_property_group_t *group_, QWidget *widget_)
		: view(view_), group(group_), widget(widget_)
	{}

public slots:

	void Toggled(int state);
};

/* ------------------------------------------------------------------------- */

class OBSPropertiesView : public VScrollArea {
	Q_OBJECT

	friend class WidgetInfo;
	friend class GroupInfo;

	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t =
		std::unique_ptr<obs_properties_t, properties_delete_t>;

private:
	QWidget                                  *widget = nullptr;
	properties_t                             properties;
	OBSData                                  settings;
	void                                     *obj = nullptr;
	std::string                              type;
	PropertiesReloadCallback                 reloadCallback;
	PropertiesUpdateCallback                 callback = nullptr;
	int                                      minSize;
	std::vector<std::unique_ptr<WidgetInfo>> children;
	std::vector<std::unique_ptr<GroupInfo>>  groups;
	std::string                              lastFocused;
	QWidget                                  *lastWidget = nullptr;
	bool                                     deferUpdate;
	std::unordered_set<obs_property_group_t *> collapsedGroups;

	QWidget *NewWidget(obs_property_t *prop, QWidget *widget,
			const char *signal);

	QWidget *AddCheckbox(obs_property_t *prop);
	QWidget *AddText(obs_property_t *prop, QFormLayout *layout,
			QLabel *&label);
	void AddPath(obs_property_t *prop, QFormLayout *layout, QLabel **label);
	void AddInt(obs_property_t *prop, QFormLayout *layout, QLabel **label);
	void AddFloat(obs_property_t *prop, QFormLayout *layout,
			QLabel**label);
	QWidget *AddList(obs_property_t *prop, bool &warning);
	void AddEditableList(obs_property_t *prop, QFormLayout *layout,
			QLabel *&label);
	QWidget *AddButton(obs_property_t *prop);
	void AddColor(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	void AddFont(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	void AddFrameRate(obs_property_t *prop, bool &warning,
			QFormLayout *layout, QLabel *&label);

	void AddProperty(obs_property_t *property, QFormLayout *layout);

	QFormLayout *AddGroup(obs_property_group_t *group, QFormLayout *layout,
			bool collapsed);

	void resizeEvent(QResizeEvent *event) override;

	void GetScrollPos(int &h, int &v);
	void SetScrollPos(int h, int v);

public slots:
	void ReloadProperties();
	void RefreshProperties();
	void SignalChanged();

signals:
	void PropertiesResized();
	void Changed();

public:
	OBSPropertiesView(OBSData settings, void *obj,
			PropertiesReloadCallback reloadCallback,
			PropertiesUpdateCallback callback,
			int minSize = 0);
	OBSPropertiesView(OBSData settings, const char *type,
			PropertiesReloadCallback reloadCallback,
			int minSize = 0);

	inline obs_data_t *GetSettings() const {return settings;}

	inline void UpdateSettings() {callback(obj, settings);}
	inline bool DeferUpdate() const {return deferUpdate;}
};
