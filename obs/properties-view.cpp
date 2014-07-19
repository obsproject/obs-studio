#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItem>
#include <QFileDialog>
#include <QColorDialog>
#include <QPlainTextEdit>
#include "qt-wrappers.hpp"
#include "properties-view.hpp"
#include "obs-app.hpp"
#include <string>

using namespace std;

static inline QColor color_from_int(long long val)
{
	return QColor( val        & 0xff,
		      (val >>  8) & 0xff,
		      (val >> 16) & 0xff,
		      (val >> 24) & 0xff);
}

static inline long long color_to_int(QColor color)
{
	auto shift = [&](unsigned val, int shift)
	{
		return ((val & 0xff) << shift);
	};

	return  shift(color.red(),    0) |
		shift(color.green(),  8) |
		shift(color.blue(),  16) |
		shift(color.alpha(), 24);
}

void OBSPropertiesView::RefreshProperties()
{
	children.clear();
	if (widget)
		widget->deleteLater();

	widget = new QWidget();

	QFormLayout *layout = new QFormLayout;
	layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	widget->setLayout(layout);

	QSizePolicy mainPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	//widget->setSizePolicy(policy);
	layout->setSizeConstraint(QLayout::SetMaximumSize);
	layout->setLabelAlignment(Qt::AlignRight);

	obs_property_t property = obs_properties_first(properties);

	while (property) {
		AddProperty(property, layout);
		obs_property_next(&property);
	}

	setWidgetResizable(true);
	setWidget(widget);
	setSizePolicy(mainPolicy);

	lastFocused.clear();
	if (lastWidget) {
		lastWidget->setFocus(Qt::OtherFocusReason);
		lastWidget = nullptr;
	}
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_,
		obs_properties_t properties_, void *obj_,
		PropertiesUpdateCallback callback_, int minSize_)
	: QScrollArea (nullptr),
	  widget      (nullptr),
	  properties  (properties_),
	  settings    (settings_),
	  obj         (obj_),
	  callback    (callback_),
	  minSize     (minSize_),
	  lastWidget  (nullptr)
{
	setFrameShape(QFrame::NoFrame);
	RefreshProperties();
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
	const char *name = obs_property_name(prop);
	const char *desc = obs_property_description(prop);
	bool       val   = obs_data_getbool(settings, name);

	QCheckBox *checkbox = new QCheckBox(QT_UTF8(desc));
	checkbox->setCheckState(val ? Qt::Checked : Qt::Unchecked);
	return NewWidget(prop, checkbox, SIGNAL(stateChanged(int)));
}

QWidget *OBSPropertiesView::AddText(obs_property_t prop)
{
	const char    *name = obs_property_name(prop);
	const char    *val  = obs_data_getstring(settings, name);
	obs_text_type type  = obs_proprety_text_type(prop);

	if (type == OBS_TEXT_MULTILINE) {
		QPlainTextEdit *edit = new QPlainTextEdit(QT_UTF8(val));
		return NewWidget(prop, edit, SIGNAL(textChanged()));
	}

	QLineEdit *edit = new QLineEdit();

	if (type == OBS_TEXT_PASSWORD)
		edit->setEchoMode(QLineEdit::Password);

	edit->setText(QT_UTF8(val));

	return NewWidget(prop, edit, SIGNAL(textEdited(const QString &)));
}

void OBSPropertiesView::AddPath(obs_property_t prop, QFormLayout *layout,
		QLabel **label)
{
	const char  *name      = obs_property_name(prop);
	const char  *val       = obs_data_getstring(settings, name);
	QLayout     *subLayout = new QHBoxLayout();
	QLineEdit   *edit      = new QLineEdit();
	QPushButton *button    = new QPushButton(QTStr("Browse"));

	edit->setText(QT_UTF8(val));
	edit->setReadOnly(true);

	subLayout->addWidget(edit);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, edit);
	connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
	children.push_back(std::move(unique_ptr<WidgetInfo>(info)));

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

