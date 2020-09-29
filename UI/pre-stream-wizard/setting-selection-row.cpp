#include "setting-selection-row.hpp"

#include <QHBoxLayout>

namespace StreamWizard {

SelectionRow::SelectionRow(QWidget *parent) : QWidget(parent)
{
	createLayout();
}

void SelectionRow::createLayout()
{
	QHBoxLayout *mainlayout = new QHBoxLayout(this);
	QMargins margins = QMargins(3, 3, 3, 3);
	mainlayout->setContentsMargins(margins);
	checkboxValueString_ = QString();
	checkbox_ = new QCheckBox(checkboxValueString_);
	checkbox_->setChecked(true);

	setLayout(mainlayout);

	mainlayout->addWidget(checkbox_);
	QSpacerItem *checkboxSpace = new QSpacerItem(
		6, 6, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	mainlayout->addSpacerItem(checkboxSpace);
	connect(checkbox_, &QCheckBox::stateChanged, this,
		&SelectionRow::checkboxUpdated);
}
void SelectionRow::checkboxUpdated()
{
	emit didChangeSelectedStatus(propertyKey_, checkbox_->isChecked());
}

void SelectionRow::updateLabel()
{
	checkboxValueString_ = name_ + separator_ + valueLabel_;
	checkbox_->setText(checkboxValueString_);
}

void SelectionRow::setName(QString newName)
{
	name_ = newName;
	updateLabel();
}

QString SelectionRow::getName()
{
	return name_;
}

void SelectionRow::setValueLabel(QString newLabel)
{
	valueLabel_ = newLabel;
	updateLabel();
}

QString SelectionRow::getValueLabel()
{
	return valueLabel_;
}

void SelectionRow::setPropertyKey(const char *newKey)
{
	propertyKey_ = newKey;
}

const char *SelectionRow::getPropertyKey()
{
	return propertyKey_;
}

} // namespace StreamWizard
