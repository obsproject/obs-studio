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
#include <QRadioButton>
#include <QButtonGroup>
#include <QStandardItem>
#include <QFileDialog>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMessageBox>
#include <QStackedWidget>
#include <QDir>
#include <QGroupBox>
#include <QObject>
#include <QDesktopServices>
#include <QUuid>
#include "double-slider.hpp"
#include "spinbox-ignorewheel.hpp"
#include "moc_properties-view.cpp"
#include "properties-view.moc.hpp"

#include <qt-wrappers.hpp>
#include <plain-text-edit.hpp>
#include <slider-ignorewheel.hpp>
#include <icon-label.hpp>
#include <cstdlib>
#include <initializer_list>
#include <obs-data.h>
#include <obs.h>
#include <qtimer.h>
#include <string>
#include <obs-frontend-api.h>

using namespace std;

static inline QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff);
}

static inline long long color_to_int(QColor color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
	};

	return shift(color.red(), 0) | shift(color.green(), 8) | shift(color.blue(), 16) | shift(color.alpha(), 24);
}

namespace {

struct frame_rate_tag {
	enum tag_type {
		SIMPLE,
		RATIONAL,
		USER,
	} type = SIMPLE;
	const char *val = nullptr;

	frame_rate_tag() = default;

	explicit frame_rate_tag(tag_type type) : type(type) {}

	explicit frame_rate_tag(const char *val) : type(USER), val(val) {}

	static frame_rate_tag simple() { return frame_rate_tag{SIMPLE}; }
	static frame_rate_tag rational() { return frame_rate_tag{RATIONAL}; }
};

struct common_frame_rate {
	const char *fps_name;
	media_frames_per_second fps;
};

} // namespace

Q_DECLARE_METATYPE(frame_rate_tag);
Q_DECLARE_METATYPE(media_frames_per_second);

void OBSPropertiesView::ReloadProperties()
{
	if (weakObj || rawObj) {
		OBSObject strongObj = GetObject();
		void *obj = strongObj ? strongObj.Get() : rawObj;
		if (obj)
			properties.reset(reloadCallback(obj));
	} else {
		properties.reset(reloadCallback((void *)type.c_str()));
		obs_properties_apply_settings(properties.get(), settings);
	}

	uint32_t flags = obs_properties_get_flags(properties.get());
	deferUpdate = enableDefer && (flags & OBS_PROPERTIES_DEFER_UPDATE) != 0;

	RefreshProperties();
}

#define NO_PROPERTIES_STRING QObject::tr("Basic.PropertiesWindow.NoProperties")

void OBSPropertiesView::RefreshProperties()
{
	int h, v, hend, vend;
	GetScrollPos(h, v, hend, vend);

	children.clear();
	if (widget)
		widget->deleteLater();

	widget = new QWidget();
	widget->setObjectName("PropertiesContainer");

	QFormLayout *layout = new QFormLayout;
	layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	widget->setLayout(layout);

	QSizePolicy mainPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	layout->setLabelAlignment(Qt::AlignRight);

	obs_property_t *property = obs_properties_first(properties.get());
	bool hasNoProperties = !property;

	while (property) {
		AddProperty(property, layout);
		obs_property_next(&property);
	}

	setWidgetResizable(true);
	setWidget(widget);
	setSizePolicy(mainPolicy);
	adjustSize();
	SetScrollPos(h, v, hend, vend);

	if (disableScrolling)
		setMinimumHeight(widget->minimumSizeHint().height());

	lastFocused.clear();
	if (lastWidget) {
		lastWidget->setFocus(Qt::OtherFocusReason);
		lastWidget = nullptr;
	}

	if (hasNoProperties) {
		QLabel *noPropertiesLabel = new QLabel(NO_PROPERTIES_STRING);
		layout->addWidget(noPropertiesLabel);
	}

	emit PropertiesRefreshed();
}

void OBSPropertiesView::SetScrollPos(int h, int v, int old_hend, int old_vend)
{
	QScrollBar *scroll = horizontalScrollBar();
	if (scroll) {
		int hend = scroll->maximum() + scroll->pageStep();
		scroll->setValue(h * hend / old_hend);
	}

	scroll = verticalScrollBar();
	if (scroll) {
		int vend = scroll->maximum() + scroll->pageStep();
		scroll->setValue(v * vend / old_vend);
	}
}

void OBSPropertiesView::GetScrollPos(int &h, int &v, int &hend, int &vend)
{
	h = v = 0;

	QScrollBar *scroll = horizontalScrollBar();
	if (scroll) {
		h = scroll->value();
		hend = scroll->maximum() + scroll->pageStep();
	}

	scroll = verticalScrollBar();
	if (scroll) {
		v = scroll->value();
		vend = scroll->maximum() + scroll->pageStep();
	}
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, obs_object_t *obj, PropertiesReloadCallback reloadCallback,
				     PropertiesUpdateCallback callback_, PropertiesVisualUpdateCb visUpdateCb_,
				     int minSize_)
	: VScrollArea(nullptr),
	  properties(nullptr, obs_properties_destroy),
	  settings(settings_),
	  weakObj(obs_object_get_weak_object(obj)),
	  reloadCallback(reloadCallback),
	  callback(callback_),
	  visUpdateCb(visUpdateCb_),
	  minSize(minSize_)
{
	setFrameShape(QFrame::NoFrame);
	QMetaObject::invokeMethod(this, "ReloadProperties", Qt::QueuedConnection);
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, void *obj, PropertiesReloadCallback reloadCallback,
				     PropertiesUpdateCallback callback_, PropertiesVisualUpdateCb visUpdateCb_,
				     int minSize_)
	: VScrollArea(nullptr),
	  properties(nullptr, obs_properties_destroy),
	  settings(settings_),
	  rawObj(obj),
	  reloadCallback(reloadCallback),
	  callback(callback_),
	  visUpdateCb(visUpdateCb_),
	  minSize(minSize_)
{
	setFrameShape(QFrame::NoFrame);
	QMetaObject::invokeMethod(this, "ReloadProperties", Qt::QueuedConnection);
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, const char *type_, PropertiesReloadCallback reloadCallback_,
				     int minSize_)
	: VScrollArea(nullptr),
	  properties(nullptr, obs_properties_destroy),
	  settings(settings_),
	  type(type_),
	  reloadCallback(reloadCallback_),
	  minSize(minSize_)
{
	setFrameShape(QFrame::NoFrame);
	QMetaObject::invokeMethod(this, "ReloadProperties", Qt::QueuedConnection);
}

void OBSPropertiesView::SetDisabled(bool disabled)
{
	for (auto child : findChildren<QWidget *>()) {
		child->setDisabled(disabled);
	}
}

void OBSPropertiesView::resizeEvent(QResizeEvent *event)
{
	emit PropertiesResized();
	VScrollArea::resizeEvent(event);
}

