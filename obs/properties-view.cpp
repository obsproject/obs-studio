#include <QFormLayout>
#include <QScrollBar>
#include <QLabel>
#include <QCheckBox>
#include <QFont>
#include <QFontDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QStandardItem>
#include <QFileDialog>
#include <QColorDialog>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QMenu>
#include "double-slider.hpp"
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

void OBSPropertiesView::ReloadProperties()
{
	if (obj) {
		properties.reset(reloadCallback(obj));
	} else {
		properties.reset(reloadCallback((void*)type.c_str()));
		obs_properties_apply_settings(properties.get(), settings);
	}

	uint32_t flags = obs_properties_get_flags(properties.get());
	deferUpdate = (flags & OBS_PROPERTIES_DEFER_UPDATE) != 0;

	RefreshProperties();
}

#define NO_PROPERTIES_STRING QTStr("Basic.PropertiesWindow.NoProperties")

void OBSPropertiesView::RefreshProperties()
{
	int h, v;
	GetScrollPos(h, v);

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

	layout->setLabelAlignment(Qt::AlignRight);

	obs_property_t *property = obs_properties_first(properties.get());
	bool hasNoProperties = !property;

	while (property) {
		AddProperty(property, layout);
		obs_property_next(&property);
	}

	setWidgetResizable(true);
	setWidget(widget);
	SetScrollPos(h, v);
	setSizePolicy(mainPolicy);

	lastFocused.clear();
	if (lastWidget) {
		lastWidget->setFocus(Qt::OtherFocusReason);
		lastWidget = nullptr;
	}

	if (hasNoProperties) {
		QLabel *noPropertiesLabel = new QLabel(NO_PROPERTIES_STRING);
		layout->addWidget(noPropertiesLabel);
	}
}

void OBSPropertiesView::SetScrollPos(int h, int v)
{
	QScrollBar *scroll = horizontalScrollBar();
	if (scroll)
		scroll->setValue(h);

	scroll = verticalScrollBar();
	if (scroll)
		scroll->setValue(v);
}

void OBSPropertiesView::GetScrollPos(int &h, int &v)
{
	h = v = 0;

	QScrollBar *scroll = horizontalScrollBar();
	if (scroll)
		h = scroll->value();

	scroll = verticalScrollBar();
	if (scroll)
		v = scroll->value();
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, void *obj_,
		PropertiesReloadCallback reloadCallback,
		PropertiesUpdateCallback callback_, int minSize_)
	: VScrollArea    (nullptr),
	  properties     (nullptr, obs_properties_destroy),
	  settings       (settings_),
	  obj            (obj_),
	  reloadCallback (reloadCallback),
	  callback       (callback_),
	  minSize        (minSize_)
{
	setFrameShape(QFrame::NoFrame);
	ReloadProperties();
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, const char *type_,
		PropertiesReloadCallback reloadCallback_, int minSize_)
	: VScrollArea    (nullptr),
	  properties     (nullptr, obs_properties_destroy),
	  settings       (settings_),
	  type           (type_),
	  reloadCallback (reloadCallback_),
	  minSize        (minSize_)
{
	setFrameShape(QFrame::NoFrame);
	ReloadProperties();
}

void OBSPropertiesView::resizeEvent(QResizeEvent *event)
{
	emit PropertiesResized();
	VScrollArea::resizeEvent(event);
}

QWidget *OBSPropertiesView::NewWidget(obs_property_t *prop, QWidget *widget,
		const char *signal)
{
	WidgetInfo *info = new WidgetInfo(this, prop, widget);
	connect(widget, signal, info, SLOT(ControlChanged()));
	children.emplace_back(info);
	return widget;
}

QWidget *OBSPropertiesView::AddCheckbox(obs_property_t *prop)
{
	const char *name = obs_property_name(prop);
	const char *desc = obs_property_description(prop);
	bool       val   = obs_data_get_bool(settings, name);

	QCheckBox *checkbox = new QCheckBox(QT_UTF8(desc));
	checkbox->setCheckState(val ? Qt::Checked : Qt::Unchecked);
	return NewWidget(prop, checkbox, SIGNAL(stateChanged(int)));
}

