#include "transcription/engine-registry.hpp"

#include <obs-module.h>

namespace lcs {

namespace {

class StubEngine : public ITranscriptionEngine {
public:
	explicit StubEngine(EngineKind kind) : kind_(kind) {}

	const char *name() const override { return engine_kind_name(kind_); }
	EngineKind kind() const override { return kind_; }

	bool configure(const EngineConfig &config) override
	{
		config_ = config;
		blog(LOG_INFO, "[Live Caption Studio] Configured engine '%s' (model=%s)", name(),
		     config_.model.empty() ? "(default)" : config_.model.c_str());
		return true;
	}

	bool start(TranscriptionCallback callback) override
	{
		callback_ = std::move(callback);
		running_ = true;
		blog(LOG_INFO, "[Live Caption Studio] Engine '%s' started (stub)", name());

		if (callback_) {
			TranscriptionResult result;
			result.text = std::string("[") + name() + "] Ready — connect a real backend later.";
			result.language = config_.source_language;
			result.is_final = true;
			callback_(result);
		}
		return true;
	}

	void stop() override
	{
		running_ = false;
		blog(LOG_INFO, "[Live Caption Studio] Engine '%s' stopped", name());
	}

	bool is_running() const override { return running_; }

	void push_audio(const float *, size_t, size_t, uint32_t) override
	{
		/* Stub: real engines will buffer and send audio asynchronously. */
	}

private:
	EngineKind kind_;
	EngineConfig config_;
	TranscriptionCallback callback_;
	bool running_ = false;
};

} // namespace

const char *engine_kind_name(EngineKind kind)
{
	switch (kind) {
	case EngineKind::Whisper:
		return "Whisper";
	case EngineKind::OpenAI:
		return "OpenAI";
	case EngineKind::Gemini:
		return "Gemini";
	case EngineKind::Ollama:
		return "Ollama";
	case EngineKind::CustomApi:
		return "Custom API";
	}
	return "Unknown";
}

std::vector<EngineKind> available_engines()
{
	return {EngineKind::Whisper, EngineKind::OpenAI, EngineKind::Gemini, EngineKind::Ollama, EngineKind::CustomApi};
}

std::unique_ptr<ITranscriptionEngine> create_engine(EngineKind kind)
{
	return std::make_unique<StubEngine>(kind);
}

EngineRegistry &EngineRegistry::instance()
{
	static EngineRegistry registry;
	return registry;
}

void EngineRegistry::set_config(const EngineConfig &config)
{
	std::lock_guard lock(mutex_);
	config_ = config;
	if (engine_ && engine_->is_running()) {
		engine_->stop();
		engine_.reset();
	}
}

EngineConfig EngineRegistry::config() const
{
	std::lock_guard lock(mutex_);
	return config_;
}

bool EngineRegistry::start()
{
	EngineConfig config;
	{
		std::lock_guard lock(mutex_);
		if (engine_ && engine_->is_running())
			engine_->stop();
		engine_.reset();
		config = config_;
	}

	auto engine = create_engine(config.kind);
	if (!engine->configure(config))
		return false;

	auto cb = [this](const TranscriptionResult &result) {
		TranscriptionCallback user;
		{
			std::lock_guard lock(mutex_);
			last_text_ = result.text;
			user = user_callback_;
		}
		if (user)
			user(result);
	};

	if (!engine->start(std::move(cb)))
		return false;

	std::lock_guard lock(mutex_);
	engine_ = std::move(engine);
	return true;
}

void EngineRegistry::stop()
{
	std::lock_guard lock(mutex_);
	if (engine_) {
		engine_->stop();
		engine_.reset();
	}
}

bool EngineRegistry::is_running() const
{
	std::lock_guard lock(mutex_);
	return engine_ && engine_->is_running();
}

void EngineRegistry::push_audio(const float *samples, size_t frames, size_t channels, uint32_t sample_rate)
{
	std::lock_guard lock(mutex_);
	if (engine_)
		engine_->push_audio(samples, frames, channels, sample_rate);
}

void EngineRegistry::set_result_callback(TranscriptionCallback callback)
{
	std::lock_guard lock(mutex_);
	user_callback_ = std::move(callback);
}

std::string EngineRegistry::last_text() const
{
	std::lock_guard lock(mutex_);
	return last_text_;
}

} // namespace lcs
