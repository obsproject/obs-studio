#include <obs-module.h>
#include <util/deque.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/darray.h>
#include <util/dstr.h>
#include <util/task.h>

#include <inttypes.h>

#define do_log(level, format, ...) \
	blog(level, "[slideshow: '%s'] " format, obs_source_get_name(ss->source), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)

/* clang-format off */

static const char *S_TR_SPEED                = "transition_speed";
static const char *S_CUSTOM_SIZE             = "use_custom_size";
static const char *S_SLIDE_TIME              = "slide_time";
static const char *S_TRANSITION              = "transition";
static const char *S_RANDOMIZE               = "randomize";
static const char *S_LOOP                    = "loop";
static const char *S_HIDE                    = "hide";
static const char *S_FILES                   = "files";
static const char *S_BEHAVIOR                = "playback_behavior";
static const char *S_BEHAVIOR_STOP_RESTART   = "stop_restart";
static const char *S_BEHAVIOR_PAUSE_UNPAUSE  = "pause_unpause";
static const char *S_BEHAVIOR_ALWAYS_PLAY    = "always_play";
static const char *S_MODE                    = "slide_mode";
static const char *S_MODE_AUTO               = "mode_auto";
static const char *S_MODE_MANUAL             = "mode_manual";
static const char *S_PLAYBACK_MODE           = "playback_mode";
static const char *S_PLAYBACK_ONCE           = "once";
static const char *S_PLAYBACK_LOOP           = "loop";
static const char *S_PLAYBACK_RANDOM         = "random";

static const char *TR_CUT                    = "cut";
static const char *TR_FADE                   = "fade";
static const char *TR_SWIPE                  = "swipe";
static const char *TR_SLIDE                  = "slide";

#define T_(text) obs_module_text("SlideShow." text)
#define T_TR_SPEED                           T_("TransitionSpeed")
#define T_CUSTOM_SIZE                        T_("CustomSize")
#define T_SLIDE_TIME                         T_("SlideTime")
#define T_TRANSITION                         T_("Transition")
#define T_HIDE                               T_("HideWhenDone")
#define T_FILES                              T_("Files")
#define T_BEHAVIOR                           T_("PlaybackBehavior")
#define T_BEHAVIOR_STOP_RESTART              T_("PlaybackBehavior.StopRestart")
#define T_BEHAVIOR_PAUSE_UNPAUSE             T_("PlaybackBehavior.PauseUnpause")
#define T_BEHAVIOR_ALWAYS_PLAY               T_("PlaybackBehavior.AlwaysPlay")
#define T_MODE                               T_("SlideMode")
#define T_MODE_AUTO                          T_("SlideMode.Auto")
#define T_MODE_MANUAL                        T_("SlideMode.Manual")
#define T_PLAYBACK_MODE                      T_("PlaybackMode")
#define T_PLAYBACK_ONCE                      T_("PlaybackMode.Once")
#define T_PLAYBACK_LOOP                      T_("PlaybackMode.Loop")
#define T_PLAYBACK_RANDOM                    T_("PlaybackMode.Random")

#define T_TR_(text) obs_module_text("SlideShow.Transition." text)
#define T_TR_CUT                             T_TR_("Cut")
#define T_TR_FADE                            T_TR_("Fade")
#define T_TR_SWIPE                           T_TR_("Swipe")
#define T_TR_SLIDE                           T_TR_("Slide")

/* clang-format on */

extern void image_source_preload_image(void *data);

/* ------------------------------------------------------------------------- */

struct image_file_data {
	char *path;
};

struct source_data {
	size_t slide_idx;
	const char *path;
	obs_source_t *source;
};

typedef DARRAY(struct image_file_data) image_file_array_t;

enum behavior {
	BEHAVIOR_STOP_RESTART,
	BEHAVIOR_PAUSE_UNPAUSE,
	BEHAVIOR_ALWAYS_PLAY,
};

#define SLIDE_BUFFER_COUNT 5

struct active_slides {
	struct deque prev;
	struct deque next;
	struct source_data cur;
	float elapsed;
	bool pending_auto_change;
};

struct slideshow_data {
	image_file_array_t files;
	float slide_time;
	uint32_t tr_speed;
	const char *tr_name;
	bool manual;
	bool randomize;
	bool loop;
	bool hide;
	enum behavior behavior;
};

struct slideshow {
	obs_source_t *source;

	struct active_slides slides;
	struct slideshow_data data;
	os_task_queue_t *queue;
	obs_source_t *transition;
	calldata_t cd;
	uint32_t cx;
	uint32_t cy;
	enum obs_media_state state;
	bool paused;
	bool stop;

	obs_hotkey_id play_pause_hotkey;
	obs_hotkey_id restart_hotkey;
	obs_hotkey_id stop_hotkey;
	obs_hotkey_id next_hotkey;
	obs_hotkey_id prev_hotkey;
};