QWidget *OBSPropertiesView::AddText(obs_property_t *prop, QFormLayout *layout,
		QLabel *&label)
{
	const char    *name = obs_property_name(prop);
	const char    *val  = obs_data_get_string(settings, name);
	obs_text_type type  = obs_proprety_text_type(prop);

	if (type == OBS_TEXT_MULTILINE) {
		QPlainTextEdit *edit = new QPlainTextEdit(QT_UTF8(val));
		return NewWidget(prop, edit, SIGNAL(textChanged()));

	} else if (type == OBS_TEXT_PASSWORD) {
		QLayout *subLayout = new QHBoxLayout();
		QLineEdit *edit = new QLineEdit();
		QPushButton *show = new QPushButton();

		show->setText(QTStr("Show"));
		show->setCheckable(true);
		edit->setText(QT_UTF8(val));
		edit->setEchoMode(QLineEdit::Password);

		subLayout->addWidget(edit);
		subLayout->addWidget(show);

		WidgetInfo *info = new WidgetInfo(this, prop, edit);
		connect(show, &QAbstractButton::toggled,
				info, &WidgetInfo::TogglePasswordText);
		connect(show, &QAbstractButton::toggled, [=](bool hide)
		{
			show->setText(hide ? QTStr("Hide") : QTStr("Show"));
		});
		children.emplace_back(info);

		label = new QLabel(QT_UTF8(obs_property_description(prop)));
		layout->addRow(label, subLayout);

		connect(edit, SIGNAL(textEdited(const QString &)),
				info, SLOT(ControlChanged()));
		return nullptr;
	}

	QLineEdit *edit = new QLineEdit();

	edit->setText(QT_UTF8(val));

	return NewWidget(prop, edit, SIGNAL(textEdited(const QString &)));
}

void OBSPropertiesView::AddPath(obs_property_t *prop, QFormLayout *layout,
		QLabel **label)
{
	const char  *name      = obs_property_name(prop);
	const char  *val       = obs_data_get_string(settings, name);
	QLayout     *subLayout = new QHBoxLayout();
	QLineEdit   *edit      = new QLineEdit();
	QPushButton *button    = new QPushButton(QTStr("Browse"));

	edit->setText(QT_UTF8(val));
	edit->setReadOnly(true);

	subLayout->addWidget(edit);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, edit);
	connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
	children.emplace_back(info);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

