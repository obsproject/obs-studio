#include "OBSDynamicDelayDock.hpp"
#include <OBSApp.hpp>
#include <obs-frontend-api.h>
#include <qt-wrappers.hpp>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QStyle>
#include <QScrollArea>

#include "moc_OBSDynamicDelayDock.cpp"

OBSDynamicDelayDock::OBSDynamicDelayDock(QWidget *parent) : OBSDock(QTStr("Basic.DynamicDelay"), parent)
{
	setObjectName(QStringLiteral("dynamicDelayDock"));
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
		    QDockWidget::DockWidgetFloatable);

	QScrollArea *scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);

	QWidget *mainWidget = new QWidget(scrollArea);
	QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
	mainLayout->setSpacing(10);
	mainLayout->setContentsMargins(10, 10, 10, 10);

	// 1. Target Delay Section
	QGroupBox *targetGroup = new QGroupBox(QTStr("Basic.DynamicDelay.TargetGroup"), mainWidget);
	QVBoxLayout *targetLayout = new QVBoxLayout(targetGroup);

	QLabel *targetHelp = new QLabel(QTStr("Basic.DynamicDelay.TargetHelp"), targetGroup);
	targetHelp->setWordWrap(true);
	targetLayout->addWidget(targetHelp);

	QHBoxLayout *sliderLayout = new QHBoxLayout();
	targetSlider = new QSlider(Qt::Horizontal, targetGroup);
	targetSlider->setRange(10, 600);
	targetSlider->setValue(180);

	targetSpinBox = new QSpinBox(targetGroup);
	targetSpinBox->setRange(10, 600);
	targetSpinBox->setValue(180);
	targetSpinBox->setSuffix(QTStr("Basic.DynamicDelay.SecondsSuffix"));

	sliderLayout->addWidget(targetSlider);
	sliderLayout->addWidget(targetSpinBox);
	targetLayout->addLayout(sliderLayout);
	mainLayout->addWidget(targetGroup);

	// 2. Temporary Media Section (Fallback)
	QGroupBox *mediaGroup = new QGroupBox(QTStr("Basic.DynamicDelay.MediaGroup"), mainWidget);
	QVBoxLayout *mediaLayout = new QVBoxLayout(mediaGroup);

	QLabel *mediaHelp = new QLabel(QTStr("Basic.DynamicDelay.MediaHelp"), mediaGroup);
	mediaHelp->setWordWrap(true);
	mediaLayout->addWidget(mediaHelp);

	QHBoxLayout *fileLayout = new QHBoxLayout();
	mediaPathEdit = new QLineEdit(mediaGroup);
	mediaPathEdit->setPlaceholderText(QTStr("Basic.DynamicDelay.NoMediaSelected"));
	mediaPathEdit->setReadOnly(true);

	browseBtn = new QPushButton(QTStr("Basic.DynamicDelay.Browse"), mediaGroup);
	fileLayout->addWidget(mediaPathEdit);
	fileLayout->addWidget(browseBtn);
	mediaLayout->addLayout(fileLayout);
	mainLayout->addWidget(mediaGroup);

	// 3. Status & Indicators Section
	QGroupBox *statusGroup = new QGroupBox(QTStr("Basic.DynamicDelay.StatusGroup"), mainWidget);
	QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);

	statusLabel = new QLabel(QTStr("Basic.DynamicDelay.Status.Offline"), statusGroup);
	QFont boldFont = statusLabel->font();
	boldFont.setBold(true);
	statusLabel->setFont(boldFont);
	statusLayout->addWidget(statusLabel);

	progressBar = new QProgressBar(statusGroup);
	progressBar->setRange(0, 180000);
	progressBar->setValue(0);
	progressBar->setTextVisible(false);
	statusLayout->addWidget(progressBar);

	QHBoxLayout *statsLayout = new QHBoxLayout();
	bufferedLabel = new QLabel(QTStr("Basic.DynamicDelay.Buffered").arg("0.0").arg("180"), statusGroup);
	memoryLabel = new QLabel(QTStr("Basic.DynamicDelay.MemoryUsage").arg("0.00").arg("500.00"), statusGroup);
	statsLayout->addWidget(bufferedLabel);
	statsLayout->addStretch();
	statsLayout->addWidget(memoryLabel);
	statusLayout->addLayout(statsLayout);
	mainLayout->addWidget(statusGroup);

	// 4. Toggle Button
	toggleBtn = new QPushButton(QTStr("Basic.DynamicDelay.Enable"), mainWidget);
	toggleBtn->setMinimumHeight(40);
	QFont btnFont = toggleBtn->font();
	btnFont.setBold(true);
	btnFont.setPointSize(btnFont.pointSize() + 1);
	toggleBtn->setFont(btnFont);
	toggleBtn->setEnabled(false);
	mainLayout->addWidget(toggleBtn);

	mainLayout->addStretch();
	scrollArea->setWidget(mainWidget);
	setWidget(scrollArea);

	// Connect signals
	connect(targetSlider, &QSlider::valueChanged, this, &OBSDynamicDelayDock::OnTargetDelayChanged);
	connect(targetSpinBox, &QSpinBox::valueChanged, this, &OBSDynamicDelayDock::OnTargetDelayChanged);
	connect(browseBtn, &QPushButton::clicked, this, &OBSDynamicDelayDock::OnBrowseMedia);
	connect(toggleBtn, &QPushButton::clicked, this, &OBSDynamicDelayDock::OnToggleDelay);

	// Setup update timer
	connect(&updateTimer, &QTimer::timeout, this, &OBSDynamicDelayDock::UpdateStats);
	updateTimer.start(500);
}

