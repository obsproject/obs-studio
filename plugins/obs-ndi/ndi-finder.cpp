#include <obs-module.h>
#include <unistd.h>
#include <mutex>
#include <boost/algorithm/string/predicate.hpp>
#include "ndi.hpp"
#include "ndi-finder.hpp"

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#endif
#endif

std::mutex globalFinderMutex;
std::unique_ptr<NDIFinder> globalFinder;

NDIFinder& GetGlobalFinder() {
	std::unique_lock<std::mutex> lg(globalFinderMutex);
	if(!globalFinder) {
		globalFinder = std::unique_ptr<NDIFinder>(new NDIFinder());
	}
	return *globalFinder;
}

void GlobalFinderDestroy() {
  globalFinder = nullptr;
}

NDIFinder::NDIFinder() {
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&this->worker, &attr, [](void *arg) -> void* {
		reinterpret_cast<NDIFinder*>(arg)->WorkerLoop();
		return nullptr;
	}, this);
}

NDIFinder::~NDIFinder() {
	pthread_cancel(this->worker);
	pthread_join(this->worker, nullptr);
}

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
void NDIFinder::WorkerRound() {
	int stdoutpipe[2];
	if(pipe(stdoutpipe) < 0) {
		throw "Could not allocate pipe for ndibridge connection";
	}
	auto child = fork();
	if(child == 0) {
		// Child
		prctl(PR_SET_PDEATHSIG, SIGKILL);
		if(dup2(stdoutpipe[1], STDOUT_FILENO) == -1) {
			perror("dup2");
			exit(1);
		}
		close(stdoutpipe[0]);
		auto res = execl(ndibridge_path.c_str(), ndibridge_path.c_str(), "find", nullptr);
		perror("execve");
		exit(res);
	} else if(child > 0) {
		// Parent
		blog(LOG_ERROR, "Spawned finder ndibridge");
		close(stdoutpipe[1]);

		pthread_cleanup_push(&cleanup_kill, new pid_t(child));

		std::string line;
		while(1) {
			char c;
			if(read(stdoutpipe[0], &c, 1) != 1) {
				throw "Error reading from ndibridge find";
			}
			if(c != '\n') {
				line += c;
			} else {
				this->HandleLine(line);
				line.clear();
			}
		}

		waitpid(child, nullptr, 0);
		pthread_cleanup_pop(true);
	} else {
		throw "Could not fork for ndibridge";
	}
}
#endif
#endif

void NDIFinder::WorkerLoop() {
	int dummy;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);

	// Continuously spawns an ndibridge and listens for all "ADD" lines.
	// Each found source is added to the known sources list
	while(1) {
		try {
			this->WorkerRound();
		} catch(const char *error) {
			blog(LOG_ERROR, "NDI finder crashed: %s\n", error);
		}
		sleep(5);
	}
}

void NDIFinder::HandleLine(const std::string& line) {
	if(boost::starts_with(line, "NEW ")) {
		this->AddSource(line.substr(4));
	}
}

void NDIFinder::AddSource(const std::string& source) {
	std::lock_guard<std::mutex> lg(this->mut);
	this->known_sources.insert(source);
}

std::set<std::string> NDIFinder::GetKnownSources() {
	std::lock_guard<std::mutex> lg(this->mut);
	return this->known_sources;
}