void OBSPropertiesView::AddInt(obs_property_t *prop, QFormLayout *layout,
		QLabel **label)
{
	obs_number_type type = obs_property_int_type(prop);
	QLayout *subLayout = new QHBoxLayout();

	const char *name = obs_property_name(prop);
	int        val   = (int)obs_data_get_int(settings, name);
	QSpinBox   *spin = new QSpinBox();

	int minVal = obs_property_int_min(prop);
	int maxVal = obs_property_int_max(prop);
	int stepVal = obs_property_int_step(prop);

	spin->setMinimum(minVal);
	spin->setMaximum(maxVal);
	spin->setSingleStep(stepVal);
	spin->setValue(val);

	WidgetInfo *info = new WidgetInfo(this, prop, spin);
	children.emplace_back(info);

	if (type == OBS_NUMBER_SLIDER) {
		QSlider *slider = new QSlider();
		slider->setMinimum(minVal);
		slider->setMaximum(maxVal);
		slider->setPageStep(stepVal);
		slider->setValue(val);
		slider->setOrientation(Qt::Horizontal);
		subLayout->addWidget(slider);

		connect(slider, SIGNAL(valueChanged(int)),
				spin, SLOT(setValue(int)));
		connect(spin, SIGNAL(valueChanged(int)),
				slider, SLOT(setValue(int)));
	}

	connect(spin, SIGNAL(valueChanged(int)), info, SLOT(ControlChanged()));

	subLayout->addWidget(spin);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

void OBSPropertiesView::AddFloat(obs_property_t *prop, QFormLayout *layout,
		QLabel **label)
{
	obs_number_type type = obs_property_float_type(prop);
	QLayout *subLayout = new QHBoxLayout();

	const char     *name = obs_property_name(prop);
	double         val   = obs_data_get_double(settings, name);
	QDoubleSpinBox *spin = new QDoubleSpinBox();

	double minVal = obs_property_float_min(prop);
	double maxVal = obs_property_float_max(prop);
	double stepVal = obs_property_float_step(prop);

	spin->setMinimum(minVal);
	spin->setMaximum(maxVal);
	spin->setSingleStep(stepVal);
	spin->setValue(val);

	WidgetInfo *info = new WidgetInfo(this, prop, spin);
	children.emplace_back(info);

	if (type == OBS_NUMBER_SLIDER) {
		DoubleSlider *slider = new DoubleSlider();
		slider->setDoubleConstraints(minVal, maxVal, stepVal, val);
		slider->setOrientation(Qt::Horizontal);
		subLayout->addWidget(slider);

		connect(slider, SIGNAL(doubleValChanged(double)),
				spin, SLOT(setValue(double)));
		connect(spin, SIGNAL(valueChanged(double)),
				slider, SLOT(setDoubleVal(double)));
	}

	connect(spin, SIGNAL(valueChanged(double)), info,
			SLOT(ControlChanged()));

	subLayout->addWidget(spin);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

static void AddComboItem(QComboBox *combo, obs_property_t *prop,
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

template <long long get_int(obs_data_t*, const char*),
	 double get_double(obs_data_t*, const char*),
	 const char *get_string(obs_data_t*, const char*)>
static string from_obs_data(obs_data_t *data, const char *name,
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

static string from_obs_data(obs_data_t *data, const char *name,
		obs_combo_format format)
{
	return from_obs_data<obs_data_get_int, obs_data_get_double,
	       obs_data_get_string>(data, name, format);
}

static string from_obs_data_autoselect(obs_data_t *data, const char *name,
		obs_combo_format format)
{
	return from_obs_data<obs_data_get_autoselect_int,
	       obs_data_get_autoselect_double,
	       obs_data_get_autoselect_string>(data, name, format);
}

QWidget *OBSPropertiesView::AddList(obs_property_t *prop, bool &warning)
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
	
	if (obs_data_has_autoselect_value(settings, name)) {
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
	children.emplace_back(info);

	/* trigger a settings update if the index was not found */
	if (idx == -1)
		info->ControlChanged();

	return combo;
}

static void NewButton(QLayout *layout, WidgetInfo *info,
		const char *themeIcon,
		void (WidgetInfo::*method)())
{
	QPushButton *button = new QPushButton();
	button->setProperty("themeID", themeIcon);
	button->setFlat(true);
	button->setMaximumSize(22, 22);
	button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	QObject::connect(button, &QPushButton::clicked, info, method);

	layout->addWidget(button);
}

void OBSPropertiesView::AddEditableList(obs_property_t *prop,
		QFormLayout *layout, QLabel *&label)
{
	const char       *name  = obs_property_name(prop);
	obs_data_array_t *array = obs_data_get_array(settings, name);
	QListWidget      *list  = new QListWidget();
	size_t           count  = obs_data_array_count(array);

	list->setSortingEnabled(false);
	list->setSelectionMode(QAbstractItemView::ExtendedSelection);

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(array, i);
		list->addItem(QT_UTF8(obs_data_get_string(item, "value")));
		obs_data_release(item);
	}

	WidgetInfo *info = new WidgetInfo(this, prop, list);

	QVBoxLayout *sideLayout = new QVBoxLayout();
	NewButton(sideLayout, info, "addIconSmall",
			&WidgetInfo::EditListAdd);
	NewButton(sideLayout, info, "removeIconSmall",
			&WidgetInfo::EditListRemove);
	NewButton(sideLayout, info, "configIconSmall",
			&WidgetInfo::EditListEdit);
	NewButton(sideLayout, info, "upArrowIconSmall",
			&WidgetInfo::EditListUp);
	NewButton(sideLayout, info, "downArrowIconSmall",
			&WidgetInfo::EditListDown);
	sideLayout->addStretch(0);

	QHBoxLayout *subLayout = new QHBoxLayout();
	subLayout->addWidget(list);
	subLayout->addLayout(sideLayout);

	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);

	obs_data_array_release(array);
}

QWidget *OBSPropertiesView::AddButton(obs_property_t *prop)
{
	const char *desc = obs_property_description(prop);

	QPushButton *button = new QPushButton(QT_UTF8(desc));
	button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	return NewWidget(prop, button, SIGNAL(clicked()));
}

void OBSPropertiesView::AddColor(obs_property_t *prop, QFormLayout *layout,
		QLabel *&label)
{
	QPushButton *button     = new QPushButton;
	QLabel      *colorLabel = new QLabel;
	const char  *name       = obs_property_name(prop);
	long long   val         = obs_data_get_int(settings, name);
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

static void MakeQFont(obs_data_t *font_obj, QFont &font)
{
	const char *face  = obs_data_get_string(font_obj, "face");
	const char *style = obs_data_get_string(font_obj, "style");
	int        size   = (int)obs_data_get_int(font_obj, "size");
	uint32_t   flags  = (uint32_t)obs_data_get_int(font_obj, "flags");

	if (face) {
		font.setFamily(face);
		font.setStyleName(style);
	}

	if (size)
		font.setPointSize(size);

	if (flags & OBS_FONT_BOLD) font.setBold(true);
	if (flags & OBS_FONT_ITALIC) font.setItalic(true);
	if (flags & OBS_FONT_UNDERLINE) font.setUnderline(true);
	if (flags & OBS_FONT_STRIKEOUT) font.setStrikeOut(true);
}

void OBSPropertiesView::AddFont(obs_property_t *prop, QFormLayout *layout,
		QLabel *&label)
{
	const char  *name      = obs_property_name(prop);
	obs_data_t  *font_obj   = obs_data_get_obj(settings, name);
	const char  *face      = obs_data_get_string(font_obj, "face");
	const char  *style     = obs_data_get_string(font_obj, "style");
	QPushButton *button    = new QPushButton;
	QLabel      *fontLabel = new QLabel;
	QFont       font;

	font = fontLabel->font();
	MakeQFont(font_obj, font);

	button->setText(QTStr("Basic.PropertiesWindow.SelectFont"));

	fontLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	fontLabel->setFont(font);
	fontLabel->setText(QString("%1 %2").arg(face, style));
	fontLabel->setAlignment(Qt::AlignCenter);

	QHBoxLayout *subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 0, 0, 0);

	subLayout->addWidget(fontLabel);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, fontLabel);
	connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);

	obs_data_release(font_obj);
}

