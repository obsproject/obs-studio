#include <obs.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs-encoder.h> // for obs_encoder calls
#include <obs-av1.h>     // for metadata_obu
// CURL Shim Headers
#include "shim/curl/curl.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Implements core libobs functions needed by shared/media-playback

// This allows us to use the shared code without linking the massive libobs.so

extern "C" {

// -- OS Event Implementation --
// libobs/util/threading.h defines typedef struct os_event_data os_event_t;
struct os_event_data {
	std::mutex mutex;
	std::condition_variable cond;
	bool signaled;
	bool manual;
};

// libobs/util/threading.h defines typedef struct os_sem_data os_sem_t;
struct os_sem_data {
	std::mutex mutex;
	std::condition_variable cond;
	int count;
};

int os_event_init(os_event_t **event, enum os_event_type type)
{
	if (!event)
		return -1;
	auto *e = new os_event_data();
	e->signaled = false;
	e->manual = (type == OS_EVENT_TYPE_MANUAL);
	*event = e;
	return 0;
}

void os_event_destroy(os_event_t *event)
{
	delete event;
}

int os_event_wait(os_event_t *event)
{
	if (!event)
		return -1;
	std::unique_lock<std::mutex> lock(event->mutex);
	event->cond.wait(lock, [event] { return event->signaled; });
	if (!event->manual)
		event->signaled = false;
	return 0;
}

int os_event_timedwait(os_event_t *event, unsigned long milliseconds)
{
	if (!event)
		return -1;
	std::unique_lock<std::mutex> lock(event->mutex);
	bool res = event->cond.wait_for(lock, std::chrono::milliseconds(milliseconds),
					[event] { return event->signaled; });
	if (!res)
		return ETIMEDOUT;
	if (!event->manual)
		event->signaled = false;
	return 0;
}

int os_event_try(os_event_t *event)
{
	if (!event)
		return -1;
	std::unique_lock<std::mutex> lock(event->mutex);
	if (!event->signaled)
		return EAGAIN;
	if (!event->manual)
		event->signaled = false;
	return 0;
}

int os_event_signal(os_event_t *event)
{
	if (!event)
		return -1;
	std::lock_guard<std::mutex> lock(event->mutex);
	event->signaled = true;
	event->cond.notify_all();
	return 0;
}

void os_event_reset(os_event_t *event)
{
	if (!event)
		return;
	std::lock_guard<std::mutex> lock(event->mutex);
	event->signaled = false;
}

// -- Atomics are inline in threading-posix.h, no need to redefine --

// -- OS SEM --

// -- Memory --
void *bcalloc(size_t multiplier, size_t size)
{
	return calloc(multiplier, size);
}
void bfree(void *ptr)
{
	free(ptr);
}
void *bmalloc(size_t size)
{
	return malloc(size);
}
void *brealloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

// -- Threading --
// struct os_sem_data defined at top

int os_sem_init(os_sem_t **sem, int value)
{
	if (!sem)
		return -1;
	auto *s = new os_sem_data();
	s->count = value;
	*sem = s;
	return 0;
}

void os_sem_destroy(os_sem_t *sem)
{
	delete sem;
}

int os_sem_wait(os_sem_t *sem)
{
	if (!sem)
		return -1;
	std::unique_lock<std::mutex> lock(sem->mutex);
	sem->cond.wait(lock, [sem] { return sem->count > 0; });
	sem->count--;
	return 0;
}

int os_sem_post(os_sem_t *sem)
{
	if (!sem)
		return -1;
	std::lock_guard<std::mutex> lock(sem->mutex);
	sem->count++;
	sem->cond.notify_one();
	return 0;
}

// -- Properties Stubs --
struct obs_properties {
	int dummy;
};

obs_properties_t *obs_properties_create(void)
{
	return new obs_properties();
}

void obs_properties_destroy(obs_properties_t *props)
{
	delete props;
}
obs_data_t *obs_data_create()
{
	return (obs_data_t *)calloc(1, 64);
}
void obs_data_release(obs_data_t *data)
{
	free(data);
}
long long obs_data_get_int(obs_data_t *data, const char *name)
{
	return 0;
}
const char *obs_data_get_string(obs_data_t *data, const char *name)
{
	return "";
}
bool obs_data_get_bool(obs_data_t *data, const char *name)
{
	return false;
}
void obs_data_set_default_int(obs_data_t *data, const char *name, long long val) {}
void obs_data_set_default_bool(obs_data_t *data, const char *name, bool val) {}
void obs_data_set_default_string(obs_data_t *data, const char *name, const char *val) {}

// -- Properties (Advanced Stubs) --
struct obs_property {
	int dummy;
};

obs_property_t *obs_properties_add_list(obs_properties_t *props, const char *name, const char *description,
					enum obs_combo_type type, enum obs_combo_format format)
{
	return new obs_property();
}

obs_property_t *obs_properties_add_text(obs_properties_t *props, const char *name, const char *description,
					enum obs_text_type type)
{
	return new obs_property();
}

size_t obs_property_list_add_string(obs_property_t *p, const char *name, const char *val)
{
	return 0;
}
size_t obs_property_list_add_int(obs_property_t *p, const char *name, long long val)
{
	return 0;
}

void obs_property_set_modified_callback(obs_property_t *p, obs_property_modified_t modified) {}
void obs_property_set_visible(obs_property_t *p, bool visible) {}
bool obs_property_modified(obs_property_t *p, obs_data_t *settings)
{
	return true;
}

obs_property_t *obs_properties_get(obs_properties_t *props, const char *property)
{
	// Return valid ptr to mock property
	static obs_property_t mock_prop;
	return &mock_prop;
}

// -- Source Audio --
void obs_source_output_audio(obs_source_t *source, const struct obs_source_audio *audio)
{
	// TODO: Route audio to AudioManager
}

// -- Logging --
void blog(int log_level, const char *format, ...)
{
	// Forward to cout?
}

// -- OS --
uint64_t os_gettime_ns(void)
{
	auto now = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

void os_sleep_ms(uint32_t duration)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

void os_set_thread_name(const char *name)
{
	// Platform specific, safe to no-op for now
}

void *bmemdup(const void *ptr, size_t size)
{
	void *mem = malloc(size);
	if (mem && ptr)
		memcpy(mem, ptr, size);
	return mem;
}

// bmemdup is needed (not inline usually?)
// Error log said "undefined reference to bmemdup", so keep bmemdup, remove bstrdup.

// -- Video --
// From error log: bool video_format_get_parameters_for_format(enum video_colorspace color_space, enum video_range_type range, enum video_format format, float* matrix, float* range_min, float* range_max)
bool video_format_get_parameters_for_format(enum video_colorspace color_space, enum video_range_type range,
					    enum video_format format, float *matrix, float *range_min, float *range_max)
{

	// Return identity or defaults?
	// Use NULL check
	if (matrix) {
		// Identity or standard YUV?
		// For now, no-op or memset 0
	}
	return false; // Stub failure, fallback to sRGB or something?
}

// -- Encoder/Output Stubs --
int obs_output_get_total_frames(const obs_output_t *output)
{
	return 0;
}
int obs_output_get_frames_dropped(const obs_output_t *output)
{
	return 0;
}
uint32_t obs_get_total_frames(void)
{
	return 0;
}
uint32_t obs_get_lagged_frames(void)
{
	return 0;
}

video_t *obs_encoder_video(const obs_encoder_t *encoder)
{
	return nullptr;
}
uint32_t video_output_get_total_frames(const video_t *video)
{
	return 0;
}
uint32_t video_output_get_skipped_frames(const video_t *video)
{
	return 0;
}
uint32_t obs_encoder_get_encoded_frames(const obs_encoder_t *encoder)
{
	return 0;
}
const char *obs_encoder_get_name(const obs_encoder_t *encoder)
{
	return "MockEncoder";
}
const char *obs_encoder_get_codec(const obs_encoder_t *encoder)
{
	return "h264";
}
void obs_encoder_packet_release(struct encoder_packet *packet) {}

// -- OS Filesystem (POSIX/std::filesystem wrapper) --
int os_mkdir(const char *path)
{
	try {
		std::filesystem::create_directories(path);
		return 0;
	} catch (...) {
		return -1;
	}
}
int os_rmdir(const char *path)
{
	try {
		std::filesystem::remove_all(path);
		return 0;
	} catch (...) {
		return -1;
	}
}
int os_unlink(const char *path)
{
	try {
		std::filesystem::remove(path);
		return 0;
	} catch (...) {
		return -1;
	}
}
int os_rename(const char *src, const char *dst)
{
	try {
		std::filesystem::rename(src, dst);
		return 0;
	} catch (...) {
		return -1;
	}
}
int os_copyfile(const char *src, const char *dst)
{
	try {
		std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
		return 0;
	} catch (...) {
		return -1;
	}
}
bool os_quick_write_utf8_file(const char *path, const char *str, size_t len, bool marker)
{
	std::ofstream out(path);
	if (!out)
		return false;
	out.write(str, len);
	return true;
}

// -- OBS Data (JSON Mock) --
// We mock these enough to not crash, but they won't parse real JSON.
obs_data_t *obs_data_create_from_json(const char *json_string)
{
	return obs_data_create();
}
obs_data_t *obs_data_create_from_json_file(const char *json_file)
{
	return obs_data_create();
}
bool obs_data_save_json(obs_data_t *data, const char *file)
{
	return true;
}
void obs_data_set_string(obs_data_t *data, const char *name, const char *val) {}
void obs_data_addref(obs_data_t *data) {}

// -- Encoding / String Stubs (for dstr.c) --
size_t os_mbs_to_utf8_ptr(const char *str, size_t len, char **pstr)
{
	if (!str)
		return 0;
	if (len == 0)
		len = strlen(str);
	*pstr = strndup(str, len); // Assume UTF-8
	return len;
}
size_t os_utf8_to_wcs_ptr(const char *str, size_t len, wchar_t **pstr)
{
	if (!str)
		return 0;
	size_t req = mbstowcs(nullptr, str, 0);
	if (req == (size_t)-1)
		return 0;
	*pstr = (wchar_t *)malloc((req + 1) * sizeof(wchar_t));
	return mbstowcs(*pstr, str, req + 1);
}
size_t wchar_to_utf8(const wchar_t *str, size_t len, char *dst, size_t dst_size, int flags)
{
	if (!str)
		return 0;
	// Simple wrapper around wcstombs
	if (!dst)
		return wcstombs(nullptr, str, 0);
	return wcstombs(dst, str, dst_size);
}

// Array mocks
obs_data_array_t *obs_data_get_array(obs_data_t *data, const char *name)
{
	return (obs_data_array_t *)calloc(1, sizeof(void *));
}
size_t obs_data_array_count(obs_data_array_t *array)
{
	return 0;
}
obs_data_t *obs_data_array_item(obs_data_array_t *array, size_t idx)
{
	return nullptr;
}
void obs_data_array_release(obs_data_array_t *array)
{
	free(array);
}

// -- CURL Mocks --
CURL *curl_easy_init(void)
{
	return (CURL *)malloc(1);
}
CURLcode curl_easy_setopt(CURL *curl, int option, ...)
{
	return CURLE_OK;
}
CURLcode curl_easy_perform(CURL *curl)
{
	return CURLE_OK;
}
void curl_easy_cleanup(CURL *curl)
{
	free(curl);
}
CURLcode curl_easy_getinfo(CURL *curl, int info, ...)
{
	return CURLE_OK;
}
struct curl_slist *curl_slist_append(struct curl_slist *list, const char *s)
{
	return (struct curl_slist *)malloc(1);
}
void curl_slist_free_all(struct curl_slist *list)
{
	free(list);
} // Simplified, will leak if loop, but it's mock
void curl_obs_set_revoke_setting(CURL *curl) {}
const char *curl_easy_strerror(CURLcode code)
{
	return "Mock Error";
}

// -- OS --
struct timespec *os_nstime_to_timespec(uint64_t ns, struct timespec *ts)
{
	if (!ts)
		return nullptr;
	ts->tv_sec = ns / 1000000000;
	ts->tv_nsec = ns % 1000000000;
	return ts;
}

} // extern C
