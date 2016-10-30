#include <obs-module.h>
#include <mutex>
#include <boost/scope_exit.hpp>
#include "ndi.hpp"
#include "framelink.hpp"

#ifdef QT_GUI_LIB
#include <obs-frontend-api.h>
#include <QInputDialog>
#endif

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#endif
#endif

int SpeakerLayoutToChannels(speaker_layout speakers) {
	switch(speakers) {
		case SPEAKERS_MONO: return 1;
		case SPEAKERS_STEREO: return 2;
		case SPEAKERS_2POINT1: return 3;
		case SPEAKERS_QUAD: return 4;
		case SPEAKERS_4POINT1: return 5;
		case SPEAKERS_5POINT1: return 6;
		case SPEAKERS_7POINT1: return 8;
		default: return 0;
	}
}

struct NDIOutput {
	std::mutex mut;
	obs_output_t *output;
	std::string name;

	pthread_t worker;
	bool started { false };
	frame_link link;

	NDIOutput() {
	}

	~NDIOutput() {
		KillWorker();
	}

	void SetFormat() {
		obs_get_video_info(&this->vinfo);
		obs_get_audio_info(&this->ainfo);

		vc.format = VIDEO_FORMAT_BGRA;
		vc.width = vinfo.output_width;
		vc.height = vinfo.output_height;
		vc.colorspace = VIDEO_CS_DEFAULT;
		vc.range = VIDEO_RANGE_DEFAULT;
		obs_output_set_video_conversion(this->output, &vc);

		ac.samples_per_sec = ainfo.samples_per_sec;
		ac.format = AUDIO_FORMAT_FLOAT_PLANAR;
		ac.speakers = ainfo.speakers;
		obs_output_set_audio_conversion(this->output, &ac);
	}

	void SetName(const std::string& newname) {
		if(this->name == newname) return;
		this->name = newname;
		if(this->started) this->CreateWorker();
	}

	void KillWorker() {
		if(started) {
			pthread_cancel(this->worker);
			pthread_join(this->worker, nullptr);
			started = false;
			obs_output_end_data_capture(this->output);
		}
	}

	void CreateWorker() {
		this->KillWorker();
		if(this->name.empty()) return;
		this->SetFormat();
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&this->worker, &attr, [](void *arg) -> void* {
			reinterpret_cast<NDIOutput*>(arg)->SenderWorker();
			return nullptr;
		}, this);
		started = true;
		obs_output_begin_data_capture(this->output, 0);
	}

	void RawVideo(struct video_data *data) {
		if(this->link.is_null()) return;
		auto frame = link.pop_timed(0, std::chrono::milliseconds(100));
		if(!frame) return;

		frame->kind = frame_link::kind::FRAME_KIND_VIDEO;
		frame->timestamp = data->timestamp;
		auto payload = frame->payload_video();
		payload->width = vinfo.output_width;
		payload->height = vinfo.output_height;
		payload->format =
			frame_link::frame_video::video_format::VIDEO_FORMAT_BGRA;
		payload->fps_num = vinfo.fps_num;
		payload->fps_denom = vinfo.fps_den;
		payload->aspect_ratio = (float)vinfo.output_width
					/ (float)vinfo.output_height;
		payload->stride = payload->width * 4;

		if(payload->size() < frame->max_payload_length()) {
			std::memcpy(payload->data(), data->data[0], payload->data_size());
			frame->payload_length = payload->size();
			link.push(1, frame);
		} else {
			link.push(0, frame);
		}
	}

	void RawAudio(struct audio_data *data) {
		if(this->link.is_null()) return;
		auto frame = link.pop_timed(0, std::chrono::milliseconds(100));
		if(!frame) return;

		frame->kind = frame_link::kind::FRAME_KIND_AUDIO;
		frame->timestamp = data->timestamp;
		auto payload = frame->payload_audio();
		payload->channels = SpeakerLayoutToChannels(ac.speakers);
		payload->samples = data->frames;
		payload->samplerate = ac.samples_per_sec;
		payload->stride = data->frames * sizeof(float);
		if(payload->size() < frame->max_payload_length()) {
			std::memcpy(payload->data(), data->data, payload->data_size());
			frame->payload_length = payload->size();
			link.push(1, frame);
		} else {
			link.push(0, frame);
		}
	}

private:

	// Frame format data
	struct obs_video_info vinfo;
	struct obs_audio_info ainfo;
	struct audio_convert_info ac;
	struct video_scale_info vc;

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
	void SpawnWorker() {
		std::string name;
		{
			std::unique_lock<std::mutex> lg(this->mut);
			name = this->name;
		}

		auto child = fork();
		if(child == 0) {
			// Child
			prctl(PR_SET_PDEATHSIG, SIGKILL);
			auto res = execl(ndibridge_path.c_str(),
				ndibridge_path.c_str(),
				"transmit", this->link.get_name().c_str(),
				"1", name.c_str(), nullptr);
			perror("execve");
			exit(res);
		} else if(child > 0) {
			// Parent
			blog(LOG_ERROR, "Spawned transmitter ndibridge");

			pthread_cleanup_push(&cleanup_kill, new pid_t(child));
			waitpid(child, nullptr, 0);
			pthread_cleanup_pop(true);
		} else {
			throw "Could not fork for ndibridge";
		}
	}