/* Sets new media state.
signal_change should be false for media callbacks that are managed by the caller to avoid double signaling. Otherwise true. */
static void set_media_state(void *data, enum obs_media_state state, bool signal_change)
{
	struct slideshow *ss = data;
	enum obs_media_state old_state = ss->state;

	ss->state = state;

	if (signal_change) {
		bool was_stopped = (old_state == OBS_MEDIA_STATE_ENDED || old_state == OBS_MEDIA_STATE_STOPPED);
		bool is_stopped = (state == OBS_MEDIA_STATE_ENDED || state == OBS_MEDIA_STATE_STOPPED);

		if (!was_stopped && is_stopped) {
			obs_source_media_ended(ss->source);
		} else if (was_stopped && !is_stopped) {
			obs_source_media_started(ss->source);
		}
	}
}

static enum obs_media_state ss_get_state(void *data)
{
	struct slideshow *ss = data;
	return ss->state;
}

bool is_last_slide(struct slideshow *ss)
{
	struct slideshow_data *ssd = &ss->data;

	return ssd->files.num == 0 || (!ssd->randomize && !ssd->loop && ss->slides.cur.slide_idx == ssd->files.num - 1);
}

static void update_media_state(struct slideshow *ss)
{
	enum obs_media_state state;

	if (is_last_slide(ss))
		state = OBS_MEDIA_STATE_ENDED;
	else if (ss->stop)
		state = OBS_MEDIA_STATE_STOPPED;
	else if (ss->paused)
		state = OBS_MEDIA_STATE_PAUSED;
	else
		state = OBS_MEDIA_STATE_PLAYING;

	set_media_state(ss, state, true);
}

static inline obs_source_t *get_transition(struct slideshow *ss)
{
	return obs_source_get_ref(ss->transition);
}

static inline void free_source_data(struct source_data *sd)
{
	obs_source_release(sd->source);
}

static void free_active_slides(struct active_slides *slides)
{
	while (slides->prev.size) {
		struct source_data file;
		deque_pop_front(&slides->prev, &file, sizeof(file));
		free_source_data(&file);
	}

	while (slides->next.size) {
		struct source_data file;
		deque_pop_front(&slides->next, &file, sizeof(file));
		free_source_data(&file);
	}

	free_source_data(&slides->cur);
	slides->cur.source = NULL;

	deque_free(&slides->prev);
	deque_free(&slides->next);
}

static inline void free_slideshow_data(struct slideshow_data *ssd)
{
	for (size_t i = 0; i < ssd->files.num; i++)
		bfree(ssd->files.array[i].path);
	da_free(ssd->files);
}

static size_t random_file(struct slideshow_data *ssd, size_t slide_idx)
{
	const size_t files = ssd->files.num;
	size_t next = slide_idx;

	if (ssd->files.num > 1) {
		while (next == slide_idx) {
			size_t threshold = (~files + 1) % files;
			size_t r;
			do {
				r = rand();
			} while (r < threshold);
			next = r % files;
		}
	}

	return next;
}

/* gets a new file idx based upon a prior index */
static inline size_t get_new_file(struct slideshow_data *ssd, size_t slide_idx, bool next)
{
	if (ssd->randomize) {
		return random_file(ssd, slide_idx);
	} else if (next) {
		return (++slide_idx >= ssd->files.num) ? 0 : slide_idx;
	} else {
		return (slide_idx == 0) ? ssd->files.num - 1 : slide_idx - 1;
	}
}

/* ------------------------------------------------------------------------- */

static const char *ss_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("SlideShow");
}

/* adds a new file to the files list. used only in update */
static inline void add_file(image_file_array_t *new_files, const char *path)
{
	struct image_file_data data;
	data.path = bstrdup(path);
	da_push_back(*new_files, &data);
}

extern bool valid_extension(const char *ext);

/* transition to new source. assumes cur has already been set to new file */
static void do_transition(void *data, bool to_null, bool use_cut)
{
	struct slideshow *ss = data;
	struct slideshow_data *ssd = &ss->data;
	obs_source_t *source = (to_null || !ssd->files.num) ? NULL : ss->slides.cur.source;

	if (use_cut) {
		obs_transition_set(ss->transition, source);

		// To keep the same transition interval after cutting
		if (source)
			ss->slides.elapsed = (float)ssd->tr_speed / 1000.0f;
	} else {
		obs_transition_start(ss->transition, OBS_TRANSITION_MODE_AUTO, ssd->tr_speed, source);
	}

	if (source) {
		calldata_set_int(&ss->cd, "index", ss->slides.cur.slide_idx);
		calldata_set_string(&ss->cd, "path", ss->slides.cur.path);

		signal_handler_t *sh = obs_source_get_signal_handler(ss->source);
		signal_handler_signal(sh, "slide_changed", &ss->cd);
	}
}

/* get a source via its slide idx in one of slideshow_data's deques. *
 * only used in get_new_source().                                        */
static inline struct source_data *deque_get_source(struct deque *buf, size_t slide_idx)
{
	struct source_data *cur;
	size_t count = buf->size / sizeof(*cur);

	for (size_t i = 0; i < count; i++) {
		cur = deque_data(buf, i * sizeof(struct source_data));
		if (cur->slide_idx == slide_idx)
			return cur;
	}

	return NULL;
}

