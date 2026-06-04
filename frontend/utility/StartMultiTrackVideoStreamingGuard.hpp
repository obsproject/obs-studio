#pragma once

#include <future>

struct StartMultitrackVideoStreamingGuard {
	StartMultitrackVideoStreamingGuard() { future = guard.get_future().share(); };
	~StartMultitrackVideoStreamingGuard() { guard.set_value(); }

	std::shared_future<void> GetFuture() const { return future; }

	static std::shared_future<void> MakeReadyFuture()
	{
		StartMultitrackVideoStreamingGuard guard;
		return guard.GetFuture();
	}

private:
	std::promise<void> guard;
	std::shared_future<void> future;
};
