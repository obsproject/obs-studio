#include "OBSBasicTransitions.hpp"
#include "OBSBasic.hpp"

#include "moc_OBSBasicTransitions.cpp"

OBSBasicTransitions::OBSBasicTransitions(OBSBasic *main) : QFrame(nullptr), ui(new Ui::OBSBasicTransitions)
{
	/* Create UI elements */
	ui->setupUi(this);

	/* Transfer widget signals as OBSBasicTransitions signals */
	connect(
		ui->transitions, &QComboBox::currentIndexChanged, this,
		[this]() { emit this->transitionChanged(ui->transitions->currentData().toString()); },
		Qt::DirectConnection);

	connect(
		ui->transitionDuration, &QSpinBox::valueChanged, this,
		[this](int duration) { emit this->transitionDurationChanged(duration); }, Qt::DirectConnection);

	connect(
		ui->transitionAdd, &QPushButton::clicked, this, [this]() { emit this->addTransitionClicked(); },
		Qt::DirectConnection);

	connect(
		ui->transitionRemove, &QPushButton::clicked, this,
		[this]() { emit this->removeCurrentTransitionClicked(); }, Qt::DirectConnection);

	connect(
		ui->transitionProps, &QPushButton::clicked, this,
		[this]() { emit this->currentTransitionPropertiesMenuClicked(); }, Qt::DirectConnection);

	/* Set up state update connections */
	connect(main, &OBSBasic::TransitionAdded, this, &OBSBasicTransitions::transitionAdded);
	connect(main, &OBSBasic::TransitionRenamed, this, &OBSBasicTransitions::transitionRenamed);
	connect(main, &OBSBasic::TransitionRemoved, this, &OBSBasicTransitions::transitionRemoved);
	connect(main, &OBSBasic::TransitionsCleared, this, &OBSBasicTransitions::transitionsCleared);

	connect(main, &OBSBasic::CurrentTransitionChanged, this, &OBSBasicTransitions::currentTransitionChanged);

	connect(main, &OBSBasic::TransitionDurationChanged, this, &OBSBasicTransitions::durationChanged);

	connect(main, &OBSBasic::transitionsControlChanged, this, &OBSBasicTransitions::transitionsControlChanged);
}

void OBSBasicTransitions::transitionAdded(const QString &name, const QString &uuid)
{
	QSignalBlocker sb(ui->transitions);
	ui->transitions->addItem(name, uuid);
}

void OBSBasicTransitions::transitionRenamed(const QString &uuid, const QString &newName)
{
	QSignalBlocker sb(ui->transitions);
	ui->transitions->setItemText(ui->transitions->findData(uuid), newName);
}

void OBSBasicTransitions::transitionRemoved(const QString &uuid)
{
	QSignalBlocker sb(ui->transitions);
	ui->transitions->removeItem(ui->transitions->findData(uuid));
}

void OBSBasicTransitions::transitionsCleared()
{
	QSignalBlocker sb(ui->transitions);
	ui->transitions->clear();
}

void OBSBasicTransitions::currentTransitionChanged(const QString &uuid, bool fixed, bool configurable)
{
	QSignalBlocker sb(ui->transitions);
	ui->transitions->setCurrentIndex(ui->transitions->findData(uuid));

	ui->transitionDurationLabel->setVisible(!fixed);
	ui->transitionDuration->setVisible(!fixed);

	ui->transitionRemove->setEnabled(configurable);
	ui->transitionProps->setEnabled(configurable);

	transitionConfigurable = configurable;
}

void OBSBasicTransitions::durationChanged(const int &duration)
{
	QSignalBlocker sb(ui->transitionDuration);
	ui->transitionDuration->setValue(duration);
}

void OBSBasicTransitions::transitionsControlChanged(bool locked)
{
	ui->transitions->setDisabled(locked);
	ui->transitionProps->setDisabled(locked ? true : transitionConfigurable);
}