static void decode_image(void *data)
{
	obs_weak_source_t *weak = data;

	obs_source_t *source = obs_weak_source_get_source(weak);
	if (source) {
		image_source_preload_image(obs_obj_get_data(source));
		obs_source_release(source);
	}

	obs_weak_source_release(weak);
}

/* creates source from a file path. only used in get_new_source(). */
static inline obs_source_t *create_source_from_file(struct slideshow *ss, const char *file, bool now)
{
	obs_data_t *settings = obs_data_create();
	obs_source_t *source;

	obs_data_set_string(settings, "file", file);
	obs_data_set_bool(settings, "unload", false);
	obs_data_set_bool(settings, "is_slide", !now);
	source = obs_source_create_private("image_source", NULL, settings);

	obs_data_release(settings);

	os_task_queue_queue_task(ss->queue, decode_image, obs_source_get_weak_source(source));

	return source;
}

/* searches the active slides for the same slide so we can reuse existing *
 * sources                                                                */
static struct source_data *find_existing_source(struct active_slides *as, size_t slide_idx)
{
	struct source_data *psd;

	if (as->cur.source && as->cur.slide_idx == slide_idx)
		return &as->cur;

	psd = deque_get_source(&as->prev, slide_idx);
	if (psd)
		return psd;

	psd = deque_get_source(&as->next, slide_idx);
	return psd;
}

/* get a new source_data structure and reuse existing sources if possible. *
 * use the 'new' parameter if you want to create a brand new list of       *
 * active sources while still reusing the old list as well.                */
static struct source_data get_new_source(struct slideshow *ss, struct active_slides *new, size_t slide_idx)
{
	struct slideshow_data *ssd = &ss->data;
	struct source_data *psd;
	struct source_data sd;

	/* ----------------------------------------------- */
	/* use existing source if we already have it       */

	psd = find_existing_source(&ss->slides, slide_idx);
	if (psd) {
		sd = *psd;
		sd.source = obs_source_get_ref(sd.source);
		if (sd.source) {
			return sd;
		}
	}

	if (new) {
		psd = find_existing_source(new, slide_idx);
		if (psd) {
			sd = *psd;
			sd.source = obs_source_get_ref(sd.source);
			if (sd.source) {
				return sd;
			}
		}
	}

	/* ----------------------------------------------- */
	/* and then create a new one if we don't           */

	sd.path = ssd->files.array[slide_idx].path;
	sd.slide_idx = slide_idx;
	sd.source = create_source_from_file(ss, sd.path, false);
	return sd;
}

bool slides_match_at_idx(struct slideshow_data *old_data, struct slideshow_data *new_data, size_t new_i, size_t old_i)
{
	image_file_array_t *files = &new_data->files;
	image_file_array_t *old_files = &old_data->files;

	if (old_i != new_i || strcmp(files->array[new_i].path, old_files->array[old_i].path) != 0)
		return false;

	size_t old_idx = old_i;
	size_t new_idx = new_i;
	size_t i;
	for (i = 0; i < SLIDE_BUFFER_COUNT; i++) {
		new_idx = get_new_file(new_data, new_idx, true);
		old_idx = get_new_file(old_data, old_idx, true);

		if (old_idx != new_idx || strcmp(files->array[new_idx].path, old_files->array[old_idx].path) != 0)
			return false;
	}
	old_idx = old_i;
	new_idx = new_i;
	for (i = 0; i < SLIDE_BUFFER_COUNT; i++) {
		new_idx = get_new_file(new_data, new_idx, false);
		old_idx = get_new_file(old_data, old_idx, false);

		if (old_idx != new_idx || strcmp(files->array[new_idx].path, old_files->array[old_idx].path) != 0)
			return false;
	}
	return true;
}

static bool slides_are_reusable(struct slideshow *ss, struct slideshow_data *new)
{
	if (!ss->slides.cur.source || ss->data.randomize || new->randomize)
		return false;

	size_t old_idx = ss->slides.cur.slide_idx;
	return (old_idx < new->files.num) && slides_match_at_idx(&ss->data, new, old_idx, old_idx);
}

static void restart_slides(struct slideshow *ss)
{
	struct slideshow_data *ssd = &ss->data;
	size_t start_idx = 0;

	// Re-use old index if available, random only after, so we can keep the same slide if possible
	if (ss->slides.cur.source && ss->slides.cur.slide_idx < ssd->files.num) {
		start_idx = ss->slides.cur.slide_idx;
	} else if (ssd->randomize && ssd->files.num > 0) {
		start_idx = (size_t)rand() % ssd->files.num;
	}

	struct active_slides new_slides = {0};

	if (ssd->files.num) {
		struct source_data sd;
		size_t idx;

		new_slides.cur = get_new_source(ss, &new_slides, start_idx);

		idx = start_idx;
		for (size_t i = 0; i < SLIDE_BUFFER_COUNT; i++) {
			idx = get_new_file(ssd, idx, true);
			sd = get_new_source(ss, &new_slides, idx);
			deque_push_back(&new_slides.next, &sd, sizeof(sd));
		}

		idx = start_idx;
		for (size_t i = 0; i < SLIDE_BUFFER_COUNT; i++) {
			idx = get_new_file(ssd, idx, false);
			sd = get_new_source(ss, &new_slides, idx);
			deque_push_front(&new_slides.prev, &sd, sizeof(sd));
		}
	}

	free_active_slides(&ss->slides);
	ss->slides = new_slides;
}

