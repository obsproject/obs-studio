#include "VisibilityItemWidget.hpp"

#include <components/OBSSourceLabel.hpp>

#include <QCheckBox>
#include <QHBoxLayout>

#include "moc_VisibilityItemWidget.cpp"

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

	label->setStyleSheet(QString("color: %1;").arg(color.name()));

	active = active_;
	selected = selected_;
}

void SetupVisibilityItem(QListWidget *list, QListWidgetItem *item, obs_source_t *source)
{
	VisibilityItemWidget *baseWidget = new VisibilityItemWidget(source);

	list->setItemWidget(item, baseWidget);
}