void OBSPropertiesView::AddProperty(obs_property_t *property,
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
		AddInt(property, layout, &label);
		break;
	case OBS_PROPERTY_FLOAT:
		AddFloat(property, layout, &label);
		break;
	case OBS_PROPERTY_TEXT:
		widget = AddText(property, layout, label);
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
	case OBS_PROPERTY_FONT:
		AddFont(property, layout, label);
		break;
	case OBS_PROPERTY_BUTTON:
		widget = AddButton(property);
		break;
	case OBS_PROPERTY_EDITABLE_LIST:
		AddEditableList(property, layout, label);
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

void OBSPropertiesView::SignalChanged()
{
	emit Changed();
}

void WidgetInfo::BoolChanged(const char *setting)
{
	QCheckBox *checkbox = static_cast<QCheckBox*>(widget);
	obs_data_set_bool(view->settings, setting,
			checkbox->checkState() == Qt::Checked);
}

void WidgetInfo::IntChanged(const char *setting)
{
	QSpinBox *spin = static_cast<QSpinBox*>(widget);
	obs_data_set_int(view->settings, setting, spin->value());
}

void WidgetInfo::FloatChanged(const char *setting)
{
	QDoubleSpinBox *spin = static_cast<QDoubleSpinBox*>(widget);
	obs_data_set_double(view->settings, setting, spin->value());
}

void WidgetInfo::TextChanged(const char *setting)
{
	obs_text_type type  = obs_proprety_text_type(property);

	if (type == OBS_TEXT_MULTILINE) {
		QPlainTextEdit *edit = static_cast<QPlainTextEdit*>(widget);
		obs_data_set_string(view->settings, setting,
				QT_TO_UTF8(edit->toPlainText()));
		return;
	}

	QLineEdit *edit = static_cast<QLineEdit*>(widget);
	obs_data_set_string(view->settings, setting, QT_TO_UTF8(edit->text()));
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
	obs_data_set_string(view->settings, setting, QT_TO_UTF8(path));
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
		obs_data_set_int(view->settings, setting,
				data.value<long long>());
		break;
	case OBS_COMBO_FORMAT_FLOAT:
		obs_data_set_double(view->settings, setting,
				data.value<double>());
		break;
	case OBS_COMBO_FORMAT_STRING:
		obs_data_set_string(view->settings, setting,
				QT_TO_UTF8(data.toString()));
		break;
	}
}

bool WidgetInfo::ColorChanged(const char *setting)
{
	const char *desc = obs_property_description(property);
	long long  val   = obs_data_get_int(view->settings, setting);
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

	obs_data_set_int(view->settings, setting, color_to_int(color));

	return true;
}