QWidget *OBSPropertiesView::AddInt(obs_property_t prop)
{
	const char *name = obs_property_name(prop);
	int        val   = (int)obs_data_getint(settings, name);
	QSpinBox   *spin = new QSpinBox();

	spin->setMinimum(obs_property_int_min(prop));
	spin->setMaximum(obs_property_int_max(prop));
	spin->setSingleStep(obs_property_int_step(prop));
	spin->setValue(val);

	return NewWidget(prop, spin, SIGNAL(valueChanged(int)));
}

QWidget *OBSPropertiesView::AddFloat(obs_property_t prop)
{
	const char     *name = obs_property_name(prop);
	double         val   = obs_data_getdouble(settings, name);
	QDoubleSpinBox *spin = new QDoubleSpinBox();

	spin->setMinimum(obs_property_float_min(prop));
	spin->setMaximum(obs_property_float_max(prop));
	spin->setSingleStep(obs_property_float_step(prop));
	spin->setValue(val);

	return NewWidget(prop, spin, SIGNAL(valueChanged(double)));
}

static void AddComboItem(QComboBox *combo, obs_property_t prop,
		obs_combo_format format, size_t idx)
{
	const char *name = obs_property_list_item_name(prop, idx);
	QVariant var;

	if (format == OBS_COMBO_FORMAT_INT) {
		long long val = obs_property_list_item_int(prop, idx);
		var = QVariant::fromValue<long long>(val);

	} else if (format == OBS_COMBO_FORMAT_FLOAT) {
		double val = obs_property_list_item_float(prop, idx);
		var = QVariant::fromValue<double>(val);

	} else if (format == OBS_COMBO_FORMAT_STRING) {
		var = obs_property_list_item_string(prop, idx);
	}

	combo->addItem(QT_UTF8(name), var);

	if (!obs_property_list_item_disabled(prop, idx))
		return;

	int index = combo->findText(QT_UTF8(name));
	if (index < 0)
		return;

	QStandardItemModel *model =
		dynamic_cast<QStandardItemModel*>(combo->model());
	if (!model)
		return;

	QStandardItem *item = model->item(index);
	item->setFlags(Qt::NoItemFlags);
}

template <long long get_int(obs_data_t, const char*),
	 double get_double(obs_data_t, const char*),
	 const char *get_string(obs_data_t, const char*)>
static string from_obs_data(obs_data_t data, const char *name,
		obs_combo_format format)
{
	switch (format) {
	case OBS_COMBO_FORMAT_INT:
		return to_string(get_int(data, name));
	case OBS_COMBO_FORMAT_FLOAT:
		return to_string(get_double(data, name));
	case OBS_COMBO_FORMAT_STRING:
		return get_string(data, name);
	default:
		return "";
	}
}

static string from_obs_data(obs_data_t data, const char *name,
		obs_combo_format format)
{
	return from_obs_data<obs_data_getint, obs_data_getdouble,
	       obs_data_getstring>(data, name, format);
}

static string from_obs_data_autoselect(obs_data_t data, const char *name,
		obs_combo_format format)
{
	return from_obs_data<obs_data_get_autoselect_int,
	       obs_data_get_autoselect_double,
	       obs_data_get_autoselect_string>(data, name, format);
}

