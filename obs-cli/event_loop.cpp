#include"event_loop.hpp"

#include<iostream>
#include<thread>

EventLoop::EventLoop() {

}
namespace {
	void background_io(EventLoop* el, bool* event_loop_is_valid) {
		while (true) {
			// wait for user orders to stop recording.
			std::cout << "press q<enter> to stop recording." << std::endl;
			int c = std::cin.get();
			if (c == static_cast<int>('q')) {
				if (*event_loop_is_valid) {
					el->stop();
				}
				return;
			}
		}
	}
}

void EventLoop::run() {
	// stuff will leak but I cannot clean it up correctly.
	bool* event_loop_is_valid = new bool(true);
	// launch background thread, will also leak.
	new std::thread(&background_io, this, event_loop_is_valid);

	// wait for stop condition
	std::unique_lock<std::mutex> lock(stop_mutex);
	stop_condition.wait(lock);

	// signal that `this` can no longer be used.
	*event_loop_is_valid = false;
}

void EventLoop::stop() {
	stop_condition.notify_all();
}