static void ss_update(void *data, obs_data_t *settings)
{
	struct slideshow *ss = data;
	struct slideshow_data new_data = {0};
	obs_data_array_t *array;
	obs_source_t *new_tr = NULL;
	obs_source_t *old_tr = NULL;
	uint32_t new_duration;
	uint32_t cx = 0;
	uint32_t cy = 0;
	size_t count;
	const char *behavior;
	const char *tr_name;
	const char *mode;

	/* ------------------------------------- */
	/* get settings data                     */

	behavior = obs_data_get_string(settings, S_BEHAVIOR);

	if (astrcmpi(behavior, S_BEHAVIOR_PAUSE_UNPAUSE) == 0)
		new_data.behavior = BEHAVIOR_PAUSE_UNPAUSE;
	else if (astrcmpi(behavior, S_BEHAVIOR_ALWAYS_PLAY) == 0)
		new_data.behavior = BEHAVIOR_ALWAYS_PLAY;
	else /* S_BEHAVIOR_STOP_RESTART */
		new_data.behavior = BEHAVIOR_STOP_RESTART;

	mode = obs_data_get_string(settings, S_MODE);

	new_data.manual = (astrcmpi(mode, S_MODE_MANUAL) == 0);

	// Only change current pause status if original settings were edited
	if (ss->data.manual != new_data.manual) {
		ss->paused = new_data.manual;
	}

	/* Migrate and old loop/random settings to playback mode. */
	if (!obs_data_has_user_value(settings, S_PLAYBACK_MODE)) {
		if (obs_data_has_user_value(settings, S_RANDOMIZE) && obs_data_get_bool(settings, S_RANDOMIZE)) {
			obs_data_set_string(settings, S_PLAYBACK_MODE, S_PLAYBACK_RANDOM);
		} else if (obs_data_has_user_value(settings, S_LOOP)) {
			bool loop = obs_data_get_bool(settings, S_LOOP);
			obs_data_set_string(settings, S_PLAYBACK_MODE, loop ? S_PLAYBACK_LOOP : S_PLAYBACK_ONCE);
		}
	}

	const char *playback_mode = obs_data_get_string(settings, S_PLAYBACK_MODE);
	new_data.randomize = strcmp(playback_mode, S_PLAYBACK_RANDOM) == 0;
	new_data.loop = strcmp(playback_mode, S_PLAYBACK_LOOP) == 0;

	new_data.hide = obs_data_get_bool(settings, S_HIDE);

	tr_name = obs_data_get_string(settings, S_TRANSITION);
	if (astrcmpi(tr_name, TR_CUT) == 0)
		tr_name = "cut_transition";
	else if (astrcmpi(tr_name, TR_SWIPE) == 0)
		tr_name = "swipe_transition";
	else if (astrcmpi(tr_name, TR_SLIDE) == 0)
		tr_name = "slide_transition";
	else
		tr_name = "fade_transition";

	new_data.tr_name = tr_name;

	if (!ss->data.tr_name || strcmp(tr_name, ss->data.tr_name) != 0)
		new_tr = obs_source_create_private(tr_name, NULL, NULL);

	new_duration = (uint32_t)obs_data_get_int(settings, S_SLIDE_TIME);
	new_data.tr_speed = (uint32_t)obs_data_get_int(settings, S_TR_SPEED);

	if (strcmp(tr_name, "cut_transition") != 0) {
		if (new_duration < 100)
			new_duration = 100;

		new_duration += new_data.tr_speed;
	} else {
		if (new_duration < 50)
			new_duration = 50;
		new_data.tr_speed = 0;
	}

	new_data.slide_time = (float)new_duration / 1000.0f;

	/* ------------------------------------- */
	/* create new list of sources            */

	array = obs_data_get_array(settings, S_FILES);
	count = obs_data_array_count(array);

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(array, i);
		const char *path = obs_data_get_string(item, "value");
		os_dir_t *dir = os_opendir(path);

		if (!path || !*path) {
			obs_data_release(item);
			continue;
		}

		if (dir) {
			struct dstr dir_path = {0};
			struct os_dirent *ent;

			for (;;) {
				const char *ext;

				ent = os_readdir(dir);
				if (!ent)
					break;
				if (ent->directory)
					continue;

				ext = os_get_path_extension(ent->d_name);
				if (!valid_extension(ext))
					continue;

				dstr_copy(&dir_path, path);
				dstr_cat_ch(&dir_path, '/');
				dstr_cat(&dir_path, ent->d_name);
				add_file(&new_data.files, dir_path.array);
			}

			dstr_free(&dir_path);
			os_closedir(dir);
		} else {
			add_file(&new_data.files, path);
		}

		obs_data_release(item);
	}
	obs_data_array_release(array);

	/* ------------------------------------- */
	/* update files				 */

	bool reuse_slides = slides_are_reusable(ss, &new_data);

	free_slideshow_data(&ss->data);
	ss->data = new_data;

	if (!reuse_slides && ss->state != OBS_MEDIA_STATE_STOPPED) {

		// restart_slides() uses old indexes to reuse data if able,
		// so free first because existing indexes point to different files now
		free_active_slides(&ss->slides);
		restart_slides(ss);
	}

	/* ------------------------------------- */
	/* update transition                     */

	if (new_tr) {
		old_tr = ss->transition;
		ss->transition = new_tr;

		if (old_tr)
			obs_source_release(old_tr);
	}

	const char *res_str = obs_data_get_string(settings, S_CUSTOM_SIZE);
	int ret = sscanf(res_str, "%" PRIu32 "x%" PRIu32, &cx, &cy);
	if (ret != 2) {
		cx = 0;
		cy = 0;
	}

	ss->cx = cx;
	ss->cy = cy;
	obs_transition_set_size(ss->transition, cx, cy);
	obs_transition_set_alignment(ss->transition, OBS_ALIGN_CENTER);
	obs_transition_set_scale_type(ss->transition, OBS_TRANSITION_SCALE_ASPECT);

	if (new_tr)
		obs_source_add_active_child(ss->source, new_tr);

	/* ------------------------------------- */
	/* restart transition                    */

	do_transition(ss, ss->state == OBS_MEDIA_STATE_STOPPED, true);

	// Don't change play state if update was called while not active (e.g. via api)
	if (obs_source_active(ss->source))
		update_media_state(ss);
}