QWidget *OBSPropertiesView::AddList(obs_property_t prop, bool &warning)
{
	const char       *name  = obs_property_name(prop);
	QComboBox        *combo = new QComboBox();
	obs_combo_type   type   = obs_property_list_type(prop);
	obs_combo_format format = obs_property_list_format(prop);
	size_t           count  = obs_property_list_item_count(prop);
	int              idx    = -1;

	for (size_t i = 0; i < count; i++)
		AddComboItem(combo, prop, format, i);

	if (type == OBS_COMBO_TYPE_EDITABLE)
		combo->setEditable(true);

	string value = from_obs_data(settings, name, format);

	if (format == OBS_COMBO_FORMAT_STRING &&
			type == OBS_COMBO_TYPE_EDITABLE)
		combo->lineEdit()->setText(QT_UTF8(value.c_str()));
	else
		idx = combo->findData(QT_UTF8(value.c_str()));

	if (type == OBS_COMBO_TYPE_EDITABLE)
		return NewWidget(prop, combo,
				SIGNAL(editTextChanged(const QString &)));

	if (idx != -1)
		combo->setCurrentIndex(idx);
	
	if (obs_data_has_autoselect(settings, name)) {
		string autoselect =
			from_obs_data_autoselect(settings, name, format);
		int id = combo->findData(QT_UTF8(autoselect.c_str()));

		if (id != -1 && id != idx) {
			QString actual   = combo->itemText(id);
			QString selected = combo->itemText(idx);
			QString combined = QTStr(
				"Basic.PropertiesWindow.AutoSelectFormat");
			combo->setItemText(idx,
					combined.arg(selected).arg(actual));
		}
	}


	QAbstractItemModel *model = combo->model();
	warning = idx != -1 &&
		model->flags(model->index(idx, 0)) == Qt::NoItemFlags;

	WidgetInfo *info = new WidgetInfo(this, prop, combo);
	connect(combo, SIGNAL(currentIndexChanged(int)), info,
				SLOT(ControlChanged()));
	children.push_back(std::move(unique_ptr<WidgetInfo>(info)));

	/* trigger a settings update if the index was not found */
	if (idx == -1)
		info->ControlChanged();

	return combo;
}

QWidget *OBSPropertiesView::AddButton(obs_property_t prop)
{
	const char *desc = obs_property_description(prop);

	QPushButton *button = new QPushButton(QT_UTF8(desc));
	button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	return NewWidget(prop, button, SIGNAL(clicked()));
}

void OBSPropertiesView::AddColor(obs_property_t prop, QFormLayout *layout,
		QLabel *&label)
{
	QPushButton *button     = new QPushButton;
	QLabel      *colorLabel = new QLabel;
	const char  *name       = obs_property_name(prop);
	long long   val         = obs_data_getint(settings, name);
	QColor      color       = color_from_int(val);

	button->setText(QTStr("Basic.PropertiesWindow.SelectColor"));

	colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	colorLabel->setText(color.name(QColor::HexArgb));
	colorLabel->setPalette(QPalette(color));
	colorLabel->setAutoFillBackground(true);
	colorLabel->setAlignment(Qt::AlignCenter);

	QHBoxLayout *subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 0, 0, 0);

	subLayout->addWidget(colorLabel);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, colorLabel);
	connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

void OBSPropertiesView::AddProperty(obs_property_t property,
		QFormLayout *layout)
{
	const char        *name = obs_property_name(property);
	obs_property_type type  = obs_property_get_type(property);

	if (!obs_property_visible(property))
		return;

	QLabel  *label  = nullptr;
	QWidget *widget = nullptr;
	bool    warning = false;

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
		AddPath(property, layout, &label);
		break;
	case OBS_PROPERTY_LIST:
		widget = AddList(property, warning);
		break;
	case OBS_PROPERTY_COLOR:
		AddColor(property, layout, label);
		break;
	case OBS_PROPERTY_BUTTON:
		widget = AddButton(property);
		break;
	}

	if (widget && !obs_property_enabled(property))
		widget->setEnabled(false);

	if (!label &&
	    type != OBS_PROPERTY_BOOL &&
	    type != OBS_PROPERTY_BUTTON)
		label = new QLabel(QT_UTF8(obs_property_description(property)));

	if (warning && label) //TODO: select color based on background color
		label->setStyleSheet("QLabel { color: red; }");

	if (label && minSize) {
		label->setMinimumWidth(minSize);
		label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	}

	if (!widget)
		return;

	layout->addRow(label, widget);

	if (!lastFocused.empty())
		if (lastFocused.compare(name) == 0)
			lastWidget = widget;
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
	obs_text_type type  = obs_proprety_text_type(property);

	if (type == OBS_TEXT_MULTILINE) {
		QPlainTextEdit *edit = static_cast<QPlainTextEdit*>(widget);
		obs_data_setstring(view->settings, setting,
				QT_TO_UTF8(edit->toPlainText()));
		return;
	}

	QLineEdit *edit = static_cast<QLineEdit*>(widget);
	obs_data_setstring(view->settings, setting, QT_TO_UTF8(edit->text()));
}

