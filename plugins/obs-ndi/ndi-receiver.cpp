#include <obs-module.h>
#include <chrono>
#include <unistd.h>
#include <set>
#include <memory>
#include <boost/scope_exit.hpp>
#include "ndi-receiver.hpp"
#include "ndi.hpp"

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#endif
#endif

std::mutex globalReceiverMutex;
std::unique_ptr<NDIReceiver> globalReceiver;

NDIReceiver& GetGlobalReceiver() {
	std::unique_lock<std::mutex> lg(globalReceiverMutex);
	if(!globalReceiver) {
		globalReceiver = std::unique_ptr<NDIReceiver>(
				new NDIReceiver());
	}
	return *globalReceiver;
}

void GlobalReceiverDestroy() {
	globalReceiver = nullptr;
}

NDIReceiver::NDIReceiver() : nextframetag(0)
{
	// Spawn workers
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&this->frameworker, &attr, [](void *arg) -> void* {
		reinterpret_cast<NDIReceiver*>(arg)->WorkerFrames();
		return nullptr;
	}, this);
	pthread_create(&this->subworker, &attr, [](void *arg) -> void* {
		reinterpret_cast<NDIReceiver*>(arg)->WorkerSubscriptions();
		return nullptr;
	}, this);
}

NDIReceiver::~NDIReceiver() {
	pthread_cancel(this->frameworker);
	pthread_cancel(this->subworker);
	pthread_join(this->frameworker, nullptr);
	pthread_join(this->subworker, nullptr);
}

uint64_t NDIReceiver::Subscribe(const std::string& name,
		cb_t cb)
{
	std::unique_lock<std::mutex> lg(this->mut);

	auto frametag = this->nametotag[name];
	if(frametag == 0) {
		frametag = ++this->nextframetag;
		this->nametotag[name] = frametag;
	}

	auto& sub = this->subscriptions[frametag];
	auto subtag = ++sub.nextsubtag;
	if(sub.subscribers.empty()) {
		// New subscription
		sub.name = name;
		this->update_queue.emplace_back(name);
		this->cond_queue.notify_one();
	}
	sub.subscribers.emplace(subtag, cb);
	return subtag;
}

void NDIReceiver::Unsubscribe(const std::string& name, uint64_t tag) {
	std::unique_lock<std::mutex> lg(this->mut);

	auto ite = this->nametotag.find(name);
	if(ite == this->nametotag.end()) return;
	auto frametag = ite->second;
	auto& sub = this->subscriptions[frametag];
	sub.subscribers.erase(tag);
	if(sub.subscribers.empty()) {
		this->update_queue.emplace_back(name);
		this->cond_queue.notify_one();
	}
}

void NDIReceiver::ResetLink() {
	// Lock ordering important: WorkerFrames will take link_mut before
	// mut, so we risk deadlock if we do the opposite.
	std::unique_lock<std::mutex> lgl(this->link_mut);
	std::unique_lock<std::mutex> lgm(this->mut);
	this->link = frame_link(1);
}

video_format FramelinkToOBSVideoFormat(frame_link::frame_video::video_format format) {
	typedef frame_link::frame_video::video_format fl_format;
	switch(format) {
	case fl_format::VIDEO_FORMAT_UYVY: return VIDEO_FORMAT_UYVY;
	case fl_format::VIDEO_FORMAT_BGRA: return VIDEO_FORMAT_BGRA;
	default: return video_format::VIDEO_FORMAT_NONE;
	}
}

speaker_layout ChannelsToSpeakerLayout(int channels) {
	switch(channels) {
		case 1: return SPEAKERS_MONO;
		case 2: return SPEAKERS_STEREO;
		case 3: return SPEAKERS_2POINT1;
		case 4: return SPEAKERS_QUAD;
		case 5: return SPEAKERS_4POINT1;
		case 6: return SPEAKERS_5POINT1;
		case 8: return SPEAKERS_7POINT1;
		default: return SPEAKERS_UNKNOWN;
	}
}

void NDIReceiver::HandleVideoFrame(subscription& sub, frame_link::frame_t *frame) {
	obs_source_frame obsf = { 0 };
	auto payload = frame->payload_video();
	obsf.timestamp = frame->timestamp;
	obsf.width = payload->width;
	obsf.height = payload->height;
	obsf.format = FramelinkToOBSVideoFormat(payload->format);
	obsf.data[0] = payload->data();
	obsf.linesize[0] = payload->stride;
	for(auto& s : sub.subscribers) {
		s.second(&obsf, nullptr);
	}
}

void NDIReceiver::HandleAudioFrame(subscription& sub, frame_link::frame_t *frame) {
	obs_source_audio obsf = { 0 };
	auto payload = frame->payload_audio();
	obsf.timestamp = frame->timestamp;
	obsf.samples_per_sec = payload->samplerate;
	obsf.frames = payload->samples;
	obsf.format = AUDIO_FORMAT_FLOAT_PLANAR;
	obsf.speakers = ChannelsToSpeakerLayout(payload->channels);
	if(obsf.speakers == SPEAKERS_UNKNOWN) {
		blog(LOG_ERROR, "NDI: Unknown speaker layout for %d channels",
				payload->channels);
		return;
	}
	for(uint32_t i = 0; i < payload->channels; i++) {
		obsf.data[i] = reinterpret_cast<uint8_t*>(payload->data())
			+ i * payload->stride;
	}
	for(auto& s : sub.subscribers) {
		s.second(nullptr, &obsf);
	}
}