#endif
#endif

	void SenderWorker() {
		int dummy;
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
		while(1) {
			{
				std::unique_lock<std::mutex> lg(this->mut);
				link = frame_link(1);
			}
			this->SpawnWorker();
			sleep(5);
		}
	}
};

static const char *NDIOutputGetName(void *data) {
	UNUSED_PARAMETER(data);
	return obs_module_text("NDIOutput");
}

static void NDIOutputUpdate(void *data, obs_data_t *settings);
static void *NDIOutputCreate(obs_data_t *settings, obs_output_t *output) {
	NDIOutput *o = new NDIOutput();
	o->output = output;
	NDIOutputUpdate(o, settings);
	return o;
}

static void NDIOutputDestroy(void *data) {
	NDIOutput *o = reinterpret_cast<NDIOutput*>(data);
	delete o;
}

static bool NDIOutputStart(void *data) {
	NDIOutput *o = reinterpret_cast<NDIOutput*>(data);
	std::unique_lock<std::mutex> lg(o->mut);
	if(o->name.empty()) return false;
	o->CreateWorker();
	return true;
}

static void NDIOutputStop(void *data, uint64_t ts)
{
	UNUSED_PARAMETER(ts);
	NDIOutput *o = reinterpret_cast<NDIOutput*>(data);
	std::unique_lock<std::mutex> lg(o->mut);
	o->KillWorker();
}

static void NDIOutputRawVideo(void *data, struct video_data *frame) {
	NDIOutput *o = reinterpret_cast<NDIOutput*>(data);
	std::unique_lock<std::mutex> lg(o->mut);
	o->RawVideo(frame);
}

static void NDIOutputRawAudio(void *data, struct audio_data *frame) {
	NDIOutput *o = reinterpret_cast<NDIOutput*>(data);
	std::unique_lock<std::mutex> lg(o->mut);
	o->RawAudio(frame);
}

static void NDIOutputUpdate(void *data, obs_data_t *settings) {
	NDIOutput *o = reinterpret_cast<NDIOutput*>(data);
	std::unique_lock<std::mutex> lg(o->mut);
	const char *name = obs_data_get_string(settings, "ndi_name");
	o->SetName(name);
}

static void NDIOutputDefaults(obs_data_t *defaults) {
	obs_data_set_string(defaults, "ndi_name", "OBS");
}

static obs_properties_t *NDIOutputProperties(void *data) {
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();

	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	obs_properties_add_text(props, "ndi_name",
			obs_module_text("NDIOutputName"), OBS_TEXT_DEFAULT);

	return props;
}

#ifdef QT_GUI_LIB

static obs_output_t *globalOutput = nullptr;
std::string globalOutputName = "OBS";

void GUISourceStart(const std::string& name) {
	if(name.empty()) return;
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "ndi_name", name.c_str());
	if(globalOutput == nullptr) {
		globalOutput = obs_output_create("ndi_output",
				"gui_ndi_output", settings, nullptr);
		obs_output_start(globalOutput);
	} else {
		obs_output_update(globalOutput, settings);
	}
	globalOutputName = name;
	obs_data_release(settings);
}

void GUISourceStop() {
	if(globalOutput == nullptr) return;
	obs_output_stop(globalOutput);
	obs_output_release(globalOutput);
	globalOutput = nullptr;
	globalOutputName = "OBS";
}

void GUIRegister() {
	obs_frontend_add_tools_menu_item(obs_module_text("NDIGUIMenu"), [](void *data) {
		UNUSED_PARAMETER(data);
		QInputDialog *dia = new QInputDialog();
		bool ok;
		auto text = dia->getText(0, obs_module_text("NDIGUISinkName"),
				obs_module_text("NDIGUIOutputDesc"),
				QLineEdit::Normal, globalOutputName.c_str(), &ok);
		if(!ok) return;
		if(!text.isEmpty()) {
			GUISourceStart(text.toStdString());
		} else {
			GUISourceStop();
		}
	}, nullptr);

	obs_frontend_add_event_callback([](enum obs_frontend_event event, void *data) {
		UNUSED_PARAMETER(data);
		if(event == OBS_FRONTEND_EVENT_EXIT) {
			GUISourceStop();
		}
	}, nullptr);
}
#endif

void NDIOutputRegister() {
	struct obs_output_info info = {};
	info.id                 = "ndi_output";
	info.flags              = OBS_OUTPUT_AV;
	info.get_name           = NDIOutputGetName;
	info.create             = NDIOutputCreate;
	info.destroy            = NDIOutputDestroy;
	info.start              = NDIOutputStart;
	info.stop               = NDIOutputStop;
	info.raw_video          = NDIOutputRawVideo;
	info.raw_audio          = NDIOutputRawAudio;

	info.update             = NDIOutputUpdate;
	info.get_defaults       = NDIOutputDefaults;
	info.get_properties     = NDIOutputProperties;

	obs_register_output(&info);
	blog(LOG_ERROR, "Register output");

#ifdef QT_GUI_LIB
	GUIRegister();
#endif
};