bool WidgetInfo::FontChanged(const char *setting)
{
	obs_data_t *font_obj = obs_data_get_obj(view->settings, setting);
	bool       success;
	uint32_t   flags;
	QFont      font;

	if (!font_obj) {
		font = QFontDialog::getFont(&success, view);
	} else {
		MakeQFont(font_obj, font);
		font = QFontDialog::getFont(&success, font, view);
		obs_data_release(font_obj);
	}

	if (!success)
		return false;

	font_obj = obs_data_create();

	obs_data_set_string(font_obj, "face", QT_TO_UTF8(font.family()));
	obs_data_set_string(font_obj, "style", QT_TO_UTF8(font.styleName()));
	obs_data_set_int(font_obj, "size", font.pointSize());
	flags  = font.bold() ? OBS_FONT_BOLD : 0;
	flags |= font.italic() ? OBS_FONT_ITALIC : 0;
	flags |= font.underline() ? OBS_FONT_UNDERLINE : 0;
	flags |= font.strikeOut() ? OBS_FONT_STRIKEOUT : 0;
	obs_data_set_int(font_obj, "flags", flags);

	QLabel *label = static_cast<QLabel*>(widget);
	label->setFont(font);
	label->setText(QString("%1 %2").arg(font.family(), font.styleName()));

	obs_data_set_obj(view->settings, setting, font_obj);
	obs_data_release(font_obj);
	return true;
}

void WidgetInfo::EditableListChanged()
{
	const char *setting = obs_property_name(property);
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	obs_data_array *array = obs_data_array_create();

	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);
		obs_data_t *arrayItem = obs_data_create();
		obs_data_set_string(arrayItem, "value",
				QT_TO_UTF8(item->text()));

		obs_data_array_push_back(array, arrayItem);
		obs_data_release(arrayItem);
	}

	obs_data_set_array(view->settings, setting, array);
	obs_data_array_release(array);
}

void WidgetInfo::ButtonClicked()
{
	if (obs_property_button_clicked(property, view->obj)) {
		QMetaObject::invokeMethod(view, "RefreshProperties",
				Qt::QueuedConnection);
	}
}

void WidgetInfo::TogglePasswordText(bool show)
{
	reinterpret_cast<QLineEdit*>(widget)->setEchoMode(
			show ? QLineEdit::Normal : QLineEdit::Password);
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
	case OBS_PROPERTY_FONT:
		if (!FontChanged(setting))
			return;
		break;
	case OBS_PROPERTY_PATH:
		if (!PathChanged(setting))
			return;
		break;
	case OBS_PROPERTY_EDITABLE_LIST: return;
	}

	if (view->callback && !view->deferUpdate)
		view->callback(view->obj, view->settings);

	view->SignalChanged();

	if (obs_property_modified(property, view->settings)) {
		view->lastFocused = setting;
		QMetaObject::invokeMethod(view, "RefreshProperties",
				Qt::QueuedConnection);
	}
}

class EditableItemDialog : public QDialog {
	QLineEdit *edit;
	QString filter;
	QString default_path;

	void BrowseClicked()
	{
		QString curPath = QFileInfo(edit->text()).absoluteDir().path();

		if (curPath.isEmpty())
			curPath = default_path;

		QString path = QFileDialog::getOpenFileName(
				App()->GetMainWindow(), QTStr("Browse"),
				curPath, filter);
		if (path.isEmpty())
			return;

		edit->setText(path);
	}

public:
	EditableItemDialog(QWidget *parent, const QString &text,
			bool browse, const char *filter_ = nullptr,
			const char *default_path_ = nullptr)
		: QDialog              (parent),
		  filter               (QT_UTF8(filter_)),
		  default_path         (QT_UTF8(default_path_))
	{
		QHBoxLayout *topLayout = new QHBoxLayout();
		QVBoxLayout *mainLayout = new QVBoxLayout();

		edit = new QLineEdit();
		edit->setText(text);
		topLayout->addWidget(edit);
		topLayout->setAlignment(edit, Qt::AlignVCenter);

		if (browse) {
			QPushButton *browseButton =
				new QPushButton(QTStr("Browse"));
			topLayout->addWidget(browseButton);
			topLayout->setAlignment(browseButton, Qt::AlignVCenter);

			connect(browseButton, &QPushButton::clicked, this,
					&EditableItemDialog::BrowseClicked);
		}

		QDialogButtonBox::StandardButtons buttons =
			QDialogButtonBox::Ok |
			QDialogButtonBox::Cancel;

		QDialogButtonBox *buttonBox = new QDialogButtonBox(buttons);
		buttonBox->setCenterButtons(true);

		mainLayout->addLayout(topLayout);
		mainLayout->addWidget(buttonBox);

		setLayout(mainLayout);
		resize(QSize(400, 80));

		connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
		connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	}