template<typename Sender, typename SenderParent, typename... Args>
QWidget *OBSPropertiesView::NewWidget(obs_property_t *prop, Sender *widget, void (SenderParent::*signal)(Args...))
{
	const char *long_desc = obs_property_long_description(prop);

	WidgetInfo *info = new WidgetInfo(this, prop, widget);
	QObject::connect(widget, signal, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	widget->setToolTip(QT_UTF8(long_desc));
	return widget;
}

QWidget *OBSPropertiesView::AddCheckbox(obs_property_t *prop)
{
	const char *name = obs_property_name(prop);
	const char *desc = obs_property_description(prop);
	const char *long_desc = obs_property_long_description(prop);
	bool val = obs_data_get_bool(settings, name);

	QCheckBox *checkbox = new QCheckBox(QT_UTF8(desc));
	checkbox->setCheckState(val ? Qt::Checked : Qt::Unchecked);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	QWidget *widget = NewWidget(prop, checkbox, &QCheckBox::checkStateChanged);
#else
	QWidget *widget = NewWidget(prop, checkbox, &QCheckBox::stateChanged);
#endif

	if (!long_desc) {
		return widget;
	}

	QString file = !obs_frontend_is_theme_dark() ? ":/res/images/help.svg" : ":/res/images/help_light.svg";

	IconLabel *help = new IconLabel(checkbox);
	help->setIcon(QIcon(file));
	help->setToolTip(long_desc);

#ifdef __APPLE__
	checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

	widget = new QWidget();
	QHBoxLayout *layout = new QHBoxLayout(widget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setAlignment(Qt::AlignLeft);
	layout->setSpacing(0);

	layout->addWidget(checkbox);
	layout->addWidget(help);
	widget->setLayout(layout);

	return widget;
}

QWidget *OBSPropertiesView::AddText(obs_property_t *prop, QFormLayout *layout, QLabel *&label)
{
	const char *name = obs_property_name(prop);
	const char *val = obs_data_get_string(settings, name);
	bool monospace = obs_property_text_monospace(prop);
	obs_text_type type = obs_property_text_type(prop);

	if (type == OBS_TEXT_MULTILINE) {
		OBSPlainTextEdit *edit = new OBSPlainTextEdit(this, monospace);
		edit->setPlainText(QT_UTF8(val));
		edit->setTabStopDistance(40);
		return NewWidget(prop, edit, &OBSPlainTextEdit::textChanged);

	} else if (type == OBS_TEXT_PASSWORD) {
		QLayout *subLayout = new QHBoxLayout();
		QLineEdit *edit = new QLineEdit();
		QPushButton *show = new QPushButton();

		show->setText(tr("Show"));
		show->setCheckable(true);
		edit->setText(QT_UTF8(val));
		edit->setEchoMode(QLineEdit::Password);

		subLayout->addWidget(edit);
		subLayout->addWidget(show);

		WidgetInfo *info = new WidgetInfo(this, prop, edit);
		connect(show, &QAbstractButton::toggled, info, &WidgetInfo::TogglePasswordText);
		connect(show, &QAbstractButton::toggled,
			[=](bool hide) { show->setText(hide ? tr("Hide") : tr("Show")); });
		children.emplace_back(info);

		label = new QLabel(QT_UTF8(obs_property_description(prop)));
		layout->addRow(label, subLayout);

		edit->setToolTip(QT_UTF8(obs_property_long_description(prop)));

		connect(edit, &QLineEdit::textEdited, info, &WidgetInfo::ControlChanged);
		return nullptr;
	} else if (type == OBS_TEXT_INFO) {
		QString desc = QT_UTF8(obs_property_description(prop));
		const char *long_desc = obs_property_long_description(prop);
		obs_text_info_type info_type = obs_property_text_info_type(prop);

		QLabel *info_label = new QLabel(QT_UTF8(val));

		if (info_label->text().isEmpty() && long_desc == NULL) {
			label = nullptr;
			info_label->setText(desc);
		} else
			label = new QLabel(desc);

		if (long_desc != NULL && !info_label->text().isEmpty()) {
			QString file = !obs_frontend_is_theme_dark() ? ":/res/images/help.svg"
								     : ":/res/images/help_light.svg";
			QString lStr = "<html>%1 <img src='%2' style=' \
				vertical-align: bottom; ' /></html>";

			info_label->setText(lStr.arg(info_label->text(), file));
			info_label->setToolTip(QT_UTF8(long_desc));
		} else if (long_desc != NULL) {
			info_label->setText(QT_UTF8(long_desc));
		}

		info_label->setOpenExternalLinks(true);
		info_label->setWordWrap(obs_property_text_info_word_wrap(prop));

		if (info_type == OBS_TEXT_INFO_WARNING)
			info_label->setProperty("class", "text-warning");
		else if (info_type == OBS_TEXT_INFO_ERROR)
			info_label->setProperty("class", "text-danger");

		if (label)
			label->setObjectName(info_label->objectName());

		WidgetInfo *info = new WidgetInfo(this, prop, info_label);
		children.emplace_back(info);

		layout->addRow(label, info_label);

		return nullptr;
	}

	QLineEdit *edit = new QLineEdit();

	edit->setText(QT_UTF8(val));
	edit->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	return NewWidget(prop, edit, &QLineEdit::textEdited);
}

void OBSPropertiesView::AddPath(obs_property_t *prop, QFormLayout *layout, QLabel **label)
{
	const char *name = obs_property_name(prop);
	const char *val = obs_data_get_string(settings, name);
	QLayout *subLayout = new QHBoxLayout();
	QLineEdit *edit = new QLineEdit();
	QPushButton *button = new QPushButton(tr("Browse"));

	if (!obs_property_enabled(prop)) {
		edit->setEnabled(false);
		button->setEnabled(false);
	}

	edit->setText(QT_UTF8(val));
	edit->setReadOnly(true);
	edit->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	subLayout->addWidget(edit);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, edit);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

void OBSPropertiesView::AddInt(obs_property_t *prop, QFormLayout *layout, QLabel **label)
{
	obs_number_type type = obs_property_int_type(prop);
	QLayout *subLayout = new QHBoxLayout();

	const char *name = obs_property_name(prop);
	int val = (int)obs_data_get_int(settings, name);
	QSpinBox *spin = new SpinBoxIgnoreScroll();

	spin->setEnabled(obs_property_enabled(prop));

	int minVal = obs_property_int_min(prop);
	int maxVal = obs_property_int_max(prop);
	int stepVal = obs_property_int_step(prop);
	const char *suffix = obs_property_int_suffix(prop);

	spin->setMinimum(minVal);
	spin->setMaximum(maxVal);
	spin->setSingleStep(stepVal);
	spin->setValue(val);
	spin->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	spin->setSuffix(QT_UTF8(suffix));

	WidgetInfo *info = new WidgetInfo(this, prop, spin);
	children.emplace_back(info);

	if (type == OBS_NUMBER_SLIDER) {
		QSlider *slider = new SliderIgnoreScroll();
		slider->setMinimum(minVal);
		slider->setMaximum(maxVal);
		slider->setPageStep(stepVal);
		slider->setValue(val);
		slider->setOrientation(Qt::Horizontal);
		slider->setEnabled(obs_property_enabled(prop));
		subLayout->addWidget(slider);

		connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
		connect(spin, &QSpinBox::valueChanged, slider, &QSlider::setValue);
	}

	connect(spin, &QSpinBox::valueChanged, info, &WidgetInfo::ControlChanged);

	subLayout->addWidget(spin);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

void OBSPropertiesView::AddFloat(obs_property_t *prop, QFormLayout *layout, QLabel **label)
{
	obs_number_type type = obs_property_float_type(prop);
	QLayout *subLayout = new QHBoxLayout();

	const char *name = obs_property_name(prop);
	double val = obs_data_get_double(settings, name);
	QDoubleSpinBox *spin = new QDoubleSpinBox();

	if (!obs_property_enabled(prop))
		spin->setEnabled(false);

	double minVal = obs_property_float_min(prop);
	double maxVal = obs_property_float_max(prop);
	double stepVal = obs_property_float_step(prop);
	const char *suffix = obs_property_float_suffix(prop);

	if (stepVal < 1.0) {
		constexpr int sane_limit = 8;
		const int decimals = std::min<int>(log10(1.0 / stepVal) + 0.99, sane_limit);
		if (decimals > spin->decimals())
			spin->setDecimals(decimals);
	}

	spin->setMinimum(minVal);
	spin->setMaximum(maxVal);
	spin->setSingleStep(stepVal);
	spin->setValue(val);
	spin->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	spin->setSuffix(QT_UTF8(suffix));

	WidgetInfo *info = new WidgetInfo(this, prop, spin);
	children.emplace_back(info);

	if (type == OBS_NUMBER_SLIDER) {
		DoubleSlider *slider = new DoubleSlider();
		slider->setDoubleConstraints(minVal, maxVal, stepVal, val);
		slider->setOrientation(Qt::Horizontal);
		subLayout->addWidget(slider);

		connect(slider, &DoubleSlider::doubleValChanged, spin, &QDoubleSpinBox::setValue);
		connect(spin, &QDoubleSpinBox::valueChanged, slider, &DoubleSlider::setDoubleVal);
	}

	connect(spin, &QDoubleSpinBox::valueChanged, info, &WidgetInfo::ControlChanged);

	subLayout->addWidget(spin);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

static QVariant propertyListToQVariant(obs_property_t *prop, size_t idx)
{
	obs_combo_format format = obs_property_list_format(prop);

	QVariant var;
	if (format == OBS_COMBO_FORMAT_INT) {
		long long val = obs_property_list_item_int(prop, idx);
		var = QVariant::fromValue<long long>(val);
	} else if (format == OBS_COMBO_FORMAT_FLOAT) {
		double val = obs_property_list_item_float(prop, idx);
		var = QVariant::fromValue<double>(val);
	} else if (format == OBS_COMBO_FORMAT_STRING) {
		var = QByteArray(obs_property_list_item_string(prop, idx));
	} else if (format == OBS_COMBO_FORMAT_BOOL) {
		bool val = obs_property_list_item_bool(prop, idx);
		var = QVariant::fromValue<bool>(val);
	}
	return var;
}

static void AddComboItem(QComboBox *combo, obs_property_t *prop, size_t idx)
{
	const char *name = obs_property_list_item_name(prop, idx);
	QVariant var = propertyListToQVariant(prop, idx);

	combo->addItem(QT_UTF8(name), var);

	if (!obs_property_list_item_disabled(prop, idx))
		return;

	int index = combo->findText(QT_UTF8(name));
	if (index < 0)
		return;

	QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(combo->model());
	if (!model)
		return;

	QStandardItem *item = model->item(index);
	item->setFlags(Qt::NoItemFlags);
}

static void AddRadioItem(QButtonGroup *buttonGroup, QFormLayout *layout, obs_property_t *prop, QVariant value,
			 size_t idx)
{
	const char *name = obs_property_list_item_name(prop, idx);

	QVariant var = propertyListToQVariant(prop, idx);
	QRadioButton *button = new QRadioButton(name);
	button->setChecked(value == var);
	button->setProperty("value", var);
	buttonGroup->addButton(button);
	layout->addRow(button);
}

template<long long get_int(obs_data_t *, const char *), double get_double(obs_data_t *, const char *),
	 const char *get_string(obs_data_t *, const char *), bool get_bool(obs_data_t *, const char *)>
static QVariant from_obs_data(obs_data_t *data, const char *name, obs_combo_format format)
{
	switch (format) {
	case OBS_COMBO_FORMAT_INT:
		return QVariant::fromValue(get_int(data, name));
	case OBS_COMBO_FORMAT_FLOAT:
		return QVariant::fromValue(get_double(data, name));
	case OBS_COMBO_FORMAT_STRING:
		return QByteArray(get_string(data, name));
	case OBS_COMBO_FORMAT_BOOL:
		return QVariant::fromValue(get_bool(data, name));
	default:
		return QVariant();
	}
}

static QVariant from_obs_data(obs_data_t *data, const char *name, obs_combo_format format)
{
	return from_obs_data<obs_data_get_int, obs_data_get_double, obs_data_get_string, obs_data_get_bool>(data, name,
													    format);
}

static QVariant from_obs_data_autoselect(obs_data_t *data, const char *name, obs_combo_format format)
{
	return from_obs_data<obs_data_get_autoselect_int, obs_data_get_autoselect_double,
			     obs_data_get_autoselect_string, obs_data_get_autoselect_bool>(data, name, format);
}

QWidget *OBSPropertiesView::AddList(obs_property_t *prop, bool &warning)
{
	const char *name = obs_property_name(prop);
	obs_combo_type type = obs_property_list_type(prop);
	obs_combo_format format = obs_property_list_format(prop);
	size_t count = obs_property_list_item_count(prop);

	QVariant value = from_obs_data(settings, name, format);

	if (type == OBS_COMBO_TYPE_RADIO) {
		QButtonGroup *buttonGroup = new QButtonGroup();
		QFormLayout *subLayout = new QFormLayout();
		subLayout->setContentsMargins(0, 0, 0, 0);

		for (size_t idx = 0; idx < count; idx++)
			AddRadioItem(buttonGroup, subLayout, prop, value, idx);

		if (count > 0) {
			buttonGroup->setExclusive(true);
			WidgetInfo *info = new WidgetInfo(this, prop, buttonGroup->buttons()[0]);
			children.emplace_back(info);
			connect(buttonGroup, &QButtonGroup::buttonClicked, info, &WidgetInfo::ControlChanged);
		}

		QWidget *widget = new QWidget();
		widget->setLayout(subLayout);
		return widget;
	}

	int idx = -1;

	QComboBox *combo = new QComboBox();
	for (size_t i = 0; i < count; i++)
		AddComboItem(combo, prop, i);

	if (type == OBS_COMBO_TYPE_EDITABLE)
		combo->setEditable(true);

	combo->setMaxVisibleItems(40);
	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	if (format == OBS_COMBO_FORMAT_STRING && type == OBS_COMBO_TYPE_EDITABLE) {
		combo->lineEdit()->setText(value.toString());
	} else {
		idx = combo->findData(value);
	}

	if (type == OBS_COMBO_TYPE_EDITABLE)
		return NewWidget(prop, combo, &QComboBox::editTextChanged);

	if (idx != -1)
		combo->setCurrentIndex(idx);

	if (obs_data_has_autoselect_value(settings, name)) {
		QVariant autoselect = from_obs_data_autoselect(settings, name, format);
		int id = combo->findData(autoselect);

		if (id != -1 && id != idx) {
			QString actual = combo->itemText(id);
			QString selected = combo->itemText(idx);
			QString combined = tr("Basic.PropertiesWindow.AutoSelectFormat");
			combo->setItemText(idx, combined.arg(selected).arg(actual));
		}
	}

	QAbstractItemModel *model = combo->model();
	warning = idx != -1 && model->flags(model->index(idx, 0)) == Qt::NoItemFlags;

	WidgetInfo *info = new WidgetInfo(this, prop, combo);
	connect(combo, &QComboBox::currentIndexChanged, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	/* trigger a settings update if the index was not found */
	if (count && idx == -1)
		info->ControlChanged();

	return combo;
}

static void NewButton(QLayout *layout, WidgetInfo *info, const char *themeIcon, void (WidgetInfo::*method)())
{
	QPushButton *button = new QPushButton();
	button->setProperty("class", "btn-tool " + QString(themeIcon));
	button->setFlat(true);

	QObject::connect(button, &QPushButton::clicked, info, method);

	layout->addWidget(button);
}

void OBSPropertiesView::AddEditableList(obs_property_t *prop, QFormLayout *layout, QLabel *&label)
{
	const char *name = obs_property_name(prop);
	OBSDataArrayAutoRelease array = obs_data_get_array(settings, name);
	QListWidget *list = new QListWidget();
	size_t count = obs_data_array_count(array);

	if (!obs_property_enabled(prop))
		list->setEnabled(false);

	list->setSortingEnabled(false);
	list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	list->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	list->setSpacing(1);

	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease item = obs_data_array_item(array, i);
		list->addItem(QT_UTF8(obs_data_get_string(item, "value")));
		QListWidgetItem *const list_item = list->item((int)i);
		list_item->setSelected(obs_data_get_bool(item, "selected"));
		list_item->setHidden(obs_data_get_bool(item, "hidden"));
		QString uuid = QT_UTF8(obs_data_get_string(item, "uuid"));
		/* for backwards compatibility */
		if (uuid.isEmpty()) {
			uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
			obs_data_set_string(item, "uuid", uuid.toUtf8());
		}
		list_item->setData(Qt::UserRole, uuid);
	}

	WidgetInfo *info = new WidgetInfo(this, prop, list);

	list->setDragDropMode(QAbstractItemView::InternalMove);
	connect(list->model(), &QAbstractItemModel::rowsMoved, [info]() { info->EditableListChanged(); });

	QVBoxLayout *sideLayout = new QVBoxLayout();
	NewButton(sideLayout, info, "icon-plus", &WidgetInfo::EditListAdd);
	NewButton(sideLayout, info, "icon-trash", &WidgetInfo::EditListRemove);
	NewButton(sideLayout, info, "icon-gear", &WidgetInfo::EditListEdit);
	NewButton(sideLayout, info, "icon-up", &WidgetInfo::EditListUp);
	NewButton(sideLayout, info, "icon-down", &WidgetInfo::EditListDown);
	sideLayout->addStretch(0);

	QHBoxLayout *subLayout = new QHBoxLayout();
	subLayout->addWidget(list);
	subLayout->addLayout(sideLayout);

	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

QWidget *OBSPropertiesView::AddButton(obs_property_t *prop)
{
	const char *desc = obs_property_description(prop);

	QPushButton *button = new QPushButton(QT_UTF8(desc));
	button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	return NewWidget(prop, button, &QPushButton::clicked);
}

void OBSPropertiesView::AddColorInternal(obs_property_t *prop, QFormLayout *layout, QLabel *&label, bool supportAlpha)
{
	QPushButton *button = new QPushButton;
	QLabel *colorLabel = new QLabel;
	const char *name = obs_property_name(prop);
	long long val = obs_data_get_int(settings, name);
	QColor color = color_from_int(val);
	QColor::NameFormat format;

	if (!obs_property_enabled(prop)) {
		button->setEnabled(false);
		colorLabel->setEnabled(false);
	}

	button->setText(tr("Basic.PropertiesWindow.SelectColor"));
	button->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	if (supportAlpha) {
		format = QColor::HexArgb;
	} else {
		format = QColor::HexRgb;
		color.setAlpha(255);
	}

	QPalette palette = QPalette(color);
	colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	colorLabel->setText(color.name(format));
	colorLabel->setPalette(palette);
	colorLabel->setStyleSheet(QString("background-color :%1; color: %2;")
					  .arg(palette.color(QPalette::Window).name(format))
					  .arg(palette.color(QPalette::WindowText).name(format)));
	colorLabel->setAutoFillBackground(true);
	colorLabel->setAlignment(Qt::AlignCenter);
	colorLabel->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	QHBoxLayout *subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 0, 0, 0);

	subLayout->addWidget(colorLabel);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, colorLabel);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

void OBSPropertiesView::AddColor(obs_property_t *prop, QFormLayout *layout, QLabel *&label)
{
	AddColorInternal(prop, layout, label, false);
}

void OBSPropertiesView::AddColorAlpha(obs_property_t *prop, QFormLayout *layout, QLabel *&label)
{
	AddColorInternal(prop, layout, label, true);
}

void MakeQFont(obs_data_t *font_obj, QFont &font, bool limit = false)
{
	const char *face = obs_data_get_string(font_obj, "face");
	const char *style = obs_data_get_string(font_obj, "style");
	int size = (int)obs_data_get_int(font_obj, "size");
	uint32_t flags = (uint32_t)obs_data_get_int(font_obj, "flags");

	if (face) {
		font.setFamily(face);
		font.setStyleName(style);
	}

	if (size) {
		if (limit) {
			int max_size = font.pointSize();
			if (max_size < 28)
				max_size = 28;
			if (size > max_size)
				size = max_size;
		}
		font.setPointSize(size);
	}

	if (flags & OBS_FONT_BOLD)
		font.setBold(true);
	if (flags & OBS_FONT_ITALIC)
		font.setItalic(true);
	if (flags & OBS_FONT_UNDERLINE)
		font.setUnderline(true);
	if (flags & OBS_FONT_STRIKEOUT)
		font.setStrikeOut(true);
}

void OBSPropertiesView::AddFont(obs_property_t *prop, QFormLayout *layout, QLabel *&label)
{
	const char *name = obs_property_name(prop);
	OBSDataAutoRelease font_obj = obs_data_get_obj(settings, name);
	const char *face = obs_data_get_string(font_obj, "face");
	const char *style = obs_data_get_string(font_obj, "style");
	QPushButton *button = new QPushButton;
	QLabel *fontLabel = new QLabel;
	QFont font;

	if (!obs_property_enabled(prop)) {
		button->setEnabled(false);
		fontLabel->setEnabled(false);
	}

	font = fontLabel->font();
	MakeQFont(font_obj, font, true);

	button->setText(tr("Basic.PropertiesWindow.SelectFont"));
	button->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	fontLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	fontLabel->setFont(font);
	fontLabel->setText(QString("%1 %2").arg(face, style));
	fontLabel->setAlignment(Qt::AlignCenter);
	fontLabel->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	QHBoxLayout *subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 0, 0, 0);

	subLayout->addWidget(fontLabel);
	subLayout->addWidget(button);

	WidgetInfo *info = new WidgetInfo(this, prop, fontLabel);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

namespace std {

template<> struct default_delete<obs_data_t> {
	void operator()(obs_data_t *data) { obs_data_release(data); }
};

template<> struct default_delete<obs_data_item_t> {
	void operator()(obs_data_item_t *item) { obs_data_item_release(&item); }
};

} // namespace std

template<typename T> static double make_epsilon(T val)
{
	return val * 0.00001;
}

static bool matches_range(media_frames_per_second &match, media_frames_per_second fps, const frame_rate_range_t &pair)
{
	auto val = media_frames_per_second_to_frame_interval(fps);
	auto max_ = media_frames_per_second_to_frame_interval(pair.first);
	auto min_ = media_frames_per_second_to_frame_interval(pair.second);

	if (min_ <= val && val <= max_) {
		match = fps;
		return true;
	}

	return false;
}

static bool matches_ranges(media_frames_per_second &best_match, media_frames_per_second fps,
			   const frame_rate_ranges_t &fps_ranges, bool exact = false)
{
	auto convert_fn = media_frames_per_second_to_frame_interval;
	auto val = convert_fn(fps);
	auto epsilon = make_epsilon(val);

	bool match = false;
	auto best_dist = numeric_limits<double>::max();
	for (auto &pair : fps_ranges) {
		auto max_ = convert_fn(pair.first);
		auto min_ = convert_fn(pair.second);
		/*blog(LOG_INFO, "%lg <= %lg <= %lg? %s %s %s",
				min_, val, max_,
				fabsl(min_ - val) < epsilon ? "true" : "false",
				min_ <= val && val <= max_  ? "true" : "false",
				fabsl(min_ - val) < epsilon ? "true" :
				"false");*/

		if (matches_range(best_match, fps, pair))
			return true;

		if (exact)
			continue;

		auto min_dist = fabsl(min_ - val);
		auto max_dist = fabsl(max_ - val);
		if (min_dist < epsilon && min_dist < best_dist) {
			best_match = pair.first;
			match = true;
			continue;
		}

		if (max_dist < epsilon && max_dist < best_dist) {
			best_match = pair.second;
			match = true;
			continue;
		}
	}

	return match;
}

static media_frames_per_second make_fps(uint32_t num, uint32_t den)
{
	media_frames_per_second fps{};
	fps.numerator = num;
	fps.denominator = den;
	return fps;
}

static const common_frame_rate common_fps[] = {
	{"240", {240, 1}},         {"144", {144, 1}},        {"120", {120, 1}}, {"119.88", {120000, 1001}},
	{"60", {60, 1}},           {"59.94", {60000, 1001}}, {"50", {50, 1}},   {"48", {48, 1}},
	{"30", {30, 1}},           {"29.97", {30000, 1001}}, {"25", {25, 1}},   {"24", {24, 1}},
	{"23.976", {24000, 1001}},
};

static void UpdateSimpleFPSSelection(OBSFrameRatePropertyWidget *fpsProps, const media_frames_per_second *current_fps)
{
	if (!current_fps || !media_frames_per_second_is_valid(*current_fps)) {
		fpsProps->simpleFPS->setCurrentIndex(0);
		return;
	}

	auto combo = fpsProps->simpleFPS;
	auto num = combo->count();
	for (int i = 0; i < num; i++) {
		auto variant = combo->itemData(i);
		if (!variant.canConvert<media_frames_per_second>())
			continue;

		auto fps = variant.value<media_frames_per_second>();
		if (fps != *current_fps)
			continue;

		combo->setCurrentIndex(i);
		return;
	}

	combo->setCurrentIndex(0);
}

static void AddFPSRanges(vector<common_frame_rate> &items, const frame_rate_ranges_t &ranges)
{
	auto InsertFPS = [&](media_frames_per_second fps) {
		auto fps_val = media_frames_per_second_to_fps(fps);

		auto end_ = end(items);
		auto i = begin(items);
		for (; i != end_; i++) {
			auto i_fps_val = media_frames_per_second_to_fps(i->fps);
			if (fabsl(i_fps_val - fps_val) < 0.01)
				return;

			if (i_fps_val > fps_val)
				continue;

			break;
		}

		items.insert(i, {nullptr, fps});
	};

	for (auto &range : ranges) {
		InsertFPS(range.first);
		InsertFPS(range.second);
	}
}

static QWidget *CreateSimpleFPSValues(OBSFrameRatePropertyWidget *fpsProps, bool &selected,
				      const media_frames_per_second *current_fps)
{
	auto widget = new QWidget{};
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto layout = new QVBoxLayout{};
	layout->setContentsMargins(0, 0, 0, 0);

	auto items = vector<common_frame_rate>{};
	items.reserve(sizeof(common_fps) / sizeof(common_frame_rate));

	auto combo = fpsProps->simpleFPS = new QComboBox();

	combo->addItem("", QVariant::fromValue(make_fps(0, 0)));
	for (const auto &fps : common_fps) {
		media_frames_per_second best_match{};
		if (!matches_ranges(best_match, fps.fps, fpsProps->fps_ranges))
			continue;

		items.push_back({fps.fps_name, best_match});
	}

	AddFPSRanges(items, fpsProps->fps_ranges);

	for (const auto &item : items) {
		auto var = QVariant::fromValue(item.fps);
		auto name = item.fps_name ? QString(item.fps_name)
					  : QString("%1").arg(media_frames_per_second_to_fps(item.fps));
		combo->addItem(name, var);

		bool select = current_fps && *current_fps == item.fps;
		if (select) {
			combo->setCurrentIndex(combo->count() - 1);
			selected = true;
		}
	}

	layout->addWidget(combo, 0, Qt::AlignTop);
	widget->setLayout(layout);

	return widget;
}

static void UpdateRationalFPSWidgets(OBSFrameRatePropertyWidget *fpsProps, const media_frames_per_second *current_fps)
{
	if (!current_fps || !media_frames_per_second_is_valid(*current_fps)) {
		fpsProps->numEdit->setValue(0);
		fpsProps->denEdit->setValue(0);
		return;
	}

	auto combo = fpsProps->fpsRange;
	auto num = combo->count();
	for (int i = 0; i < num; i++) {
		auto variant = combo->itemData(i);
		if (!variant.canConvert<size_t>())
			continue;

		auto idx = variant.value<size_t>();
		if (fpsProps->fps_ranges.size() < idx)
			continue;

		media_frames_per_second match{};
		if (!matches_range(match, *current_fps, fpsProps->fps_ranges[idx]))
			continue;

		combo->setCurrentIndex(i);
		break;
	}

	fpsProps->numEdit->setValue(current_fps->numerator);
	fpsProps->denEdit->setValue(current_fps->denominator);
}

static QWidget *CreateRationalFPS(OBSFrameRatePropertyWidget *fpsProps, bool &selected,
				  const media_frames_per_second *current_fps)
{
	auto widget = new QWidget{};
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto layout = new QFormLayout{};
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	auto str = QObject::tr("Basic.PropertiesView.FPS.ValidFPSRanges");
	auto rlabel = new QLabel{str};

	auto combo = fpsProps->fpsRange = new QComboBox();
	auto convert_fps = media_frames_per_second_to_fps;
	//auto convert_fi  = media_frames_per_second_to_frame_interval;

	for (size_t i = 0; i < fpsProps->fps_ranges.size(); i++) {
		auto &pair = fpsProps->fps_ranges[i];
		combo->addItem(QString{"%1 - %2"}.arg(convert_fps(pair.first)).arg(convert_fps(pair.second)),
			       QVariant::fromValue(i));

		media_frames_per_second match;
		if (!current_fps || !matches_range(match, *current_fps, pair))
			continue;

		combo->setCurrentIndex(combo->count() - 1);
		selected = true;
	}

	layout->addRow(rlabel, combo);

	auto num_edit = fpsProps->numEdit = new SpinBoxIgnoreScroll{};
	auto den_edit = fpsProps->denEdit = new SpinBoxIgnoreScroll{};

	num_edit->setRange(0, INT_MAX);
	den_edit->setRange(0, INT_MAX);

	if (current_fps) {
		num_edit->setValue(current_fps->numerator);
		den_edit->setValue(current_fps->denominator);
	}

	layout->addRow(QObject::tr("Basic.Settings.Video.Numerator"), num_edit);
	layout->addRow(QObject::tr("Basic.Settings.Video.Denominator"), den_edit);

	widget->setLayout(layout);

	return widget;
}

static OBSFrameRatePropertyWidget *CreateFrameRateWidget(obs_property_t *prop, bool &warning, const char *option,
							 media_frames_per_second *current_fps,
							 frame_rate_ranges_t &fps_ranges)
{
	auto widget = new OBSFrameRatePropertyWidget{};
	auto hlayout = new QHBoxLayout{};
	hlayout->setContentsMargins(0, 0, 0, 0);

	swap(widget->fps_ranges, fps_ranges);

	auto combo = widget->modeSelect = new QComboBox();
	combo->addItem(QObject::tr("Basic.PropertiesView.FPS.Simple"), QVariant::fromValue(frame_rate_tag::simple()));
	combo->addItem(QObject::tr("Basic.PropertiesView.FPS.Rational"),
		       QVariant::fromValue(frame_rate_tag::rational()));

	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	auto num = obs_property_frame_rate_options_count(prop);
	if (num)
		combo->insertSeparator(combo->count());

	bool option_found = false;
	for (size_t i = 0; i < num; i++) {
		auto name = obs_property_frame_rate_option_name(prop, i);
		auto desc = obs_property_frame_rate_option_description(prop, i);
		combo->addItem(desc, QVariant::fromValue(frame_rate_tag{name}));

		if (!name || !option || string(name) != option)
			continue;

		option_found = true;
		combo->setCurrentIndex(combo->count() - 1);
	}

	hlayout->addWidget(combo, 0, Qt::AlignTop);

	auto stack = widget->modeDisplay = new QStackedWidget{};

	bool match_found = option_found;
	auto AddWidget = [&](decltype(CreateRationalFPS) func) {
		bool selected = false;
		stack->addWidget(func(widget, selected, current_fps));

		if (match_found || !selected)
			return;

		match_found = true;

		stack->setCurrentIndex(stack->count() - 1);
		combo->setCurrentIndex(stack->count() - 1);
	};

	AddWidget(CreateSimpleFPSValues);
	AddWidget(CreateRationalFPS);
	stack->addWidget(new QWidget{});

	if (option_found)
		stack->setCurrentIndex(stack->count() - 1);
	else if (!match_found) {
		int idx = current_fps ? 1 : 0; // Rational for "unsupported"
					       // Simple as default
		stack->setCurrentIndex(idx);
		combo->setCurrentIndex(idx);
		warning = true;
	}

	hlayout->addWidget(stack, 0, Qt::AlignTop);

	auto label_area = widget->labels = new QWidget{};
	label_area->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto vlayout = new QVBoxLayout{};
	vlayout->setContentsMargins(0, 0, 0, 0);

	auto fps_label = widget->currentFPS = new QLabel{"FPS: 22"};
	auto time_label = widget->timePerFrame = new QLabel{"Frame Interval: 0.123 ms"};
	auto min_label = widget->minLabel = new QLabel{"Min FPS: 1/1"};
	auto max_label = widget->maxLabel = new QLabel{"Max FPS: 2/1"};

	min_label->setHidden(true);
	max_label->setHidden(true);

	auto flags = Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
	min_label->setTextInteractionFlags(flags);
	max_label->setTextInteractionFlags(flags);

	vlayout->addWidget(fps_label);
	vlayout->addWidget(time_label);
	vlayout->addWidget(min_label);
	vlayout->addWidget(max_label);
	label_area->setLayout(vlayout);

	hlayout->addWidget(label_area, 0, Qt::AlignTop);

	widget->setLayout(hlayout);

	return widget;
}

static void UpdateMinMaxLabels(OBSFrameRatePropertyWidget *w)
{
	auto Hide = [&](bool hide) {
		w->minLabel->setHidden(hide);
		w->maxLabel->setHidden(hide);
	};

	auto variant = w->modeSelect->currentData();
	if (!variant.canConvert<frame_rate_tag>() || variant.value<frame_rate_tag>().type != frame_rate_tag::RATIONAL) {
		Hide(true);
		return;
	}

	variant = w->fpsRange->currentData();
	if (!variant.canConvert<size_t>()) {
		Hide(true);
		return;
	}

	auto idx = variant.value<size_t>();
	if (idx >= w->fps_ranges.size()) {
		Hide(true);
		return;
	}

	Hide(false);

	auto min = w->fps_ranges[idx].first;
	auto max = w->fps_ranges[idx].second;

	w->minLabel->setText(QString("Min FPS: %1/%2").arg(min.numerator).arg(min.denominator));
	w->maxLabel->setText(QString("Max FPS: %1/%2").arg(max.numerator).arg(max.denominator));
}

static void UpdateFPSLabels(OBSFrameRatePropertyWidget *w)
{
	UpdateMinMaxLabels(w);

	unique_ptr<obs_data_item_t> obj{obs_data_item_byname(w->settings, w->name)};

	media_frames_per_second fps{};
	media_frames_per_second *valid_fps = nullptr;
	if (obs_data_item_get_autoselect_frames_per_second(obj.get(), &fps, nullptr) ||
	    obs_data_item_get_frames_per_second(obj.get(), &fps, nullptr))
		valid_fps = &fps;

	const char *option = nullptr;
	obs_data_item_get_frames_per_second(obj.get(), nullptr, &option);

	if (!valid_fps) {
		w->currentFPS->setHidden(true);
		w->timePerFrame->setHidden(true);
		if (!option)
			w->warningLabel->setProperty("class", "text-danger");

		return;
	}

	w->currentFPS->setHidden(false);
	w->timePerFrame->setHidden(false);

	media_frames_per_second match{};
	if (!option && !matches_ranges(match, *valid_fps, w->fps_ranges, true))
		w->warningLabel->setProperty("class", "text-danger");
	else
		w->warningLabel->setProperty("class", "");

	auto convert_to_fps = media_frames_per_second_to_fps;
	auto convert_to_frame_interval = media_frames_per_second_to_frame_interval;

	w->currentFPS->setText(QString("FPS: %1").arg(convert_to_fps(*valid_fps)));
	w->timePerFrame->setText(QString("Frame Interval: %1 ms").arg(convert_to_frame_interval(*valid_fps) * 1000));
}

void OBSPropertiesView::AddFrameRate(obs_property_t *prop, bool &warning, QFormLayout *layout, QLabel *&label)
{
	const char *name = obs_property_name(prop);
	bool enabled = obs_property_enabled(prop);
	unique_ptr<obs_data_item_t> obj{obs_data_item_byname(settings, name)};

	const char *option = nullptr;
	obs_data_item_get_frames_per_second(obj.get(), nullptr, &option);

	media_frames_per_second fps{};
	media_frames_per_second *valid_fps = nullptr;
	if (obs_data_item_get_frames_per_second(obj.get(), &fps, nullptr))
		valid_fps = &fps;

	frame_rate_ranges_t fps_ranges;
	size_t num = obs_property_frame_rate_fps_ranges_count(prop);
	fps_ranges.reserve(num);
	for (size_t i = 0; i < num; i++)
		fps_ranges.emplace_back(obs_property_frame_rate_fps_range_min(prop, i),
					obs_property_frame_rate_fps_range_max(prop, i));

	auto widget = CreateFrameRateWidget(prop, warning, option, valid_fps, fps_ranges);
	auto info = new WidgetInfo(this, prop, widget);

	widget->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	widget->name = name;
	widget->settings = settings;

	widget->modeSelect->setEnabled(enabled);
	widget->simpleFPS->setEnabled(enabled);
	widget->fpsRange->setEnabled(enabled);
	widget->numEdit->setEnabled(enabled);
	widget->denEdit->setEnabled(enabled);

	label = widget->warningLabel = new QLabel{obs_property_description(prop)};

	layout->addRow(label, widget);

	children.emplace_back(info);

	UpdateFPSLabels(widget);

	auto stack = widget->modeDisplay;
	auto combo = widget->modeSelect;

	stack->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	auto comboIndexChanged = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
	connect(combo, comboIndexChanged, stack, [=](int index) {
		bool out_of_bounds = index >= stack->count();
		auto idx = out_of_bounds ? stack->count() - 1 : index;
		stack->setCurrentIndex(idx);

		if (widget->updating)
			return;

		UpdateFPSLabels(widget);
		emit info->ControlChanged();
	});

	connect(widget->simpleFPS, comboIndexChanged, [=](int) {
		if (widget->updating)
			return;

		emit info->ControlChanged();
	});

	connect(widget->fpsRange, comboIndexChanged, [=](int) {
		if (widget->updating)
			return;

		UpdateFPSLabels(widget);
	});

	auto sbValueChanged = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
	connect(widget->numEdit, sbValueChanged, [=](int) {
		if (widget->updating)
			return;

		emit info->ControlChanged();
	});

	connect(widget->denEdit, sbValueChanged, [=](int) {
		if (widget->updating)
			return;

		emit info->ControlChanged();
	});
}

void OBSPropertiesView::AddGroup(obs_property_t *prop, QFormLayout *layout)
{
	const char *name = obs_property_name(prop);
	bool val = obs_data_get_bool(settings, name);
	const char *desc = obs_property_description(prop);
	enum obs_group_type type = obs_property_group_type(prop);

	// Create GroupBox
	QGroupBox *groupBox = new QGroupBox(QT_UTF8(desc));
	groupBox->setCheckable(type == OBS_GROUP_CHECKABLE);
	groupBox->setChecked(groupBox->isCheckable() ? val : true);
	groupBox->setAccessibleName("group");
	groupBox->setEnabled(obs_property_enabled(prop));

	// Create Layout and build content
	QFormLayout *subLayout = new QFormLayout();
	subLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	groupBox->setLayout(subLayout);

	obs_properties_t *content = obs_property_group_content(prop);
	obs_property_t *el = obs_properties_first(content);
	while (el != nullptr) {
		AddProperty(el, subLayout);
		obs_property_next(&el);
	}

	// Insert into UI
	layout->setWidget(layout->rowCount(), QFormLayout::ItemRole::SpanningRole, groupBox);

	// Register Group Widget
	WidgetInfo *info = new WidgetInfo(this, prop, groupBox);
	children.emplace_back(info);

	// Signals
	connect(groupBox, &QGroupBox::toggled, info, &WidgetInfo::ControlChanged);
}

void OBSPropertiesView::AddProperty(obs_property_t *property, QFormLayout *layout)
{
	const char *name = obs_property_name(property);
	obs_property_type type = obs_property_get_type(property);

	if (!obs_property_visible(property))
		return;

	QLabel *label = nullptr;
	QWidget *widget = nullptr;
	bool warning = false;

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
	case OBS_PROPERTY_FRAME_RATE:
		AddFrameRate(property, warning, layout, label);
		break;
	case OBS_PROPERTY_GROUP:
		AddGroup(property, layout);
		break;
	case OBS_PROPERTY_COLOR_ALPHA:
		AddColorAlpha(property, layout, label);
	}

	if (!widget && !label)
		return;

	if (!label && type != OBS_PROPERTY_BOOL && type != OBS_PROPERTY_BUTTON && type != OBS_PROPERTY_GROUP)
		label = new QLabel(QT_UTF8(obs_property_description(property)));

	if (label) {
		if (warning)
			label->setObjectName("errorLabel");

		if (minSize) {
			label->setMinimumWidth(minSize);
			label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		}

		if (!obs_property_enabled(property))
			label->setEnabled(false);
	}

	if (!widget)
		return;

	if (!obs_property_enabled(property))
		widget->setEnabled(false);

	QWidget *leftWidget = label;
	if (obs_property_long_description(property) && label) {
		QString file = !obs_frontend_is_theme_dark() ? ":/res/images/help.svg" : ":/res/images/help_light.svg";

		QWidget *newWidget = new QWidget();
		newWidget->setToolTip(obs_property_long_description(property));

		QHBoxLayout *boxLayout = new QHBoxLayout(newWidget);
		boxLayout->setContentsMargins(0, 0, 0, 0);
		boxLayout->setAlignment(Qt::AlignLeft);
		boxLayout->setSpacing(0);

		IconLabel *help = new IconLabel(newWidget);
		help->setIcon(QIcon(file));
		help->setToolTip(obs_property_long_description(property));

		boxLayout->addWidget(label);
		boxLayout->addWidget(help);
		leftWidget = newWidget;
	}

	layout->addRow(leftWidget, widget);

	if (!lastFocused.empty())
		if (lastFocused.compare(name) == 0)
			lastWidget = widget;
}

void OBSPropertiesView::SignalChanged()
{
	emit Changed();
}

static bool FrameRateChangedVariant(const QVariant &variant, media_frames_per_second &fps, obs_data_item_t *&obj,
				    const media_frames_per_second *valid_fps)
{
	if (!variant.canConvert<media_frames_per_second>())
		return false;

	fps = variant.value<media_frames_per_second>();
	if (valid_fps && fps == *valid_fps)
		return false;

	obs_data_item_set_frames_per_second(&obj, fps, nullptr);
	return true;
}

static bool FrameRateChangedCommon(OBSFrameRatePropertyWidget *w, obs_data_item_t *&obj,
				   const media_frames_per_second *valid_fps)
{
	media_frames_per_second fps{};
	if (!FrameRateChangedVariant(w->simpleFPS->currentData(), fps, obj, valid_fps))
		return false;

	UpdateRationalFPSWidgets(w, &fps);
	return true;
}

static bool FrameRateChangedRational(OBSFrameRatePropertyWidget *w, obs_data_item_t *&obj,
				     const media_frames_per_second *valid_fps)
{
	auto num = w->numEdit->value();
	auto den = w->denEdit->value();

	auto fps = make_fps(num, den);
	if (valid_fps && media_frames_per_second_is_valid(fps) && fps == *valid_fps)
		return false;

	obs_data_item_set_frames_per_second(&obj, fps, nullptr);
	UpdateSimpleFPSSelection(w, &fps);
	return true;
}

static bool FrameRateChanged(QWidget *widget, const char *name, OBSData &settings)
{
	auto w = qobject_cast<OBSFrameRatePropertyWidget *>(widget);
	if (!w)
		return false;

	auto variant = w->modeSelect->currentData();
	if (!variant.canConvert<frame_rate_tag>())
		return false;

	auto StopUpdating = [&](void *) {
		w->updating = false;
	};
	unique_ptr<void, decltype(StopUpdating)> signalGuard(static_cast<void *>(w), StopUpdating);
	w->updating = true;

	if (!obs_data_has_user_value(settings, name))
		obs_data_set_obj(settings, name, nullptr);

	unique_ptr<obs_data_item_t> obj{obs_data_item_byname(settings, name)};
	auto obj_ptr = obj.get();
	auto CheckObj = [&]() {
		if (!obj_ptr)
			obj.release();
	};

	const char *option = nullptr;
	obs_data_item_get_frames_per_second(obj.get(), nullptr, &option);

	media_frames_per_second fps{};
	media_frames_per_second *valid_fps = nullptr;
	if (obs_data_item_get_frames_per_second(obj.get(), &fps, nullptr))
		valid_fps = &fps;

	auto tag = variant.value<frame_rate_tag>();
	switch (tag.type) {
	case frame_rate_tag::SIMPLE:
		if (!FrameRateChangedCommon(w, obj_ptr, valid_fps))
			return false;
		break;

	case frame_rate_tag::RATIONAL:
		if (!FrameRateChangedRational(w, obj_ptr, valid_fps))
			return false;
		break;

	case frame_rate_tag::USER:
		if (tag.val && option && strcmp(tag.val, option) == 0)
			return false;

		obs_data_item_set_frames_per_second(&obj_ptr, {}, tag.val);
		break;
	}

	UpdateFPSLabels(w);
	CheckObj();
	return true;
}

void WidgetInfo::BoolChanged(const char *setting)
{
	QCheckBox *checkbox = static_cast<QCheckBox *>(widget);
	obs_data_set_bool(view->settings, setting, checkbox->checkState() == Qt::Checked);
}

void WidgetInfo::IntChanged(const char *setting)
{
	QSpinBox *spin = static_cast<QSpinBox *>(widget);
	obs_data_set_int(view->settings, setting, spin->value());
}

void WidgetInfo::FloatChanged(const char *setting)
{
	QDoubleSpinBox *spin = static_cast<QDoubleSpinBox *>(widget);
	obs_data_set_double(view->settings, setting, spin->value());
}

void WidgetInfo::TextChanged(const char *setting)
{
	obs_text_type type = obs_property_text_type(property);

	if (type == OBS_TEXT_MULTILINE) {
		OBSPlainTextEdit *edit = static_cast<OBSPlainTextEdit *>(widget);
		obs_data_set_string(view->settings, setting, QT_TO_UTF8(edit->toPlainText()));
		return;
	}

	QLineEdit *edit = static_cast<QLineEdit *>(widget);
	obs_data_set_string(view->settings, setting, QT_TO_UTF8(edit->text()));
}

bool WidgetInfo::PathChanged(const char *setting)
{
	const char *desc = obs_property_description(property);
	obs_path_type type = obs_property_path_type(property);
	const char *filter = obs_property_path_filter(property);
	const char *default_path = obs_property_path_default_path(property);

	QLineEdit *edit = static_cast<QLineEdit *>(widget);

	QString startDir = edit->text();
	if (startDir.isEmpty())
		startDir = default_path;

	QString path;

	if (type == OBS_PATH_DIRECTORY)
		path = SelectDirectory(view, QT_UTF8(desc), startDir);
	else if (type == OBS_PATH_FILE)
		path = OpenFile(view, QT_UTF8(desc), startDir, QT_UTF8(filter));
	else if (type == OBS_PATH_FILE_SAVE)
		path = SaveFile(view, QT_UTF8(desc), startDir, QT_UTF8(filter));

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if (path.isEmpty())
		return false;

	edit->setText(path);
	obs_data_set_string(view->settings, setting, QT_TO_UTF8(path));
	return true;
}

void WidgetInfo::ListChanged(const char *setting)
{
	obs_combo_format format = obs_property_list_format(property);
	obs_combo_type type = obs_property_list_type(property);
	QVariant data;

	if (type == OBS_COMBO_TYPE_RADIO) {
		QButtonGroup *group = static_cast<QAbstractButton *>(widget)->group();
		QAbstractButton *button = group->checkedButton();
		data = button->property("value");
	} else if (type == OBS_COMBO_TYPE_EDITABLE) {
		data = static_cast<QComboBox *>(widget)->currentText().toUtf8();
	} else {
		QComboBox *combo = static_cast<QComboBox *>(widget);
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
		obs_data_set_int(view->settings, setting, data.value<long long>());
		break;
	case OBS_COMBO_FORMAT_FLOAT:
		obs_data_set_double(view->settings, setting, data.value<double>());
		break;
	case OBS_COMBO_FORMAT_STRING:
		obs_data_set_string(view->settings, setting, data.toByteArray().constData());
		break;
	case OBS_COMBO_FORMAT_BOOL:
		obs_data_set_bool(view->settings, setting, data.value<double>());
		break;
	}
}

bool WidgetInfo::ColorChangedInternal(const char *setting, bool supportAlpha)
{
	const char *desc = obs_property_description(property);
	long long val = obs_data_get_int(view->settings, setting);
	QColor color = color_from_int(val);
	QColor::NameFormat format;

	QColorDialog::ColorDialogOptions options;

	if (supportAlpha) {
		options |= QColorDialog::ShowAlphaChannel;
	}

#ifdef __linux__
	// TODO: Revisit hang on Ubuntu with native dialog
	options |= QColorDialog::DontUseNativeDialog;
#endif

	color = QColorDialog::getColor(color, view, QT_UTF8(desc), options);

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if (!color.isValid())
		return false;

	if (supportAlpha) {
		format = QColor::HexArgb;
	} else {
		color.setAlpha(255);
		format = QColor::HexRgb;
	}

	QLabel *label = static_cast<QLabel *>(widget);
	label->setText(color.name(format));
	QPalette palette = QPalette(color);
	label->setPalette(palette);
	label->setStyleSheet(QString("background-color :%1; color: %2;")
				     .arg(palette.color(QPalette::Window).name(format))
				     .arg(palette.color(QPalette::WindowText).name(format)));

	obs_data_set_int(view->settings, setting, color_to_int(color));

	return true;
}

bool WidgetInfo::ColorChanged(const char *setting)
{
	return ColorChangedInternal(setting, false);
}

bool WidgetInfo::ColorAlphaChanged(const char *setting)
{
	return ColorChangedInternal(setting, true);
}

bool WidgetInfo::FontChanged(const char *setting)
{
	OBSDataAutoRelease font_obj = obs_data_get_obj(view->settings, setting);
	bool success;
	uint32_t flags;
	QFont font;

	QFontDialog::FontDialogOptions options;

#ifndef _WIN32
	options = QFontDialog::DontUseNativeDialog;
#endif

	if (!font_obj) {
		QFont initial;
		font = QFontDialog::getFont(&success, initial, view,
					    tr("Basic.PropertiesWindow.SelectFont.WindowTitle"), options);
	} else {
		MakeQFont(font_obj, font);
		font = QFontDialog::getFont(&success, font, view, tr("Basic.PropertiesWindow.SelectFont.WindowTitle"),
					    options);
	}

	if (!success)
		return false;

	font_obj = obs_data_create();

	obs_data_set_string(font_obj, "face", QT_TO_UTF8(font.family()));
	obs_data_set_string(font_obj, "style", QT_TO_UTF8(font.styleName()));
	obs_data_set_int(font_obj, "size", font.pointSize());
	flags = font.bold() ? OBS_FONT_BOLD : 0;
	flags |= font.italic() ? OBS_FONT_ITALIC : 0;
	flags |= font.underline() ? OBS_FONT_UNDERLINE : 0;
	flags |= font.strikeOut() ? OBS_FONT_STRIKEOUT : 0;
	obs_data_set_int(font_obj, "flags", flags);

	QLabel *label = static_cast<QLabel *>(widget);
	QFont labelFont;
	MakeQFont(font_obj, labelFont, true);
	label->setFont(labelFont);
	label->setText(QString("%1 %2").arg(font.family(), font.styleName()));

	obs_data_set_obj(view->settings, setting, font_obj);
	return true;
}

void WidgetInfo::GroupChanged(const char *setting)
{
	QGroupBox *groupbox = static_cast<QGroupBox *>(widget);
	obs_data_set_bool(view->settings, setting, groupbox->isCheckable() ? groupbox->isChecked() : true);
}

void WidgetInfo::EditableListChanged()
{
	const char *setting = obs_property_name(property);
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);
		OBSDataAutoRelease arrayItem = obs_data_create();
		obs_data_set_string(arrayItem, "value", QT_TO_UTF8(item->text()));
		obs_data_set_string(arrayItem, "uuid", QT_TO_UTF8(item->data(Qt::UserRole).toString()));
		obs_data_set_bool(arrayItem, "selected", item->isSelected());
		obs_data_set_bool(arrayItem, "hidden", item->isHidden());
		obs_data_array_push_back(array, arrayItem);
	}

	obs_data_set_array(view->settings, setting, array);

	ControlChanged();
}

void WidgetInfo::ButtonClicked()
{
	obs_button_type type = obs_property_button_type(property);
	const char *savedUrl = obs_property_button_url(property);

	if (type == OBS_BUTTON_URL && strcmp(savedUrl, "") != 0) {
		QUrl url(savedUrl, QUrl::StrictMode);
		if (url.isValid() && (url.scheme().compare("http") == 0 || url.scheme().compare("https") == 0)) {
			QString msg(tr("Basic.PropertiesView.UrlButton.Text"));
			msg += "\n\n";
			msg += QString(tr("Basic.PropertiesView.UrlButton.Text.Url")).arg(savedUrl);

			QMessageBox::StandardButton button =
				OBSMessageBox::question(view->window(), tr("Basic.PropertiesView.UrlButton.OpenUrl"),
							msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::Yes)
				QDesktopServices::openUrl(url);
		}
		return;
	}

	OBSObject strongObj = view->GetObject();
	void *obj = strongObj ? strongObj.Get() : view->rawObj;
	if (obs_property_button_clicked(property, obj)) {
		QMetaObject::invokeMethod(view, "RefreshProperties", Qt::QueuedConnection);
	}
}

void WidgetInfo::TogglePasswordText(bool show)
{
	reinterpret_cast<QLineEdit *>(widget)->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void WidgetInfo::ControlChanged()
{
	const char *setting = obs_property_name(property);
	obs_property_type type = obs_property_get_type(property);

	if (!recently_updated) {
		old_settings_cache = obs_data_create();
		obs_data_apply(old_settings_cache, view->settings);
		obs_data_release(old_settings_cache);
	}

	switch (type) {
	case OBS_PROPERTY_INVALID:
		return;
	case OBS_PROPERTY_BOOL:
		BoolChanged(setting);
		break;
	case OBS_PROPERTY_INT:
		IntChanged(setting);
		break;
	case OBS_PROPERTY_FLOAT:
		FloatChanged(setting);
		break;
	case OBS_PROPERTY_TEXT:
		TextChanged(setting);
		break;
	case OBS_PROPERTY_LIST:
		ListChanged(setting);
		break;
	case OBS_PROPERTY_BUTTON:
		ButtonClicked();
		return;
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
	case OBS_PROPERTY_EDITABLE_LIST:
		break;
	case OBS_PROPERTY_FRAME_RATE:
		if (!FrameRateChanged(widget, setting, view->settings))
			return;
		break;
	case OBS_PROPERTY_GROUP:
		GroupChanged(setting);
		break;
	case OBS_PROPERTY_COLOR_ALPHA:
		if (!ColorAlphaChanged(setting))
			return;
		break;
	}

	if (!recently_updated) {
		recently_updated = true;
		update_timer = new QTimer;
		connect(update_timer, &QTimer::timeout, [this, &ru = recently_updated]() {
			OBSObject strongObj = view->GetObject();
			void *obj = strongObj ? strongObj.Get() : view->rawObj;
			if (obj && view->callback && !view->deferUpdate) {
				view->callback(obj, old_settings_cache, view->settings);
			}

			ru = false;
		});
		connect(update_timer, &QTimer::timeout, &QTimer::deleteLater);
		update_timer->setSingleShot(true);
	}

	if (update_timer) {
		update_timer->stop();
		update_timer->start(500);
	} else {
		blog(LOG_DEBUG, "No update timer or no callback!");
	}

	if (view->visUpdateCb && !view->deferUpdate) {
		OBSObject strongObj = view->GetObject();
		void *obj = strongObj ? strongObj.Get() : view->rawObj;
		if (obj)
			view->visUpdateCb(obj, view->settings);
	}

	view->SignalChanged();

	if (obs_property_modified(property, view->settings)) {
		view->lastFocused = setting;
		QMetaObject::invokeMethod(view, "RefreshProperties", Qt::QueuedConnection);
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

		QString path = OpenFile(this, tr("Browse"), curPath, filter);
		if (path.isEmpty())
			return;

		edit->setText(path);
	}

public:
	EditableItemDialog(QWidget *parent, const QString &text, bool browse, const char *filter_ = nullptr,
			   const char *default_path_ = nullptr)
		: QDialog(parent),
		  filter(QT_UTF8(filter_)),
		  default_path(QT_UTF8(default_path_))
	{
		QHBoxLayout *topLayout = new QHBoxLayout();
		QVBoxLayout *mainLayout = new QVBoxLayout();

		edit = new QLineEdit();
		edit->setText(text);
		topLayout->addWidget(edit);
		topLayout->setAlignment(edit, Qt::AlignVCenter);

		if (browse) {
			QPushButton *browseButton = new QPushButton(tr("Browse"));
			topLayout->addWidget(browseButton);
			topLayout->setAlignment(browseButton, Qt::AlignVCenter);

			connect(browseButton, &QPushButton::clicked, this, &EditableItemDialog::BrowseClicked);
		}

		QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;

		QDialogButtonBox *buttonBox = new QDialogButtonBox(buttons);
		buttonBox->setCenterButtons(true);

		mainLayout->addLayout(topLayout);
		mainLayout->addWidget(buttonBox);

		setLayout(mainLayout);
		resize(QSize(400, 80));

		connect(buttonBox, &QDialogButtonBox::accepted, this, &EditableItemDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &EditableItemDialog::reject);
	}

	inline QString GetText() const { return edit->text(); }
};

void WidgetInfo::EditListAdd()
{
	enum obs_editable_list_type type = obs_property_editable_list_type(property);

	if (type == OBS_EDITABLE_LIST_TYPE_STRINGS) {
		EditListAddText();
		return;
	}

	/* Files and URLs */
	QMenu popup(view->window());

	QAction *action;

	action = new QAction(tr("Basic.PropertiesWindow.AddFiles"), this);
	connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddFiles);
	popup.addAction(action);

	action = new QAction(tr("Basic.PropertiesWindow.AddDir"), this);
	connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddDir);
	popup.addAction(action);

	if (type == OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS) {
		action = new QAction(tr("Basic.PropertiesWindow.AddURL"), this);
		connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddText);
		popup.addAction(action);
	}

	popup.exec(QCursor::pos());
}