static void ss_play_pause(void *data, bool pause)
{
	struct slideshow *ss = data;

	if (ss->stop) {
		obs_source_media_restart(ss->source);
		return;
	}

	ss->paused = pause;
	if (pause)
		set_media_state(ss, OBS_MEDIA_STATE_PAUSED, false);
	else
		set_media_state(ss, OBS_MEDIA_STATE_PLAYING, false);
}

static void ss_restart(void *data)
{
	struct slideshow *ss = data;

	restart_slides(ss);
	ss->stop = false;
	ss->paused = false;
	do_transition(ss, false, true);
	set_media_state(ss, OBS_MEDIA_STATE_PLAYING, false);
}

static void ss_stop(void *data)
{
	struct slideshow *ss = data;

	ss->stop = true;
	ss->paused = false;
	do_transition(ss, true, true);

	set_media_state(ss, OBS_MEDIA_STATE_STOPPED, false);

	// Free slide textures while stopped
	free_active_slides(&ss->slides);
}

static void ss_next_slide(void *data)
{
	struct slideshow *ss = data;
	struct slideshow_data *ssd = &ss->data;
	struct active_slides *slides = &ss->slides;
	struct source_data sd;

	if (ss->stop) {
		obs_source_media_restart(ss->source);
		return;
	}

	if (!ssd->files.num)
		return;

	// Automatic slide changes adjust for drift, but manual changes should reset the timer
	if (slides->pending_auto_change) {
		slides->pending_auto_change = false;
	} else {
		slides->elapsed = 0.0f;
	}

	struct source_data *last = deque_data(&slides->next, (SLIDE_BUFFER_COUNT - 1) * sizeof(sd));
	size_t slide_idx = get_new_file(ssd, last->slide_idx, true);

	sd = get_new_source(ss, NULL, slide_idx);
	deque_push_back(&slides->next, &sd, sizeof(sd));
	deque_push_back(&slides->prev, &slides->cur, sizeof(sd));
	deque_pop_front(&slides->next, &slides->cur, sizeof(sd));
	deque_pop_front(&slides->prev, &sd, sizeof(sd));
	free_source_data(&sd);

	do_transition(ss, false, false);

	// If we were at the end, update the media state now that we're moving
	if (ss->state == OBS_MEDIA_STATE_ENDED)
		update_media_state(ss);
}

static void ss_previous_slide(void *data)
{
	struct slideshow *ss = data;
	struct slideshow_data *ssd = &ss->data;
	struct active_slides *slides = &ss->slides;
	struct source_data sd;

	if (ss->stop) {
		obs_source_media_restart(ss->source);
		return;
	}

	if (!ssd->files.num)
		return;

	// Automatic slide changes adjust for drift, but manual changes should reset the timer
	if (slides->pending_auto_change) {
		slides->pending_auto_change = false;
	} else {
		slides->elapsed = 0.0f;
	}

	struct source_data *first = deque_data(&slides->prev, 0);
	size_t slide_idx = get_new_file(ssd, first->slide_idx, false);

	sd = get_new_source(ss, NULL, slide_idx);
	deque_push_front(&slides->prev, &sd, sizeof(sd));
	deque_push_front(&slides->next, &slides->cur, sizeof(sd));
	deque_pop_back(&slides->prev, &slides->cur, sizeof(sd));
	deque_pop_back(&slides->next, &sd, sizeof(sd));
	free_source_data(&sd);

	do_transition(ss, false, false);

	// If we were at the end, update the media state now that we're moving
	if (ss->state == OBS_MEDIA_STATE_ENDED)
		update_media_state(ss);
}

