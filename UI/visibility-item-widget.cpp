#include "moc_visibility-item-widget.cpp"
#include "obs-app.hpp"
#include "source-label.hpp"

#include <qt-wrappers.hpp>
#include <QListWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QKeyEvent>
#include <QCheckBox>

VisibilityItemWidget::VisibilityItemWidget(obs_source_t *source_)
	: source(source_),
	  enabledSignal(obs_source_get_signal_handler(source), "enable", OBSSourceEnabled, this)
{
	bool enabled = obs_source_enabled(source);

	vis = new QCheckBox();
	vis->setProperty("class", "checkbox-icon indicator-visibility");
	vis->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	vis->setChecked(enabled);

	label = new OBSSourceLabel(source);
	label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	QHBoxLayout *itemLayout = new QHBoxLayout();
	itemLayout->addWidget(vis);
	itemLayout->addWidget(label);
	itemLayout->setContentsMargins(0, 0, 0, 0);

	setLayout(itemLayout);

	connect(vis, &QCheckBox::clicked, [this](bool visible) { obs_source_set_enabled(source, visible); });
}

void VisibilityItemWidget::OBSSourceEnabled(void *param, calldata_t *data)
{
	VisibilityItemWidget *window = reinterpret_cast<VisibilityItemWidget *>(param);
	bool enabled = calldata_bool(data, "enabled");

	QMetaObject::invokeMethod(window, "SourceEnabled", Q_ARG(bool, enabled));
}

void VisibilityItemWidget::SourceEnabled(bool enabled)
{
	if (vis->isChecked() != enabled)
		vis->setChecked(enabled);
}

void VisibilityItemWidget::SetColor(const QColor &color, bool active_, bool selected_)
{
	/* Do not update unless the state has actually changed */
	if (active_ == active && selected_ == selected)
		return;

	QPalette pal = vis->palette();
	pal.setColor(QPalette::WindowText, color);
	vis->setPalette(pal);

	label->setStyleSheet(QStringLiteral("color: %1;").arg(color.name()));

	active = active_;
	selected = selected_;
}

VisibilityItemDelegate::VisibilityItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void VisibilityItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
				   const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);

	QObject *parentObj = parent();
	QListWidget *list = qobject_cast<QListWidget *>(parentObj);
	if (!list)
		return;

	QListWidgetItem *item = list->item(index.row());
	VisibilityItemWidget *widget = qobject_cast<VisibilityItemWidget *>(list->itemWidget(item));
	if (!widget)
		return;

	bool selected = option.state.testFlag(QStyle::State_Selected);
	bool active = option.state.testFlag(QStyle::State_Active);

	QPalette palette = list->palette();
#if defined(_WIN32) || defined(__APPLE__)
	QPalette::ColorGroup group = active ? QPalette::Active : QPalette::Inactive;
#else
	QPalette::ColorGroup group = QPalette::Active;
#endif

#ifdef _WIN32
	QPalette::ColorRole highlightRole = QPalette::WindowText;
#else
	QPalette::ColorRole highlightRole = QPalette::HighlightedText;
#endif

	QPalette::ColorRole role;

	if (selected && active)
		role = highlightRole;
	else
		role = QPalette::WindowText;

	widget->SetColor(palette.color(group, role), active, selected);
}

bool VisibilityItemDelegate::eventFilter(QObject *object, QEvent *event)
{
	QWidget *editor = qobject_cast<QWidget *>(object);
	if (!editor)
		return false;

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
			return false;
		}
	}

	return QStyledItemDelegate::eventFilter(object, event);
}

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item, obs_source_t *source)
{
	VisibilityItemWidget *baseWidget = new VisibilityItemWidget(source);

	list->setItemWidget(item, baseWidget);
}
