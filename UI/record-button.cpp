#include "record-button.hpp"
#include "window-basic-main.hpp"

void RecordButton::resizeEvent(QResizeEvent *event)
{
	OBSBasic *main = OBSBasic::Get();
	if (!main->pause)
		return;

	QSize pauseSize = main->pause->size();
	int height = main->ui->recordButton->size().height();

	if (pauseSize.height() != height || pauseSize.width() != height) {
		main->pause->setMinimumSize(height, height);
		main->pause->setMaximumSize(height, height);
	}

	event->accept();
}

static QWidget *firstWidget(QLayoutItem *item)
{
	auto widget = item->widget();
	if (widget)
		return widget;

	auto layout = item->layout();
	if (!layout)
		return nullptr;

	auto n = layout->count();
	for (auto i = 0, n = layout->count(); i < n; i++) {
		widget = firstWidget(layout->itemAt(i));
		if (widget)
			return widget;
	}
	return nullptr;
}

static QWidget *lastWidget(QLayoutItem *item)
{
	auto widget = item->widget();
	if (widget)
		return widget;

	auto layout = item->layout();
	if (!layout)
		return nullptr;

	auto n = layout->count();
	for (auto i = layout->count(); i > 0; i--) {
		widget = lastWidget(layout->itemAt(i - 1));
		if (widget)
			return widget;
	}
	return nullptr;
}

static QWidget *getNextWidget(QBoxLayout *container, QLayoutItem *item)
{
	for (auto i = 1, n = container->count(); i < n; i++) {
		if (container->itemAt(i - 1) == item)
			return firstWidget(container->itemAt(i));
	}
	return nullptr;
}

ControlsSplitButton::ControlsSplitButton(const QString &text,
					 const QVariant &themeID,
					 void (OBSBasic::*clicked)())
	: QHBoxLayout(OBSBasic::Get())
{
	button.reset(new QPushButton(text));
	button->setCheckable(true);
	button->setProperty("themeID", themeID);

	button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
	button->installEventFilter(this);

	OBSBasic *main = OBSBasic::Get();
	connect(button.data(), &QPushButton::clicked, main, clicked);

	addWidget(button.data());
}

void ControlsSplitButton::addIcon(const QString &name, const QVariant &themeID,
				  void (OBSBasic::*clicked)())
{
	icon.reset(new QPushButton());
	icon->setAccessibleName(name);
	icon->setToolTip(name);
	icon->setChecked(false);
	icon->setProperty("themeID", themeID);

	QSizePolicy sp;
	sp.setHeightForWidth(true);
	icon->setSizePolicy(sp);

	OBSBasic *main = OBSBasic::Get();
	connect(icon.data(), &QAbstractButton::clicked, main, clicked);

	addWidget(icon.data());
	QWidget::setTabOrder(button.data(), icon.data());

	auto next = getNextWidget(main->ui->buttonsVLayout, this);
	if (next)
		QWidget::setTabOrder(icon.data(), next);
}

void ControlsSplitButton::removeIcon()
{
	icon.reset();
}

void ControlsSplitButton::insert(int index)
{
	OBSBasic *main = OBSBasic::Get();
	auto count = main->ui->buttonsVLayout->count();
	if (index < 0)
		index = 0;
	else if (index > count)
		index = count;

	main->ui->buttonsVLayout->insertLayout(index, this);

	QWidget *prev = button.data();

	if (index > 0) {
		prev = lastWidget(main->ui->buttonsVLayout->itemAt(index - 1));
		if (prev)
			QWidget::setTabOrder(prev, button.data());
		prev = button.data();
	}

	if (icon) {
		QWidget::setTabOrder(button.data(), icon.data());
		prev = icon.data();
	}

	if (index < count) {
		auto next = firstWidget(
			main->ui->buttonsVLayout->itemAt(index + 1));
		if (next)
			QWidget::setTabOrder(prev, next);
	}
}

bool ControlsSplitButton::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Resize && icon) {
		QSize iconSize = icon->size();
		int height = button->height();

		if (iconSize.height() != height || iconSize.width() != height) {
			icon->setMinimumSize(height, height);
			icon->setMaximumSize(height, height);
		}
	}
	return QObject::eventFilter(obj, event);
}