static void play_pause_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_showing(ss->source))
		obs_source_media_play_pause(ss->source, !ss->paused);
}

static void restart_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_showing(ss->source))
		obs_source_media_restart(ss->source);
}

static void stop_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_showing(ss->source))
		obs_source_media_stop(ss->source);
}

static void next_slide_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_showing(ss->source))
		obs_source_media_next(ss->source);
}

static void previous_slide_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_showing(ss->source))
		obs_source_media_previous(ss->source);
}

static void current_slide_proc(void *data, calldata_t *cd)
{
	struct slideshow *ss = data;
	calldata_set_int(cd, "current_index", ss->slides.cur.slide_idx);
}

static void total_slides_proc(void *data, calldata_t *cd)
{
	struct slideshow *ss = data;
	calldata_set_int(cd, "total_files", ss->stop ? 0 : ss->data.files.num);
}

static void ss_destroy(void *data)
{
	struct slideshow *ss = data;

	os_task_queue_destroy(ss->queue);
	obs_source_release(ss->transition);
	free_active_slides(&ss->slides);
	calldata_free(&ss->cd);
	free_slideshow_data(&ss->data);
	bfree(ss);
}

static void *ss_create(obs_data_t *settings, obs_source_t *source)
{
	struct slideshow *ss = bzalloc(sizeof(*ss));
	proc_handler_t *ph = obs_source_get_proc_handler(source);

	ss->source = source;

	ss->paused = false;
	ss->stop = false;
	ss->state = OBS_MEDIA_STATE_ENDED;

	ss->queue = os_task_queue_create();

	ss->play_pause_hotkey = obs_hotkey_register_source(
		source, "SlideShow.PlayPause", obs_module_text("SlideShow.PlayPause"), play_pause_hotkey, ss);

	ss->restart_hotkey = obs_hotkey_register_source(source, "SlideShow.Restart",
							obs_module_text("SlideShow.Restart"), restart_hotkey, ss);

	ss->stop_hotkey = obs_hotkey_register_source(source, "SlideShow.Stop", obs_module_text("SlideShow.Stop"),
						     stop_hotkey, ss);

	ss->next_hotkey = obs_hotkey_register_source(source, "SlideShow.NextSlide",
						     obs_module_text("SlideShow.NextSlide"), next_slide_hotkey, ss);

	ss->prev_hotkey = obs_hotkey_register_source(source, "SlideShow.PreviousSlide",
						     obs_module_text("SlideShow.PreviousSlide"), previous_slide_hotkey,
						     ss);

	proc_handler_add(ph, "void current_index(out int current_index)", current_slide_proc, ss);
	proc_handler_add(ph, "void total_files(out int total_files)", total_slides_proc, ss);

	signal_handler_t *sh = obs_source_get_signal_handler(ss->source);
	signal_handler_add(sh, "void slide_changed(int index, string path)");

	obs_source_update(source, NULL);

	UNUSED_PARAMETER(settings);
	return ss;
}

static void ss_video_render(void *data, gs_effect_t *effect)
{
	struct slideshow *ss = data;
	obs_source_t *transition = get_transition(ss);

	if (transition) {
		obs_source_video_render(transition);
		obs_source_release(transition);
	}

	UNUSED_PARAMETER(effect);
}

static void ss_video_tick(void *data, float seconds)
{
	struct slideshow *ss = data;
	struct slideshow_data *ssd = &ss->data;
	struct active_slides *slides = &ss->slides;

	if (ss->state != OBS_MEDIA_STATE_PLAYING)
		return;

	/* ----------------------------------------------------- */
	/* do transition when slide time reached                 */

	slides->elapsed += seconds;

	if (slides->elapsed > ssd->slide_time) {
		slides->elapsed -= ssd->slide_time;

		if (is_last_slide(ss)) {
			if (ssd->hide)
				do_transition(ss, true, false);

			set_media_state(ss, OBS_MEDIA_STATE_ENDED, true);
			return;
		}

		slides->pending_auto_change = true;
		obs_source_media_next(ss->source);
	}
}