OBSDynamicDelayDock::~OBSDynamicDelayDock()
{
	updateTimer.stop();
}

void OBSDynamicDelayDock::OnTargetDelayChanged(int value)
{
	if (targetSlider->value() != value) {
		targetSlider->setValue(value);
	}
	if (targetSpinBox->value() != value) {
		targetSpinBox->setValue(value);
	}

	progressBar->setMaximum(value * 1000);

	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
	if (output) {
		obs_output_set_dynamic_delay_target_sec(output, value);
	}
}

void OBSDynamicDelayDock::OnBrowseMedia()
{
	QString path = QFileDialog::getOpenFileName(this, QTStr("Basic.DynamicDelay.SelectMedia"), QString(),
						    QTStr("Basic.DynamicDelay.MediaFilter") + QStringLiteral(";;") +
							    QTStr("Basic.DynamicDelay.AllFiles"));
	if (!path.isEmpty()) {
		mediaPathEdit->setText(path);
		OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
		if (output) {
			obs_output_set_dynamic_delay_waiting_media(output, path.toUtf8().constData());
		}
	}
}

void OBSDynamicDelayDock::OnToggleDelay()
{
	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
	if (output) {
		obs_output_dynamic_delay_toggle(output);
		UpdateStats();
	}
}

void OBSDynamicDelayDock::UpdateStats()
{
	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
	if (!output) {
		statusLabel->setText(QTStr("Basic.DynamicDelay.Status.Offline"));
		statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
		toggleBtn->setEnabled(false);
		toggleBtn->setText(QTStr("Basic.DynamicDelay.Enable"));
		progressBar->setValue(0);
		bufferedLabel->setText(QTStr("Basic.DynamicDelay.Buffered").arg("0.0").arg("0.0"));
		memoryLabel->setText(QTStr("Basic.DynamicDelay.MemoryUsage").arg("0.00").arg("500.00"));
		return;
	}

	toggleBtn->setEnabled(true);
	bool enabled = obs_output_get_dynamic_delay_enabled(output);
	int state = obs_output_get_dynamic_delay_state(output);
	uint64_t buf_ms = obs_output_get_dynamic_delay_buffered_ms(output);
	uint64_t mem_bytes = obs_output_get_dynamic_delay_memory_bytes(output);
	int target_sec = obs_output_get_dynamic_delay_target_sec(output);
	if (target_sec <= 0) {
		target_sec = 180;
	}

	// Sync target slider if changed externally
	if (targetSlider->value() != target_sec && !targetSlider->isSliderDown()) {
		targetSlider->setValue(target_sec);
		targetSpinBox->setValue(target_sec);
	}

	// Sync media path if changed externally or on start
	const char *media_path = obs_output_get_dynamic_delay_waiting_media(output);
	if (media_path && mediaPathEdit->text().isEmpty()) {
		mediaPathEdit->setText(QString::fromUtf8(media_path));
	} else if (!media_path && !mediaPathEdit->text().isEmpty()) {
		obs_output_set_dynamic_delay_waiting_media(output, mediaPathEdit->text().toUtf8().constData());
	}

	if (!enabled || state == 0) {
		statusLabel->setText(QTStr("Basic.DynamicDelay.Status.Live"));
		statusLabel->setStyleSheet(QStringLiteral("color: #4CAF50;")); // Green
		toggleBtn->setText(QTStr("Basic.DynamicDelay.Enable"));
	} else if (state == 1) {
		statusLabel->setText(QTStr("Basic.DynamicDelay.Status.Accumulating"));
		statusLabel->setStyleSheet(QStringLiteral("color: #FF9800;")); // Orange/Amber
		toggleBtn->setText(QTStr("Basic.DynamicDelay.Disable"));
	} else if (state == 2) {
		statusLabel->setText(QTStr("Basic.DynamicDelay.Status.Active"));
		statusLabel->setStyleSheet(QStringLiteral("color: #2196F3;")); // Blue
		toggleBtn->setText(QTStr("Basic.DynamicDelay.Disable"));
	} else if (state == 3) {
		statusLabel->setText(QTStr("Basic.DynamicDelay.Status.ReturningLive"));
		statusLabel->setStyleSheet(QStringLiteral("color: #E91E63;")); // Pink/Purple
		toggleBtn->setText(QTStr("Basic.DynamicDelay.Enable"));
	}

	progressBar->setMaximum(target_sec * 1000);
	progressBar->setValue((int)buf_ms);

	bufferedLabel->setText(QTStr("Basic.DynamicDelay.Buffered")
				       .arg(QString::number(buf_ms / 1000.0, 'f', 1))
				       .arg(QString::number(target_sec)));
	memoryLabel->setText(QTStr("Basic.DynamicDelay.MemoryUsage")
				     .arg(QString::number(mem_bytes / (1024.0 * 1024.0), 'f', 2))
				     .arg(QStringLiteral("500.00")));
}