bool WidgetInfo::PathChanged(const char *setting)
{
	const char    *desc         = obs_property_description(property);
	obs_path_type type          = obs_property_path_type(property);
	const char    *filter       = obs_property_path_filter(property);
	const char    *default_path = obs_property_path_default_path(property);
	QString       path;

	if (type == OBS_PATH_DIRECTORY)
		path = QFileDialog::getExistingDirectory(view,
				QT_UTF8(desc), QT_UTF8(default_path),
				QFileDialog::ShowDirsOnly |
				QFileDialog::DontResolveSymlinks);
	else if (type == OBS_PATH_FILE)
		path = QFileDialog::getOpenFileName(view,
				QT_UTF8(desc), QT_UTF8(default_path),
				QT_UTF8(filter));

	if (path.isEmpty())
		return false;

	QLineEdit *edit = static_cast<QLineEdit*>(widget);
	edit->setText(path);
	obs_data_setstring(view->settings, setting, QT_TO_UTF8(path));
	return true;
}

void WidgetInfo::ListChanged(const char *setting)
{
	QComboBox        *combo = static_cast<QComboBox*>(widget);
	obs_combo_format format = obs_property_list_format(property);
	obs_combo_type   type   = obs_property_list_type(property);
	QVariant         data;

	if (type == OBS_COMBO_TYPE_EDITABLE) {
		data = combo->currentText();
	} else {
		int index = combo->currentIndex();
		if (index != -1)
			data = combo->itemData(index);
		else
			return;
	}

	switch (format) {
	case OBS_COMBO_FORMAT_INVALID:
		return;
	case OBS_COMBO_FORMAT_INT:
		obs_data_setint(view->settings, setting,
				data.value<long long>());
		break;
	case OBS_COMBO_FORMAT_FLOAT:
		obs_data_setdouble(view->settings, setting,
				data.value<double>());
		break;
	case OBS_COMBO_FORMAT_STRING:
		obs_data_setstring(view->settings, setting,
				QT_TO_UTF8(data.toString()));
		break;
	}
}

bool WidgetInfo::ColorChanged(const char *setting)
{
	const char *desc = obs_property_description(property);
	long long  val   = obs_data_getint(view->settings, setting);
	QColor     color = color_from_int(val);

	QColorDialog::ColorDialogOptions options =
		QColorDialog::ShowAlphaChannel;

	/* The native dialog on OSX has all kinds of problems, like closing
	 * other open QDialogs on exit, and
	 * https://bugreports.qt-project.org/browse/QTBUG-34532
	 */
#ifdef __APPLE__
	options |= QColorDialog::DontUseNativeDialog;
#endif

	color = QColorDialog::getColor(color, view, QT_UTF8(desc), options);

	if (!color.isValid())
		return false;

	QLabel *label = static_cast<QLabel*>(widget);
	label->setText(color.name(QColor::HexArgb));
	label->setPalette(QPalette(color));

	obs_data_setint(view->settings, setting, color_to_int(color));

	return true;
}

void WidgetInfo::ButtonClicked()
{
	obs_property_button_clicked(property, view->obj);
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
	case OBS_PROPERTY_LIST:    ListChanged(setting); break;
	case OBS_PROPERTY_BUTTON:  ButtonClicked(); return;
	case OBS_PROPERTY_COLOR:
		if (!ColorChanged(setting))
			return;
		break;
	case OBS_PROPERTY_PATH:
		if (!PathChanged(setting))
			return;
	}

	view->callback(view->obj, view->settings);
	if (obs_property_modified(property, view->settings)) {
		view->lastFocused = setting;
		QMetaObject::invokeMethod(view, "RefreshProperties",
				Qt::QueuedConnection);
	}
}
