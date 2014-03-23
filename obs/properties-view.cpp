#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include "qt-wrappers.hpp"
#include "properties-view.hpp"
#include <string>

using namespace std;

OBSPropertiesView::OBSPropertiesView(OBSData settings_,
		obs_properties_t properties_, void *obj_,
		PropertiesUpdateCallback callback_)
	: QScrollArea (nullptr),
	  properties  (properties_),
	  settings    (settings_),
	  obj         (obj_),
	  callback    (callback_)
{
	widget = new QWidget();

	QFormLayout *layout = new QFormLayout;
	widget->setLayout(layout);

	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	widget->setSizePolicy(policy);
	layout->setSizeConstraint(QLayout::SetMaximumSize);
	layout->setLabelAlignment(Qt::AlignRight);

	obs_property_t property = obs_properties_first(properties);

	while (property) {
		AddProperty(property, layout);
		obs_property_next(&property);
	}

	setWidget(widget);
	setSizePolicy(policy);
}

QWidget *OBSPropertiesView::NewWidget(obs_property_t prop, QWidget *widget,
		const char *signal)
{
	WidgetInfo *info = new WidgetInfo(this, prop, widget);
	connect(widget, signal, info, SLOT(ControlChanged()));
	children.push_back(std::move(unique_ptr<WidgetInfo>(info)));
	return widget;
}

QWidget *OBSPropertiesView::AddCheckbox(obs_property_t prop)
{
	const char *desc = obs_property_description(prop);

	return NewWidget(prop, new QCheckBox(QT_UTF8(desc)),
			SIGNAL(stateChanged(int)));
}

QWidget *OBSPropertiesView::AddText(obs_property_t prop)
{
	return NewWidget(prop, new QLineEdit(),
			SIGNAL(textEdited(const QString &)));
}

void OBSPropertiesView::AddPath(obs_property_t prop, QFormLayout *layout)
{
	/* TODO */
	UNUSED_PARAMETER(prop);
	UNUSED_PARAMETER(layout);
}

QWidget *OBSPropertiesView::AddInt(obs_property_t prop)
{
	QSpinBox *spin = new QSpinBox();
	spin->setMinimum(obs_property_int_min(prop));
	spin->setMaximum(obs_property_int_max(prop));
	spin->setSingleStep(obs_property_int_step(prop));

	return NewWidget(prop, spin, SIGNAL(valueChanged(int)));
}

QWidget *OBSPropertiesView::AddFloat(obs_property_t prop)
{
	QDoubleSpinBox *spin = new QDoubleSpinBox();
	spin->setMinimum(obs_property_float_min(prop));
	spin->setMaximum(obs_property_float_max(prop));
	spin->setSingleStep(obs_property_float_step(prop));

	return NewWidget(prop, spin, SIGNAL(valueChanged(double)));
}

QWidget *OBSPropertiesView::AddList(obs_property_t prop)
{
	QComboBox *combo        = new QComboBox();
	obs_combo_type   type   = obs_property_list_type(prop);
	size_t           count  = obs_property_list_item_count(prop);

	for (size_t i = 0; i < count; i++) {
		const char *name  = obs_property_list_item_name(prop, i);
		const char *val   = obs_property_list_item_value(prop, i);
		combo->addItem(QT_UTF8(name), QT_UTF8(val));
	}

	if (type == OBS_COMBO_TYPE_EDITABLE) {
		combo->setEditable(true);
		return NewWidget(prop, combo,
				SLOT(editTextChanged(const QString &)));
	}

	return NewWidget(prop, combo, SIGNAL(currentIndexChanged(int)));
}