static inline bool ss_audio_render_(obs_source_t *transition, uint64_t *ts_out,
				    struct obs_source_audio_mix *audio_output, uint32_t mixers, size_t channels,
				    size_t sample_rate)
{
	struct obs_source_audio_mix child_audio;
	uint64_t source_ts;

	if (obs_source_audio_pending(transition))
		return false;

	source_ts = obs_source_get_audio_timestamp(transition);
	if (!source_ts)
		return false;

	obs_source_get_audio_mix(transition, &child_audio);
	for (size_t mix = 0; mix < MAX_AUDIO_MIXES; mix++) {
		if ((mixers & (1 << mix)) == 0)
			continue;

		for (size_t ch = 0; ch < channels; ch++) {
			float *out = audio_output->output[mix].data[ch];
			float *in = child_audio.output[mix].data[ch];

			memcpy(out, in, AUDIO_OUTPUT_FRAMES * sizeof(float));
		}
	}

	*ts_out = source_ts;

	UNUSED_PARAMETER(sample_rate);
	return true;
}

static bool ss_audio_render(void *data, uint64_t *ts_out, struct obs_source_audio_mix *audio_output, uint32_t mixers,
			    size_t channels, size_t sample_rate)
{
	struct slideshow *ss = data;
	obs_source_t *transition = get_transition(ss);
	bool success;

	if (!transition)
		return false;

	success = ss_audio_render_(transition, ts_out, audio_output, mixers, channels, sample_rate);

	obs_source_release(transition);
	return success;
}

static void ss_enum_sources(void *data, obs_source_enum_proc_t cb, void *param)
{
	struct slideshow *ss = data;

	if (ss->transition)
		cb(ss->source, ss->transition, param);
}

static uint32_t ss_width(void *data)
{
	struct slideshow *ss = data;
	return ss->transition ? ss->cx : 0;
}

static uint32_t ss_height(void *data)
{
	struct slideshow *ss = data;
	return ss->transition ? ss->cy : 0;
}

static void ss_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, S_TRANSITION, TR_FADE);
	obs_data_set_default_int(settings, S_SLIDE_TIME, 2000); //8000);
	obs_data_set_default_int(settings, S_TR_SPEED, 700);
	obs_data_set_default_string(settings, S_CUSTOM_SIZE, "1920x1080");
	obs_data_set_default_string(settings, S_BEHAVIOR, S_BEHAVIOR_PAUSE_UNPAUSE);
	obs_data_set_default_string(settings, S_MODE, S_MODE_AUTO);
	obs_data_set_default_string(settings, S_PLAYBACK_MODE, S_PLAYBACK_LOOP);
}

static const char *file_filter = "Image files (*.bmp *.tga *.png *.jpeg *.jpg"
#ifdef _WIN32
				 " *.jxr"
#endif
				 " *.gif *.webp)";

static obs_properties_t *ss_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();
	struct slideshow *ss = data;
	struct obs_video_info ovi;
	struct dstr path = {0};
	obs_property_t *p;
	int cx;
	int cy;

	/* ----------------- */

	obs_get_video_info(&ovi);
	cx = (int)ovi.base_width;
	cy = (int)ovi.base_height;

	/* ----------------- */

	p = obs_properties_add_list(ppts, S_BEHAVIOR, T_BEHAVIOR, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_BEHAVIOR_ALWAYS_PLAY, S_BEHAVIOR_ALWAYS_PLAY);
	obs_property_list_add_string(p, T_BEHAVIOR_STOP_RESTART, S_BEHAVIOR_STOP_RESTART);
	obs_property_list_add_string(p, T_BEHAVIOR_PAUSE_UNPAUSE, S_BEHAVIOR_PAUSE_UNPAUSE);

	p = obs_properties_add_list(ppts, S_MODE, T_MODE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_MODE_AUTO, S_MODE_AUTO);
	obs_property_list_add_string(p, T_MODE_MANUAL, S_MODE_MANUAL);

	p = obs_properties_add_list(ppts, S_TRANSITION, T_TRANSITION, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_TR_CUT, TR_CUT);
	obs_property_list_add_string(p, T_TR_FADE, TR_FADE);
	obs_property_list_add_string(p, T_TR_SWIPE, TR_SWIPE);
	obs_property_list_add_string(p, T_TR_SLIDE, TR_SLIDE);

	p = obs_properties_add_int(ppts, S_SLIDE_TIME, T_SLIDE_TIME, 50, 3600000, 50);
	obs_property_int_set_suffix(p, " ms");

	p = obs_properties_add_int(ppts, S_TR_SPEED, T_TR_SPEED, 0, 3600000, 50);
	obs_property_int_set_suffix(p, " ms");

	p = obs_properties_add_list(ppts, S_PLAYBACK_MODE, T_PLAYBACK_MODE, OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_PLAYBACK_ONCE, S_PLAYBACK_ONCE);
	obs_property_list_add_string(p, T_PLAYBACK_LOOP, S_PLAYBACK_LOOP);
	obs_property_list_add_string(p, T_PLAYBACK_RANDOM, S_PLAYBACK_RANDOM);

	obs_properties_add_bool(ppts, S_HIDE, T_HIDE);

	p = obs_properties_add_list(ppts, S_CUSTOM_SIZE, T_CUSTOM_SIZE, OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);

	char str[32];
	snprintf(str, sizeof(str), "%dx%d", cx, cy);
	obs_property_list_add_string(p, str, str);

	if (ss) {
		if (ss->data.files.num) {
			struct image_file_data *last = da_end(ss->data.files);
			const char *slash;

			dstr_copy(&path, last->path);
			dstr_replace(&path, "\\", "/");
			slash = strrchr(path.array, '/');
			if (slash)
				dstr_resize(&path, slash - path.array + 1);
		}
	}

	obs_properties_add_editable_list(ppts, S_FILES, T_FILES, OBS_EDITABLE_LIST_TYPE_FILES, file_filter, path.array);
	dstr_free(&path);

	return ppts;
}