void NDIReceiver::HandleFrame(frame_link::frame_t *frame) {
	std::unique_lock<std::mutex> lgm(this->mut);
	auto ite = this->subscriptions.find(frame->tag);
	if(ite == this->subscriptions.end()) return;
	auto& sub = ite->second;

	// Frame for a valid subscription, so synthesize an obs frame from
	// the frame link frame.
	switch(frame->kind) {
		case frame_link::kind::FRAME_KIND_VIDEO:
			this->HandleVideoFrame(sub, frame);
			break;
		case frame_link::kind::FRAME_KIND_AUDIO:
			this->HandleAudioFrame(sub, frame);
			break;
		default:
			blog(LOG_INFO, "Unknown frame kind %d, ignoring", frame->kind);
			break;
	}
}

void NDIReceiver::WorkerFrames() {
	int dummy;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
	pthread_setname_np(pthread_self(), "NDIFrames");

	while(1) {
		std::unique_lock<std::mutex> lgl(this->link_mut);
		if(link.is_null()) {
			lgl.unlock();
			usleep(100000);
			continue;
		}
		auto frame = link.pop_timed(1, std::chrono::milliseconds(100));
		if(!frame) {
			continue;
		}
		this->HandleFrame(frame);
		link.push(0, frame);
	}
}

int FormatCmd(char* buf, size_t len, char command, uint64_t tag,
		const std::string& name)
{
	char* begin = buf;
	if(len < 5) return 0;
	*(buf++) = command;
	*reinterpret_cast<uint64_t*>(buf) = tag;
	buf += sizeof(uint64_t);
	if(command == 'S') {
		if(len < 5 + sizeof(uint32_t) + name.size() + 1) return 0;
		*reinterpret_cast<uint32_t*>(buf) = (uint32_t)name.size();
		buf += 4;
		std::memcpy(buf, name.c_str(), name.size());
		buf += name.size();
	}
	*buf = 0;
	return buf-begin;
}

void NDIReceiver::WorkerSubscriptionsCommandLoop(
		std::function<void(char *buf, size_t len)> writer) {
	std::set<uint64_t> currenttags;
	char cmdbuf[1024];

	// Send all current subscriptions first, then enter update loop
	{
		std::unique_lock<std::mutex> lg(this->mut);
		for(auto& s : this->subscriptions) {
			int n = FormatCmd(cmdbuf, sizeof(cmdbuf), 'S',
					s.first, s.second.name);
			writer(cmdbuf, n);
			currenttags.insert(s.first);
		}
	}

	while(1) {
		std::unique_lock<std::mutex> lg(this->mut);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		bool ok = this->cond_queue.wait_for(lg, std::chrono::milliseconds(100),
			[&]() { return !this->update_queue.empty(); });
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		if(!ok) continue;

		const auto& name = this->update_queue.front();
		auto ite = this->nametotag.find(name);
		if(ite != this->nametotag.end()) {
			auto frametag = ite->second;
			auto& sub = this->subscriptions[frametag];
			if(sub.subscribers.empty()) {
				if(currenttags.count(frametag) == 1) {
					int n = FormatCmd(cmdbuf, sizeof(cmdbuf),
							'U', frametag, "");
					writer(cmdbuf, n);
					currenttags.erase(frametag);
				}
				this->nametotag.erase(name);
				this->subscriptions.erase(frametag);
			} else {
				if(currenttags.count(frametag) == 0) {
					int n = FormatCmd(cmdbuf, sizeof(cmdbuf),
							'S', frametag, name);
					writer(cmdbuf, n);
					currenttags.insert(frametag);
				}
			}
		}
		this->update_queue.pop_front();
	}
}

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
void NDIReceiver::WorkerSubscriptionsRound() {
	int stdinpipe[2];
	if(pipe(stdinpipe) < 0) {
		throw "Could not allocate pipe for ndibridge connection";
	}
	std::string link_name;
	this->ResetLink();
	{
		std::unique_lock<std::mutex> lgl(this->link_mut);
		link_name = this->link.get_name();
	}
	blog(LOG_ERROR, "Link name: %s", link_name.c_str());

	auto child = fork();
	if(child == 0) {
		// Child
		prctl(PR_SET_PDEATHSIG, SIGKILL);
		if(dup2(stdinpipe[0], STDIN_FILENO) == -1) {
			perror("dup2");
			exit(1);
		}
		/* close(STDOUT_FILENO); */
		close(stdinpipe[1]);
		auto res = execl(ndibridge_path.c_str(), ndibridge_path.c_str(),
			"receive", link_name.c_str(), "1", nullptr);
		perror("execve");
		exit(res);
	} else if(child > 0) {
		// Parent
		blog(LOG_ERROR, "Spawned receiver ndibridge pid %d", child);
		close(stdinpipe[0]);

		pthread_cleanup_push(&cleanup_kill, new pid_t(child));
		// Send subscription updates to the NDI receiver.
		this->WorkerSubscriptionsCommandLoop([&](char *buf, int len) {
			if(write(stdinpipe[1], buf, len) != len) {
				throw "Could not send update to ndibridge";
			}
		});
		pthread_cleanup_pop(true);
	} else {
		throw "Could not fork for ndibridge";
	}
}
#endif
#endif

void NDIReceiver::WorkerSubscriptions() {
	int dummy;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
	pthread_setname_np(pthread_self(), "NDISub");

	// Continuously spawns an ndibridge and sends it receive subscriptions.
	while(1) {
		try {
			this->WorkerSubscriptionsRound();
		} catch(const char *error) {
			blog(LOG_ERROR, "NDI receiver crashed: %s\n", error);
		}
		sleep(5);
	}
}