void OBSPropertiesView::AddProperty(obs_property_t property,
		QFormLayout *layout)
{
	obs_property_type type = obs_property_get_type(property);

	QWidget *widget = nullptr;

	switch (type) {
	case OBS_PROPERTY_INVALID:
		return;
	case OBS_PROPERTY_BOOL:
		widget = AddCheckbox(property);
		break;
	case OBS_PROPERTY_INT:
		widget = AddInt(property);
		break;
	case OBS_PROPERTY_FLOAT:
		widget = AddFloat(property);
		break;
	case OBS_PROPERTY_TEXT:
		widget = AddText(property);
		break;
	case OBS_PROPERTY_PATH:
		AddPath(property, layout);
		break;
	case OBS_PROPERTY_LIST:
		widget = AddList(property);
		break;
	case OBS_PROPERTY_COLOR:
		/* TODO */
		break;
	}

	if (!widget)
		return;

	QLabel *label = nullptr;
	if (type != OBS_PROPERTY_BOOL)
		label = new QLabel(QT_UTF8(obs_property_description(property)));

	layout->addRow(label, widget);
}

void WidgetInfo::BoolChanged(const char *setting)
{
	QCheckBox *checkbox = static_cast<QCheckBox*>(widget);
	obs_data_setbool(view->settings, setting,
			checkbox->checkState() == Qt::Checked);
}

void WidgetInfo::IntChanged(const char *setting)
{
	QSpinBox *spin = static_cast<QSpinBox*>(widget);
	obs_data_setint(view->settings, setting, spin->value());
}

void WidgetInfo::FloatChanged(const char *setting)
{
	QDoubleSpinBox *spin = static_cast<QDoubleSpinBox*>(widget);
	obs_data_setdouble(view->settings, setting, spin->value());
}

void WidgetInfo::TextChanged(const char *setting)
{
	QLineEdit *edit = static_cast<QLineEdit*>(widget);
	obs_data_setstring(view->settings, setting, QT_TO_UTF8(edit->text()));
}

void WidgetInfo::PathChanged(const char *setting)
{
	/* TODO */
	UNUSED_PARAMETER(setting);
}

void WidgetInfo::ListChanged(const char *setting)
{
	QComboBox        *combo = static_cast<QComboBox*>(widget);
	obs_combo_format format = obs_property_list_format(property);
	obs_combo_type   type   = obs_property_list_type(property);

	string val;
	if (type == OBS_COMBO_TYPE_EDITABLE) {
		val = QT_TO_UTF8(combo->currentText());
	} else {
		int index = combo->currentIndex();
		if (index != -1) {
			QVariant variant = combo->itemData(index);
			val = QT_TO_UTF8(variant.toString());
		}
	}

	switch (format) {
	case OBS_COMBO_FORMAT_INVALID:
		return;
	case OBS_COMBO_FORMAT_INT:
		obs_data_setint(view->settings, setting, stol(val));
		break;
	case OBS_COMBO_FORMAT_FLOAT:
		obs_data_setdouble(view->settings, setting, stod(val));
		break;
	case OBS_COMBO_FORMAT_STRING:
		obs_data_setstring(view->settings, setting, val.c_str());
		break;
	}
}

void WidgetInfo::ColorChanged(const char *setting)
{
	/* TODO */
	UNUSED_PARAMETER(setting);
}

void WidgetInfo::ControlChanged()
{
	const char        *setting = obs_property_name(property);
	obs_property_type type     = obs_property_get_type(property);

	switch (type) {
	case OBS_PROPERTY_INVALID: return;
	case OBS_PROPERTY_BOOL:    BoolChanged(setting); break;
	case OBS_PROPERTY_INT:     IntChanged(setting); break;
	case OBS_PROPERTY_FLOAT:   FloatChanged(setting); break;
	case OBS_PROPERTY_TEXT:    TextChanged(setting); break;
	case OBS_PROPERTY_PATH:    PathChanged(setting); break;
	case OBS_PROPERTY_LIST:    ListChanged(setting); break;
	case OBS_PROPERTY_COLOR:   ColorChanged(setting); break;
	}

	view->callback(view->obj, view->settings);
}
