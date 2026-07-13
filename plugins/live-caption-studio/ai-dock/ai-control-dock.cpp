#include "ai-dock/ai-control-dock.hpp"
#include "settings/lcs-config.hpp"
#include "transcription/engine-registry.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {

lcs::EngineKind kind_at(int index)
{
	const auto engines = lcs::available_engines();
	if (index < 0 || index >= static_cast<int>(engines.size()))
		return lcs::EngineKind::Whisper;
	return engines[static_cast<size_t>(index)];
}

int index_of(lcs::EngineKind kind)
{
	const auto engines = lcs::available_engines();
	for (int i = 0; i < static_cast<int>(engines.size()); ++i) {
		if (engines[static_cast<size_t>(i)] == kind)
			return i;
	}
	return 0;
}

} // namespace

AIControlDock::AIControlDock(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	statusLabel_ = new QLabel(QString::fromUtf8(obs_module_text("LCS.Status.Idle")), this);
	layout->addWidget(statusLabel_);

	engineCombo_ = new QComboBox(this);
	for (auto kind : lcs::available_engines())
		engineCombo_->addItem(QString::fromUtf8(lcs::engine_kind_name(kind)));
	layout->addWidget(engineCombo_);

	auto *buttons = new QHBoxLayout();
	startStopButton_ = new QPushButton(QString::fromUtf8(obs_module_text("LCS.Action.Start")), this);
	translateButton_ = new QPushButton(QString::fromUtf8(obs_module_text("LCS.Action.Translate")), this);
	translateButton_->setCheckable(true);
	correctButton_ = new QPushButton(QString::fromUtf8(obs_module_text("LCS.Action.AICorrect")), this);
	correctButton_->setCheckable(true);
	buttons->addWidget(startStopButton_);
	buttons->addWidget(translateButton_);
	buttons->addWidget(correctButton_);
	layout->addLayout(buttons);

	preview_ = new QPlainTextEdit(this);
	preview_->setReadOnly(true);
	preview_->setPlaceholderText(QString::fromUtf8(obs_module_text("LCS.Preview.Placeholder")));
	layout->addWidget(preview_, 1);

	const auto settings = lcs::current_settings();
	engineCombo_->setCurrentIndex(index_of(settings.engine.kind));
	translateButton_->setChecked(settings.engine.translate);
	correctButton_->setChecked(settings.engine.ai_correct);

	connect(startStopButton_, &QPushButton::clicked, this, &AIControlDock::onStartStop);
	connect(engineCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&AIControlDock::onEngineChanged);
	connect(translateButton_, &QPushButton::toggled, this, &AIControlDock::onTranslateToggled);
	connect(correctButton_, &QPushButton::toggled, this, &AIControlDock::onCorrectToggled);

	lcs::EngineRegistry::instance().set_result_callback([this](const lcs::TranscriptionResult &result) {
		QMetaObject::invokeMethod(
			this,
			[this, text = QString::fromStdString(result.text)]() { appendCaption(text); },
			Qt::QueuedConnection);
	});

	refreshUiState();
}

AIControlDock::~AIControlDock()
{
	lcs::EngineRegistry::instance().set_result_callback(nullptr);
	lcs::EngineRegistry::instance().stop();
}

void AIControlDock::onStartStop()
{
	auto &registry = lcs::EngineRegistry::instance();
	if (registry.is_running()) {
		registry.stop();
	} else {
		auto settings = lcs::current_settings();
		settings.engine.kind = kind_at(engineCombo_->currentIndex());
		settings.engine.translate = translateButton_->isChecked();
		settings.engine.ai_correct = correctButton_->isChecked();
		lcs::update_settings(settings);
		registry.start();
	}
	refreshUiState();
}

void AIControlDock::onEngineChanged(int index)
{
	auto settings = lcs::current_settings();
	settings.engine.kind = kind_at(index);
	lcs::update_settings(settings);
}

void AIControlDock::onTranslateToggled(bool checked)
{
	auto settings = lcs::current_settings();
	settings.engine.translate = checked;
	lcs::update_settings(settings);
}

void AIControlDock::onCorrectToggled(bool checked)
{
	auto settings = lcs::current_settings();
	settings.engine.ai_correct = checked;
	lcs::update_settings(settings);
}

void AIControlDock::refreshUiState()
{
	const bool running = lcs::EngineRegistry::instance().is_running();
	startStopButton_->setText(QString::fromUtf8(
		running ? obs_module_text("LCS.Action.Stop") : obs_module_text("LCS.Action.Start")));
	engineCombo_->setEnabled(!running);
	statusLabel_->setText(QString::fromUtf8(running ? obs_module_text("LCS.Status.Running")
							: obs_module_text("LCS.Status.Idle")));
}

void AIControlDock::appendCaption(const QString &text)
{
	preview_->appendPlainText(text);
}
