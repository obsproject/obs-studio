#include <obs-module.h>
#include <string>
#include <mutex>
#include "ndi-finder.hpp"
#include "ndi-receiver.hpp"
#include "framelink.hpp"

struct NDISource {
	obs_source_t *source;
	std::mutex mut;

	std::string name;
	bool subscribed;
	uint64_t tag;

	NDISource() : subscribed(false) { }
	~NDISource() {
		this->Unsubscribe();
	}

	void SetName(const std::string& newname, bool force = false) {
		if(this->name == newname && !force) return;
		if(this->subscribed) this->Unsubscribe();
		this->name = newname;
		this->Subscribe();
	}

	void Subscribe() {
		using namespace std::placeholders;
		if(this->subscribed) return;
		if(this->name.empty()) return;
		blog(LOG_INFO, "NDI Subscribing to %s", this->name.c_str());
		this->tag = GetGlobalReceiver().Subscribe(this->name,
				std::bind(&NDISource::FrameCB, this, _1, _2));
		this->subscribed = true;
	}


	void Unsubscribe() {
		if(!this->subscribed) return;
		if(this->name.empty()) return;
		blog(LOG_INFO, "NDI Unsubscribing from %s",
				this->name.c_str());
		GetGlobalReceiver().Unsubscribe(this->name, this->tag);
		this->subscribed = false;
	}

private:
	void FrameCB(obs_source_frame* video, obs_source_audio* audio) {
		std::unique_lock<std::mutex> lg(this->mut);
		if(video) {
			obs_source_output_video(this->source, video);
		}
		if(audio) {
			obs_source_output_audio(this->source, audio);
		}
	}
};

static const char *NDISourceGetName(void *data) {
	UNUSED_PARAMETER(data);
	return obs_module_text("NDISource");
}

static void NDISourceUpdate(void *data, obs_data_t *settings);
static void *NDISourceCreate(obs_data_t *settings, obs_source_t *source)
{
	NDISource *s = new NDISource();
	s->source = source;
	NDISourceUpdate(s, settings);
	return s;
}

static void NDISourceDestroy(void *data)
{
	NDISource *s = reinterpret_cast<NDISource*>(data);
	delete s;
}

static void NDISourceGetDefaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	// Do nothing, there are no meaningful defaults
}

static void NDIFillKnownSources(obs_property *prop) {
	obs_property_list_clear(prop);
	auto sources = GetGlobalFinder().GetKnownSources();
	for(auto& s : sources) {
		obs_property_list_add_string(prop, s.c_str(), s.c_str());
	}
}

static bool NDIWebsiteClicked(obs_properties_t *props, obs_property_t *prop,
		void *data) {
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(prop);
	UNUSED_PARAMETER(data);
	return false;
}

static bool NDIRefreshClicked(obs_properties_t *props, obs_property_t *prop,
		void *data) {
	UNUSED_PARAMETER(prop);
	UNUSED_PARAMETER(data);
	auto list = obs_properties_get(props, "ndi_source");
	NDIFillKnownSources(list);
	return true;
}

static bool NDIApplyClicked(obs_properties_t *props, obs_property_t *prop,
		void *data) {
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(prop);
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
	auto settings = obs_source_get_settings(s->source);
	s->SetName(obs_data_get_string(settings, "ndi_source"));
	return true;
}

static obs_properties_t *NDISourceGetProperties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();

	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	obs_property *prop;

	prop = obs_properties_add_list(props, "ndi_source",
		obs_module_text("NDISources"), OBS_COMBO_TYPE_EDITABLE,
		OBS_COMBO_FORMAT_STRING);
	NDIFillKnownSources(prop);

	prop = obs_properties_add_button(props, "ndi_website",
			"NDI.NewTek.com",&NDIWebsiteClicked);
	prop = obs_properties_add_button(props, "refresh_sources",
			obs_module_text("Refresh sources"), &NDIRefreshClicked);
	prop = obs_properties_add_button(props, "apply_source",
			obs_module_text("Apply"), &NDIApplyClicked);

	return props;
}

static void NDISourceUpdate(void *data, obs_data_t *settings)
{
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
	const char *source = obs_data_get_string(settings, "ndi_source");
	s->SetName(source);
}

static void NDISourceActivate(void *data)
{
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
	s->Subscribe();
}

static void NDISourceDeactivate(void *data)
{
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
}

static void NDISourceShow(void *data)
{
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
	s->Subscribe();
}

static void NDISourceHide(void *data)
{
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
}

static void NDISourceVideoTick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	NDISource *s = reinterpret_cast<NDISource*>(data);
	std::unique_lock<std::mutex> lg(s->mut);
}

void NDISourceRegister()
{
	struct obs_source_info info = {};
	info.id = "ndi_source";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_ASYNC_VIDEO |
		OBS_SOURCE_AUDIO;
	info.get_name = NDISourceGetName;
	info.create = NDISourceCreate;
	info.destroy = NDISourceDestroy;
	info.get_defaults = NDISourceGetDefaults;
	info.get_properties = NDISourceGetProperties;
	info.update = NDISourceUpdate;
	info.activate = NDISourceActivate;
	info.deactivate = NDISourceDeactivate;
	info.show = NDISourceShow;
	info.hide = NDISourceHide;
	info.video_tick = NDISourceVideoTick;

	obs_register_source(&info);
}