static void ss_activate(void *data)
{
	struct slideshow *ss = data;

	if (ss->data.behavior == BEHAVIOR_STOP_RESTART) {

		// Don't restart yet if the stop was manual
		if (!ss->stop) {
			restart_slides(ss);
			do_transition(ss, false, true);
		}
	}

	// Update state regardless of current behavior, because APIs may have updated behavior without changing state
	update_media_state(ss);
}

static void ss_deactivate(void *data)
{
	struct slideshow *ss = data;

	if (ss->data.behavior == BEHAVIOR_STOP_RESTART) {

		// Don't override ended state with stop
		if (ss->state != OBS_MEDIA_STATE_ENDED)
			set_media_state(ss, OBS_MEDIA_STATE_STOPPED, true);

		// Free slide textures when stopped. Transition still holds the current image for fast re-activate.
		free_active_slides(&ss->slides);

	} else if (ss->data.behavior == BEHAVIOR_PAUSE_UNPAUSE) {

		// Don't override stop or ended states with pause
		if (ss->state == OBS_MEDIA_STATE_PLAYING)
			set_media_state(ss, OBS_MEDIA_STATE_PAUSED, true);
	}
}

static void missing_file_callback(void *src, const char *new_path, void *data)
{
	struct slideshow *s = src;
	const char *orig_path = data;

	obs_source_t *source = s->source;
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_array_t *files = obs_data_get_array(settings, S_FILES);

	size_t l = obs_data_array_count(files);
	for (size_t i = 0; i < l; i++) {
		obs_data_t *file = obs_data_array_item(files, i);
		const char *path = obs_data_get_string(file, "value");

		if (strcmp(path, orig_path) == 0) {
			if (new_path && *new_path)
				obs_data_set_string(file, "value", new_path);
			else
				obs_data_array_erase(files, i);

			obs_data_release(file);
			break;
		}

		obs_data_release(file);
	}

	obs_source_update(source, settings);

	obs_data_array_release(files);
	obs_data_release(settings);
}

static obs_missing_files_t *ss_missingfiles(void *data)
{
	struct slideshow *s = data;
	obs_missing_files_t *missing_files = obs_missing_files_create();

	obs_source_t *source = s->source;
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_array_t *files = obs_data_get_array(settings, S_FILES);

	size_t l = obs_data_array_count(files);
	for (size_t i = 0; i < l; i++) {
		obs_data_t *item = obs_data_array_item(files, i);
		const char *path = obs_data_get_string(item, "value");

		if (*path != 0) {
			if (!os_file_exists(path)) {
				obs_missing_file_t *file = obs_missing_file_create(
					path, missing_file_callback, OBS_MISSING_FILE_SOURCE, source, (void *)path);

				obs_missing_files_add_file(missing_files, file);
			}
		}

		obs_data_release(item);
	}

	obs_data_array_release(files);
	obs_data_release(settings);

	return missing_files;
}

static enum gs_color_space ss_video_get_color_space(void *data, size_t count,
						    const enum gs_color_space *preferred_spaces)
{
	enum gs_color_space space = GS_CS_SRGB;

	struct slideshow *ss = data;
	obs_source_t *transition = get_transition(ss);
	if (transition) {
		space = obs_source_get_color_space(transition, count, preferred_spaces);
		obs_source_release(transition);
	}

	return space;
}

struct obs_source_info slideshow_info_mk2 = {
	.id = "slideshow",
	.version = 2,
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_COMPOSITE |
			OBS_SOURCE_CONTROLLABLE_MEDIA,
	.get_name = ss_getname,
	.create = ss_create,
	.destroy = ss_destroy,
	.update = ss_update,
	.activate = ss_activate,
	.deactivate = ss_deactivate,
	.video_render = ss_video_render,
	.video_tick = ss_video_tick,
	.audio_render = ss_audio_render,
	.enum_active_sources = ss_enum_sources,
	.get_width = ss_width,
	.get_height = ss_height,
	.get_defaults = ss_defaults,
	.get_properties = ss_properties,
	.missing_files = ss_missingfiles,
	.icon_type = OBS_ICON_TYPE_SLIDESHOW,
	.media_play_pause = ss_play_pause,
	.media_restart = ss_restart,
	.media_stop = ss_stop,
	.media_next = ss_next_slide,
	.media_previous = ss_previous_slide,
	.media_get_state = ss_get_state,
	.video_get_color_space = ss_video_get_color_space,
};
