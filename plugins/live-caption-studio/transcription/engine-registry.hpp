#pragma once

#include "transcription/engine.hpp"

#include <mutex>

namespace lcs {

class EngineRegistry {
public:
	static EngineRegistry &instance();

	void set_config(const EngineConfig &config);
	EngineConfig config() const;

	bool start();
	void stop();
	bool is_running() const;

	void push_audio(const float *samples, size_t frames, size_t channels, uint32_t sample_rate);

	void set_result_callback(TranscriptionCallback callback);
	std::string last_text() const;

private:
	EngineRegistry() = default;

	mutable std::mutex mutex_;
	EngineConfig config_;
	std::unique_ptr<ITranscriptionEngine> engine_;
	TranscriptionCallback user_callback_;
	std::string last_text_;
};

} // namespace lcs
