#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace lcs {

struct TranscriptionResult {
	std::string text;
	std::string language;
	bool is_final = false;
};

using TranscriptionCallback = std::function<void(const TranscriptionResult &)>;

enum class EngineKind {
	Whisper,
	OpenAI,
	Gemini,
	Ollama,
	CustomApi,
};

struct EngineConfig {
	EngineKind kind = EngineKind::Whisper;
	std::string api_key;
	std::string endpoint;
	std::string model;
	std::string source_language = "auto";
	std::string target_language;
	bool translate = false;
	bool ai_correct = false;
};

class ITranscriptionEngine {
public:
	virtual ~ITranscriptionEngine() = default;

	virtual const char *name() const = 0;
	virtual EngineKind kind() const = 0;
	virtual bool configure(const EngineConfig &config) = 0;
	virtual bool start(TranscriptionCallback callback) = 0;
	virtual void stop() = 0;
	virtual bool is_running() const = 0;

	/* Push PCM float32 mono/stereo frames for live transcription. */
	virtual void push_audio(const float *samples, size_t frames, size_t channels, uint32_t sample_rate) = 0;
};

std::unique_ptr<ITranscriptionEngine> create_engine(EngineKind kind);
const char *engine_kind_name(EngineKind kind);
std::vector<EngineKind> available_engines();

} // namespace lcs