	inline QString GetText() const {return edit->text();}
};

void WidgetInfo::EditListAdd()
{
	bool allow_files = obs_property_editable_list_allow_files(property);
	if (!allow_files) {
		EditListAddText();
		return;
	}

	QMenu popup(view->window());

	QAction *action;

	action = new QAction(QTStr("Basic.PropertiesWindow.AddFiles"), this);
	connect(action, &QAction::triggered,
			this, &WidgetInfo::EditListAddFiles);
	popup.addAction(action);

	action = new QAction(QTStr("Basic.PropertiesWindow.AddURL"), this);
	connect(action, &QAction::triggered,
			this, &WidgetInfo::EditListAddText);
	popup.addAction(action);

	popup.exec(QCursor::pos());
}

void WidgetInfo::EditListAddText()
{
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	const char *desc = obs_property_description(property);

	EditableItemDialog dialog(widget->window(), QString(), false);
	auto title = QTStr("Basic.PropertiesWindow.AddEditableListEntry").arg(
			QT_UTF8(desc));
	dialog.setWindowTitle(title);
	if (dialog.exec() == QDialog::Rejected)
		return;

	QString text = dialog.GetText();
	if (text.isEmpty())
		return;

	list->addItem(text);
	EditableListChanged();
}

void WidgetInfo::EditListAddFiles()
{
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	const char *desc = obs_property_description(property);
	const char *filter = obs_property_editable_list_filter(property);
	const char *default_path =
		obs_property_editable_list_default_path(property);

	QString title = QTStr("Basic.PropertiesWindow.AddEditableListFiles")
		.arg(QT_UTF8(desc));

	QStringList files = QFileDialog::getOpenFileNames(
			App()->GetMainWindow(), title, QT_UTF8(default_path),
			QT_UTF8(filter));

	if (files.count() == 0)
		return;

	list->addItems(files);
	EditableListChanged();
}

void WidgetInfo::EditListRemove()
{
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	QList<QListWidgetItem*> items = list->selectedItems();

	for (QListWidgetItem *item : items)
		delete item;
	EditableListChanged();
}

void WidgetInfo::EditListEdit()
{
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	bool allow_files = obs_property_editable_list_allow_files(property);
	const char *desc = obs_property_description(property);
	const char *filter = obs_property_editable_list_filter(property);
	QList<QListWidgetItem*> selectedItems = list->selectedItems();

	if (!selectedItems.count())
		return;

	QListWidgetItem *item = selectedItems[0];
	EditableItemDialog dialog(widget->window(), item->text(), allow_files,
			filter);
	auto title = QTStr("Basic.PropertiesWindow.EditEditableListEntry").arg(
			QT_UTF8(desc));
	dialog.setWindowTitle(title);
	if (dialog.exec() == QDialog::Rejected)
		return;

	QString text = dialog.GetText();
	if (text.isEmpty())
		return;

	item->setText(text);
	EditableListChanged();
}

void WidgetInfo::EditListUp()
{
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	int lastItemRow = -1;

	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);
		if (!item->isSelected())
			continue;

		int row = list->row(item);

		if ((row - 1) != lastItemRow) {
			lastItemRow = row - 1;
			list->takeItem(row);
			list->insertItem(lastItemRow, item);
			item->setSelected(true);
		} else {
			lastItemRow = row;
		}
	}

	EditableListChanged();
}

void WidgetInfo::EditListDown()
{
	QListWidget *list = reinterpret_cast<QListWidget*>(widget);
	int lastItemRow = list->count();

	for (int i = list->count() - 1; i >= 0; i--) {
		QListWidgetItem *item = list->item(i);
		if (!item->isSelected())
			continue;

		int row = list->row(item);

		if ((row + 1) != lastItemRow) {
			lastItemRow = row + 1;
			list->takeItem(row);
			list->insertItem(lastItemRow, item);
			item->setSelected(true);
		} else {
			lastItemRow = row;
		}
	}

	EditableListChanged();
}
