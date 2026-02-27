#pragma once

#include <vertical-scroll-area.hpp>
#include <obs-data.h>
#include <obs.hpp>
#include <qtimer.h>
#include <QPointer>
#include <vector>
#include <memory>

class QFormLayout;
class OBSPropertiesView;
class QLabel;

typedef obs_properties_t *(*PropertiesReloadCallback)(void *obj);
typedef void (*PropertiesUpdateCallback)(void *obj, obs_data_t *old_settings, obs_data_t *new_settings);
typedef void (*PropertiesVisualUpdateCb)(void *obj, obs_data_t *settings);

/* ------------------------------------------------------------------------- */

class WidgetInfo : public QObject {
	Q_OBJECT

	friend class OBSPropertiesView;

private:
	OBSPropertiesView *view;
	obs_property_t *property;
	QWidget *widget;
	QPointer<QTimer> update_timer;
	bool recently_updated = false;
	OBSData old_settings_cache;

	void BoolChanged(const char *setting);
	void IntChanged(const char *setting);
	void FloatChanged(const char *setting);
	void TextChanged(const char *setting);
	bool PathChanged(const char *setting);
	void ListChanged(const char *setting);
	bool ColorChangedInternal(const char *setting, bool supportAlpha);
	bool ColorChanged(const char *setting);
	bool ColorAlphaChanged(const char *setting);
	bool FontChanged(const char *setting);
	void GroupChanged(const char *setting);
	void EditableListChanged();
	void ButtonClicked();

	void TogglePasswordText(bool checked);

public:
	inline WidgetInfo(OBSPropertiesView *view_, obs_property_t *prop, QWidget *widget_)
		: view(view_),
		  property(prop),
		  widget(widget_)
	{
	}

	~WidgetInfo()
	{
		if (update_timer) {
			update_timer->stop();
			QMetaObject::invokeMethod(update_timer, "timeout");
			update_timer->deleteLater();
		}
	}

public slots:

	void ControlChanged();

	/* editable list */
	void EditListAdd();
	void EditListAddText();
	void EditListAddFiles();
	void EditListAddSource();
	void EditListAddDir();
	void EditListRemove();
	void EditListEdit();
	void EditListUp();
	void EditListDown();
};

/* ------------------------------------------------------------------------- */

class OBSPropertiesView : public VScrollArea {
	Q_OBJECT

	friend class WidgetInfo;

	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;

private:
	QWidget *widget = nullptr;
	properties_t properties;
	OBSData settings;
	OBSWeakObjectAutoRelease weakObj;
	void *rawObj = nullptr;
	std::string type;
	PropertiesReloadCallback reloadCallback;
	PropertiesUpdateCallback callback = nullptr;
	PropertiesVisualUpdateCb visUpdateCb = nullptr;
	int minSize;
	std::vector<std::unique_ptr<WidgetInfo>> children;
	std::string lastFocused;
	QWidget *lastWidget = nullptr;
	bool deferUpdate;
	bool enableDefer = true;
	bool disableScrolling = false;

	template<typename Sender, typename SenderParent, typename... Args>
	QWidget *NewWidget(obs_property_t *prop, Sender *widget, void (SenderParent::*signal)(Args...));

	QWidget *AddCheckbox(obs_property_t *prop);
	QWidget *AddText(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	void AddPath(obs_property_t *prop, QFormLayout *layout, QLabel **label);
	void AddInt(obs_property_t *prop, QFormLayout *layout, QLabel **label);
	void AddFloat(obs_property_t *prop, QFormLayout *layout, QLabel **label);
	QWidget *AddList(obs_property_t *prop, bool &warning);
	void AddEditableList(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	QWidget *AddButton(obs_property_t *prop);
	void AddColorInternal(obs_property_t *prop, QFormLayout *layout, QLabel *&label, bool supportAlpha);
	void AddColor(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	void AddColorAlpha(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	void AddFont(obs_property_t *prop, QFormLayout *layout, QLabel *&label);
	void AddFrameRate(obs_property_t *prop, bool &warning, QFormLayout *layout, QLabel *&label);

	void AddGroup(obs_property_t *prop, QFormLayout *layout);

	void AddProperty(obs_property_t *property, QFormLayout *layout);

	void resizeEvent(QResizeEvent *event) override;

	void GetScrollPos(int &h, int &v, int &hend, int &vend);
	void SetScrollPos(int h, int v, int old_hend, int old_vend);

private slots:
	void RefreshProperties();

public slots:
	void ReloadProperties();
	void SignalChanged();

signals:
	void PropertiesResized();
	void Changed();
	void PropertiesRefreshed();

public:
	OBSPropertiesView(OBSData settings, obs_object_t *obj, PropertiesReloadCallback reloadCallback,
			  PropertiesUpdateCallback callback, PropertiesVisualUpdateCb cb = nullptr, int minSize = 0);
	OBSPropertiesView(OBSData settings, void *obj, PropertiesReloadCallback reloadCallback,
			  PropertiesUpdateCallback callback, PropertiesVisualUpdateCb cb = nullptr, int minSize = 0);
	OBSPropertiesView(OBSData settings, const char *type, PropertiesReloadCallback reloadCallback, int minSize = 0);

#define obj_constructor(type)                                                                                     \
	inline OBSPropertiesView(OBSData settings, obs_##type##_t *type, PropertiesReloadCallback reloadCallback, \
				 PropertiesUpdateCallback callback, PropertiesVisualUpdateCb cb = nullptr,        \
				 int minSize = 0)                                                                 \
		: OBSPropertiesView(settings, (obs_object_t *)type, reloadCallback, callback, cb, minSize)        \
	{                                                                                                         \
	}

	obj_constructor(source);
	obj_constructor(output);
	obj_constructor(encoder);
	obj_constructor(service);
#undef obj_constructor

	inline obs_data_t *GetSettings() const { return settings; }

	inline void UpdateSettings()
	{
		if (callback)
			callback(OBSGetStrongRef(weakObj), nullptr, settings);
		else if (visUpdateCb)
			visUpdateCb(OBSGetStrongRef(weakObj), settings);
	}
	inline bool DeferUpdate() const { return deferUpdate; }
	inline void SetDeferrable(bool deferrable) { enableDefer = deferrable; }

	inline OBSObject GetObject() const { return OBSGetStrongRef(weakObj); }

	void setScrolling(bool enabled)
	{
		disableScrolling = !enabled;
		RefreshProperties();
	}

	void SetDisabled(bool disabled);

#define Def_IsObject(type)                                \
	inline bool IsObject(obs_##type##_t *type) const  \
	{                                                 \
		OBSObject obj = OBSGetStrongRef(weakObj); \
		return obj.Get() == (obs_object_t *)type; \
	}

	/* clang-format off */
	Def_IsObject(source)
	Def_IsObject(output)
	Def_IsObject(encoder)
	Def_IsObject(service)
	/* clang-format on */

#undef Def_IsObject
};