void WidgetInfo::EditListAddText()
{
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
	const char *desc = obs_property_description(property);

	EditableItemDialog dialog(widget->window(), QString(), false);
	auto title = tr("Basic.PropertiesWindow.AddEditableListEntry").arg(QT_UTF8(desc));
	dialog.setWindowTitle(title);
	if (dialog.exec() == QDialog::Rejected)
		return;

	QString text = dialog.GetText();
	if (text.isEmpty())
		return;

	QListWidgetItem *item = new QListWidgetItem(text);
	item->setData(Qt::UserRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
	list->addItem(item);

	EditableListChanged();
}

void WidgetInfo::EditListAddFiles()
{
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
	const char *desc = obs_property_description(property);
	const char *filter = obs_property_editable_list_filter(property);
	const char *default_path = obs_property_editable_list_default_path(property);

	QString title = tr("Basic.PropertiesWindow.AddEditableListFiles").arg(QT_UTF8(desc));

	QStringList files = OpenFiles(list, title, QT_UTF8(default_path), QT_UTF8(filter));
#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if (files.count() == 0)
		return;

	for (QString file : files) {
		QListWidgetItem *item = new QListWidgetItem(file);
		item->setData(Qt::UserRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
		list->addItem(item);
	}

	EditableListChanged();
}

void WidgetInfo::EditListAddDir()
{
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
	const char *desc = obs_property_description(property);
	const char *default_path = obs_property_editable_list_default_path(property);

	QString title = tr("Basic.PropertiesWindow.AddEditableListDir").arg(QT_UTF8(desc));

	QString dir = SelectDirectory(list, title, QT_UTF8(default_path));
#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if (dir.isEmpty())
		return;

	QListWidgetItem *item = new QListWidgetItem(dir);
	item->setData(Qt::UserRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
	list->addItem(item);

	EditableListChanged();
}

void WidgetInfo::EditListRemove()
{
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
	QList<QListWidgetItem *> items = list->selectedItems();

	for (QListWidgetItem *item : items)
		delete item;
	EditableListChanged();
}

void WidgetInfo::EditListEdit()
{
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
	enum obs_editable_list_type type = obs_property_editable_list_type(property);
	const char *desc = obs_property_description(property);
	const char *filter = obs_property_editable_list_filter(property);
	QList<QListWidgetItem *> selectedItems = list->selectedItems();

	if (!selectedItems.count())
		return;

	QListWidgetItem *item = selectedItems[0];

	if (type == OBS_EDITABLE_LIST_TYPE_FILES) {
		QDir pathDir(item->text());
		QString path;

		if (pathDir.exists())
			path = SelectDirectory(list, tr("Browse"), item->text());
		else
			path = OpenFile(list, tr("Browse"), item->text(), QT_UTF8(filter));

		if (path.isEmpty())
			return;

		item->setText(path);
		EditableListChanged();
		return;
	}

	EditableItemDialog dialog(widget->window(), item->text(), type != OBS_EDITABLE_LIST_TYPE_STRINGS, filter);
	auto title = tr("Basic.PropertiesWindow.EditEditableListEntry").arg(QT_UTF8(desc));
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
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
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
	QListWidget *list = reinterpret_cast<QListWidget *>(widget);
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
