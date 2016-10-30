#pragma once

#include <set>
#include <string>
#include <mutex>
#include <pthread.h>

// Launches the ndibridge and keeps adding seen sources to an internal list.
struct NDIFinder {
	NDIFinder();
	~NDIFinder();

	std::set<std::string> GetKnownSources();

private:
	void WorkerRound();
	void WorkerLoop();
	void AddSource(const std::string& source);
	void HandleLine(const std::string& line);

	std::mutex mut;
	pthread_t worker;
	std::set<std::string> known_sources;
};

NDIFinder& GetGlobalFinder();
extern void GlobalFinderDestroy();
