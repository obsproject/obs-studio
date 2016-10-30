#pragma once

#include <functional>
#include <obs-module.h>
#include "framelink.hpp"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <pthread.h>

struct NDIReceiver {
	NDIReceiver();
	~NDIReceiver();

	typedef std::function<void(obs_source_frame*, obs_source_audio*)> cb_t;

	uint64_t Subscribe(const std::string& name,
			cb_t cb);
	void Unsubscribe(const std::string& name, uint64_t tag);

private:
	struct subscription {
		std::string name;
		uint64_t nextsubtag { 0 }; // Next subscriber tag
		std::map<uint64_t, cb_t> subscribers;
	};

	void ResetLink();
	void WorkerSubscriptions();
	void WorkerSubscriptionsRound();
	void WorkerSubscriptionsCommandLoop(
			std::function<void(char *buf, size_t len)> writer);
	void WorkerFrames();
	void HandleFrame(frame_link::frame_t* frame);
	void HandleVideoFrame(subscription& sub, frame_link::frame_t *frame);
	void HandleAudioFrame(subscription& sub, frame_link::frame_t *frame);

	std::mutex link_mut;
	frame_link link;
	pthread_t subworker;
	pthread_t frameworker;

	std::mutex mut;
	std::condition_variable cond_queue;
	std::deque<std::string> update_queue;
	uint64_t nextframetag;
	std::map<std::string, uint64_t> nametotag;
	std::map<uint64_t, subscription> subscriptions;
};

extern NDIReceiver& GetGlobalReceiver();
extern void GlobalReceiverDestroy();
