#include <obs-module.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/darray.h>
#include <util/dstr.h>

#define do_log(level, format, ...)               \
	blog(level, "[slideshow: '%s'] " format, \
	     obs_source_get_name(ss->source), ##__VA_ARGS__)

#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)

/* clang-format off */

#define S_TR_SPEED                     "transition_speed"
#define S_CUSTOM_SIZE                  "use_custom_size"
#define S_SLIDE_TIME                   "slide_time"
#define S_TRANSITION                   "transition"
#define S_RANDOMIZE                    "randomize"
#define S_LOOP                         "loop"
#define S_HIDE                         "hide"
#define S_FILES                        "files"
#define S_BEHAVIOR                     "playback_behavior"
#define S_BEHAVIOR_STOP_RESTART        "stop_restart"
#define S_BEHAVIOR_PAUSE_UNPAUSE       "pause_unpause"
#define S_BEHAVIOR_ALWAYS_PLAY         "always_play"
#define S_MODE                         "slide_mode"
#define S_MODE_AUTO                    "mode_auto"
#define S_MODE_MANUAL                  "mode_manual"

#define TR_CUT                         "cut"
#define TR_FADE                        "fade"
#define TR_SWIPE                       "swipe"
#define TR_SLIDE                       "slide"

#define T_(text) obs_module_text("SlideShow." text)
#define T_TR_SPEED                     T_("TransitionSpeed")
#define T_CUSTOM_SIZE                  T_("CustomSize")
#define T_CUSTOM_SIZE_AUTO             T_("CustomSize.Auto")
#define T_SLIDE_TIME                   T_("SlideTime")
#define T_TRANSITION                   T_("Transition")
#define T_RANDOMIZE                    T_("Randomize")
#define T_LOOP                         T_("Loop")
#define T_HIDE                         T_("HideWhenDone")
#define T_FILES                        T_("Files")
#define T_BEHAVIOR                     T_("PlaybackBehavior")
#define T_BEHAVIOR_STOP_RESTART        T_("PlaybackBehavior.StopRestart")
#define T_BEHAVIOR_PAUSE_UNPAUSE       T_("PlaybackBehavior.PauseUnpause")
#define T_BEHAVIOR_ALWAYS_PLAY         T_("PlaybackBehavior.AlwaysPlay")
#define T_MODE                         T_("SlideMode")
#define T_MODE_AUTO                    T_("SlideMode.Auto")
#define T_MODE_MANUAL                  T_("SlideMode.Manual")

#define T_TR_(text) obs_module_text("SlideShow.Transition." text)
#define T_TR_CUT                       T_TR_("Cut")
#define T_TR_FADE                      T_TR_("Fade")
#define T_TR_SWIPE                     T_TR_("Swipe")
#define T_TR_SLIDE                     T_TR_("Slide")

/* clang-format on */

/* ------------------------------------------------------------------------- */

/* How many items to cache in front of and behind the current item. Total
 * number of cached items is thus 1 + (2 * CACHE_IN_ADVANCE)
 */
#define CACHE_IN_ADVANCE 2

enum behavior {
	BEHAVIOR_STOP_RESTART,
	BEHAVIOR_PAUSE_UNPAUSE,
	BEHAVIOR_ALWAYS_PLAY,
};

struct cache_entry {
	/* Index in file_queue this cache entry belongs to. */
	size_t filequeue_index;
	/* Path of the loaded source to double-check cache consistency. */
	char *path;
	/* The loaded image source. */
	obs_source_t *source;
};

struct slideshow {
	obs_source_t *source;

	pthread_mutex_t mutex;
	volatile bool have_mutex;

	/* Variables that need mutex lock */

	bool randomize;
	bool loop;
	bool restart_on_activate;
	bool pause_on_deactivate;
	bool restart;
	bool manual;
	bool hide;
	bool use_cut;
	bool paused;
	bool stop;
	float slide_time;
	uint32_t tr_speed;
	const char *tr_name;
	obs_source_t *transition;

	float elapsed;

	uint32_t cx;
	uint32_t cy;

	DARRAY(char *) file_paths;
	DARRAY(char *) file_queue;
	size_t cur_item;

	DARRAY(struct cache_entry) cache_entries;
	DARRAY(obs_source_t *) source_cleanup;

	bool retry_transition;

	/* Variables that can do without mutex lock */

	os_event_t *cache_event;
	pthread_t cache_thread;
	bool cache_thread_created;

	enum behavior behavior;

	obs_hotkey_id play_pause_hotkey;
	obs_hotkey_id restart_hotkey;
	obs_hotkey_id stop_hotkey;
	obs_hotkey_id next_hotkey;
	obs_hotkey_id prev_hotkey;
};

static void lock_mutex(struct slideshow *ss)
{
	pthread_mutex_lock(&ss->mutex);
	assert(!ss->have_mutex);
	ss->have_mutex = true;
}

static void unlock_mutex(struct slideshow *ss)
{
	assert(ss->have_mutex);
	ss->have_mutex = false;
	pthread_mutex_unlock(&ss->mutex);
}

static obs_source_t *get_transition(struct slideshow *ss)
{
	obs_source_t *tr;

	lock_mutex(ss);
	tr = ss->transition;
	obs_source_addref(tr);
	unlock_mutex(ss);

	return tr;
}

static obs_source_t *create_source_from_file(const char *file)
{
	obs_data_t *settings = obs_data_create();
	obs_source_t *source;

	obs_data_set_string(settings, "file", file);
	obs_data_set_bool(settings, "unload", false);
	source = obs_source_create_private("image_source", NULL, settings);

	obs_data_release(settings);
	return source;
}

static void free_files(struct darray *array)
{
	DARRAY(char *) file_paths;
	file_paths.da = *array;

	for (size_t i = 0; i < file_paths.num; i++) {
		bfree(file_paths.array[i]);
	}

	da_free(file_paths);
}

/* ------------------------------------------------------------------------- */

static void free_cache_entry(struct slideshow *ss, struct cache_entry *entry)
{
	assert(ss->have_mutex);

	bfree(entry->path);
	da_push_back(ss->source_cleanup, &entry->source);
}

static void clear_cache(struct slideshow *ss)
{
	// Also happening during ss_destroy where we do not hold the lock.
	// assert(ss->have_mutex);

	for (size_t i = 0; i < ss->cache_entries.num; i++) {
		free_cache_entry(ss, &ss->cache_entries.array[i]);
	}
	da_erase_range(ss->cache_entries, 0, ss->cache_entries.num);
}

static struct cache_entry *get_cache_entry(struct slideshow *ss,
					   size_t filequeue_index)
{
	assert(ss->have_mutex);

	for (size_t i = 0; i < ss->cache_entries.num; i++) {
		struct cache_entry *entry = &ss->cache_entries.array[i];
		if (entry->filequeue_index != filequeue_index)
			continue;

		if (strcmp(entry->path,
			   ss->file_queue.array[filequeue_index]) != 0) {
			warn("Cache invalid, paths do not match.");
			clear_cache(ss);
			return NULL;
		}

		return entry;
	}

	return NULL;
}

static obs_source_t *obtain_cached_source(struct slideshow *ss,
					  size_t filequeue_index)
{
	assert(ss->have_mutex);

	struct cache_entry *entry = get_cache_entry(ss, filequeue_index);
	if (!entry)
		return NULL;

	obs_source_addref(entry->source);
	return entry->source;
}

static size_t min_size(size_t val1, size_t val2)
{
	if (val1 < val2)
		return val1;
	return val2;
}

static bool next_index_to_cache(struct slideshow *ss,
				size_t *out_filequeue_index)
{
	assert(ss->have_mutex);

	size_t num = ss->file_queue.num;
	if (num == 0)
		return false;

	size_t cur_item = ss->cur_item;
	size_t num_entries_after = min_size(CACHE_IN_ADVANCE, (num - 1));
	size_t num_entries_before =
		min_size(CACHE_IN_ADVANCE, (num - num_entries_after - 1));
	size_t index = cur_item;

	if (!get_cache_entry(ss, index)) {
		*out_filequeue_index = index;
		return true;
	}
	for (size_t i = 1; i <= num_entries_after; i++) {
		index = (cur_item + i) % num;
		if (!get_cache_entry(ss, index)) {
			*out_filequeue_index = index;
			return true;
		}
	}
	for (size_t i = 1; i <= num_entries_before; i++) {
		if (i > cur_item)
			index = num - (i - cur_item);
		else
			index = cur_item - i;

		if (!get_cache_entry(ss, index)) {
			*out_filequeue_index = index;
			return true;
		}
	}

	return false;
}

static bool should_cache_filequeue_index(size_t index, size_t min_valid,
					 size_t max_valid, size_t num)
{
	assert(min_valid < num && max_valid < num);

	if (max_valid >= min_valid) {
		/* No wrap-around of the valid range. Easy case. */
		return index >= min_valid && index <= max_valid;
	}

	return (index >= min_valid && index < num) ||
	       (index >= 0 && index <= max_valid);
}

static void evict_stale_cache_entries(struct slideshow *ss)
{
	assert(ss->have_mutex);

	size_t num = ss->file_queue.num;
	if (num == 0)
		return;

	size_t cur_item = ss->cur_item;
	size_t num_entries_after = min_size(CACHE_IN_ADVANCE, (num - 1));
	size_t num_entries_before =
		min_size(CACHE_IN_ADVANCE, (num - num_entries_after - 1));
	size_t first_index;
	if (num_entries_before > cur_item)
		first_index = num - (num_entries_before - cur_item);
	else
		first_index = cur_item - num_entries_before;
	size_t last_index = (cur_item + num_entries_after) % num;

	for (size_t i = 0; i < ss->cache_entries.num;) {
		struct cache_entry *entry = &ss->cache_entries.array[i];
		if (should_cache_filequeue_index(entry->filequeue_index,
						 first_index, last_index,
						 num)) {
			i++;
			continue;
		}

		debug("Evicting stale cache entry %zu (current: %zu)",
		      entry->filequeue_index, ss->cur_item);

		free_cache_entry(ss, entry);
		da_erase(ss->cache_entries, i);
		/* No increase of i since the array decreased. */
	}
}

static void load_and_cache(struct slideshow *ss, size_t filequeue_index,
			   char *path)
{
	assert(!ss->have_mutex);

	obs_source_t *source = create_source_from_file(path);
	if (source == NULL) {
		warn("Failed to load %s", path);
		bfree(path);
		/* How to procede? We're going to be stuck until cur_item
		 * changes as in the next cache_thread iteration we're going
		 * to try to load the same entry again. Need some way to signal
		 * this error condition so that at least the succeeding entries
		 * are going to get cached.
		 */
		return;
	}

	struct cache_entry entry = {.filequeue_index = filequeue_index,
				    .path = path,
				    .source = source};

	lock_mutex(ss);
	if (filequeue_index < ss->file_queue.num) {
		if (strcmp(path, ss->file_queue.array[filequeue_index]) == 0) {
			debug("Caching %s (%zu, current: %zu)", path,
			      filequeue_index, ss->cur_item);
			da_push_back(ss->cache_entries, &entry);
		} else {
			warn("Item %zu has changed from %s to %s, skipping cache load",
			     filequeue_index, path,
			     ss->file_queue.array[filequeue_index]);
			free_cache_entry(ss, &entry);
		}
	} else {
		warn("Filequeue shrunk while caching, skipping cache load of %s",
		     path);
		free_cache_entry(ss, &entry);
	}
	unlock_mutex(ss);
}

static void *cache_thread(void *opaque)
{
	struct slideshow *ss = opaque;

	os_set_thread_name("slideshow: cache worker thread");

	while (true) {
		os_event_wait(ss->cache_event);

		lock_mutex(ss);

		evict_stale_cache_entries(ss);

		DARRAY(obs_source_t *) cleanup = {0};
		da_move(cleanup, ss->source_cleanup);

		size_t filequeue_index = 0;
		char *path = NULL;
		if (next_index_to_cache(ss, &filequeue_index)) {
			path = bstrdup(ss->file_queue.array[filequeue_index]);
		}

		unlock_mutex(ss);

		/* Release the sources outside the lock to avoid a deadlock with
		 * the graphics thread.
		 */
		for (size_t i = 0; i < cleanup.num; i++) {
			obs_source_release(cleanup.array[i]);
		}
		da_free(cleanup);

		if (path == NULL) {
			os_event_reset(ss->cache_event);
			continue;
		}

		load_and_cache(ss, filequeue_index, path);
	}
}

/* ------------------------------------------------------------------------- */

static const char *ss_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("SlideShow");
}

static void add_file(struct slideshow *ss, struct darray *array,
		     const char *path, uint32_t *cx, uint32_t *cy)
{
	UNUSED_PARAMETER(ss);
	DARRAY(char *) new_files;

	new_files.da = *array;

	char *copiedPath = bstrdup(path);
	da_push_back(new_files, &copiedPath);

	*array = new_files.da;

	// Read the image dimensions
	obs_source_t *tmp_source = create_source_from_file(path);

	if (tmp_source) {
		uint32_t new_cx = obs_source_get_width(tmp_source);
		uint32_t new_cy = obs_source_get_height(tmp_source);

		if (new_cx > *cx)
			*cx = new_cx;
		if (new_cy > *cy)
			*cy = new_cy;

		obs_source_release(tmp_source);
	}
}

static void shuffle_queue(struct slideshow *ss)
{
	assert(ss->have_mutex);

	if (!ss->randomize || ss->file_queue.num <= 1) {
		return;
	}

	// Knuth shuffle
	const int num = (int)ss->file_queue.num;
	for (int i = num - 1; i >= 1; i--) {
		int j;

		// Avoid modulo bias by retrying if the result is outside
		// the range divisible by the upper_bound.
		do {
			j = rand();
		} while (j >= RAND_MAX - (RAND_MAX % i));

		j %= i;

		// Possible optimization: `da_swap` allocates and frees a
		// temporary value, which would not be required for `char *`.
		// Direct array access would be a lot faster. But first do it
		// correct, then optimize if it's too slow.
		da_swap(ss->file_queue, i, j);
	}
}

static void update_queue(struct slideshow *ss)
{
	assert(ss->have_mutex);

	for (size_t i = 0; i < ss->file_queue.num; i++) {
		bfree(ss->file_queue.array[i]);
	}
	da_resize(ss->file_queue, ss->file_paths.num);
	for (size_t i = 0; i < ss->file_queue.num; i++) {
		ss->file_queue.array[i] = bstrdup(ss->file_paths.array[i]);
	}

	if (ss->cur_item >= ss->file_queue.num) {
		ss->cur_item = 0;
	}

	shuffle_queue(ss);
}

static bool valid_extension(const char *ext)
{
	if (!ext)
		return false;
	return astrcmpi(ext, ".bmp") == 0 || astrcmpi(ext, ".tga") == 0 ||
	       astrcmpi(ext, ".png") == 0 || astrcmpi(ext, ".jpeg") == 0 ||
	       astrcmpi(ext, ".jpg") == 0 || astrcmpi(ext, ".gif") == 0;
}

static inline bool item_valid(struct slideshow *ss)
{
	assert(ss->have_mutex);

	return ss->file_queue.num && ss->cur_item < ss->file_queue.num;
}

static bool do_transition(struct slideshow *ss, bool to_null)
{
	assert(ss->have_mutex);
	bool valid = item_valid(ss);

	if (valid && ss->use_cut) {
		obs_source_t *source = obtain_cached_source(ss, ss->cur_item);
		if (!source) {
			debug("No cached source, need to retry");
			ss->retry_transition = true;
			return false;
		}
		obs_transition_set(ss->transition, source);
		obs_source_release(source);
	} else if (valid && !to_null) {
		obs_source_t *source = obtain_cached_source(ss, ss->cur_item);
		if (!source) {
			debug("No cached source, need to retry");
			ss->retry_transition = true;
			return false;
		}
		obs_transition_start(ss->transition, OBS_TRANSITION_MODE_AUTO,
				     ss->tr_speed, source);
		obs_source_release(source);
	} else {
		obs_transition_start(ss->transition, OBS_TRANSITION_MODE_AUTO,
				     ss->tr_speed, NULL);
	}

	ss->retry_transition = false;
	return true;
}

static void set_cur_item(struct slideshow *ss, size_t index)
{
	assert(ss->have_mutex);
	if (ss->cur_item == index)
		return;

	ss->cur_item = index;
	os_event_signal(ss->cache_event);
}

static void ss_update(void *data, obs_data_t *settings)
{
	DARRAY(char *) new_files;
	DARRAY(char *) old_files;
	obs_source_t *new_tr = NULL;
	obs_source_t *old_tr = NULL;
	struct slideshow *ss = data;
	obs_data_array_t *array;
	const char *tr_name;
	uint32_t new_duration;
	uint32_t new_speed;
	uint32_t cx = 0;
	uint32_t cy = 0;
	size_t count;
	const char *behavior;
	const char *mode;

	/* ------------------------------------- */
	/* get settings data */

	da_init(new_files);

	behavior = obs_data_get_string(settings, S_BEHAVIOR);

	if (astrcmpi(behavior, S_BEHAVIOR_PAUSE_UNPAUSE) == 0)
		ss->behavior = BEHAVIOR_PAUSE_UNPAUSE;
	else if (astrcmpi(behavior, S_BEHAVIOR_ALWAYS_PLAY) == 0)
		ss->behavior = BEHAVIOR_ALWAYS_PLAY;
	else /* S_BEHAVIOR_STOP_RESTART */
		ss->behavior = BEHAVIOR_STOP_RESTART;

	mode = obs_data_get_string(settings, S_MODE);

	ss->manual = (astrcmpi(mode, S_MODE_MANUAL) == 0);

	tr_name = obs_data_get_string(settings, S_TRANSITION);
	if (astrcmpi(tr_name, TR_CUT) == 0)
		tr_name = "cut_transition";
	else if (astrcmpi(tr_name, TR_SWIPE) == 0)
		tr_name = "swipe_transition";
	else if (astrcmpi(tr_name, TR_SLIDE) == 0)
		tr_name = "slide_transition";
	else
		tr_name = "fade_transition";

	ss->randomize = obs_data_get_bool(settings, S_RANDOMIZE);
	ss->loop = obs_data_get_bool(settings, S_LOOP);
	ss->hide = obs_data_get_bool(settings, S_HIDE);

	if (!ss->tr_name || strcmp(tr_name, ss->tr_name) != 0)
		new_tr = obs_source_create_private(tr_name, NULL, NULL);

	new_duration = (uint32_t)obs_data_get_int(settings, S_SLIDE_TIME);
	new_speed = (uint32_t)obs_data_get_int(settings, S_TR_SPEED);

	array = obs_data_get_array(settings, S_FILES);
	count = obs_data_array_count(array);

	/* ------------------------------------- */
	/* create new list of sources */

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(array, i);
		const char *path = obs_data_get_string(item, "value");
		os_dir_t *dir = os_opendir(path);

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
				add_file(ss, &new_files.da, dir_path.array, &cx,
					 &cy);
			}

			dstr_free(&dir_path);
			os_closedir(dir);
		} else {
			add_file(ss, &new_files.da, path, &cx, &cy);
		}

		obs_data_release(item);
	}

	/* ------------------------------------- */
	/* update settings data */

	lock_mutex(ss);

	old_files.da = ss->file_paths.da;
	ss->file_paths.da = new_files.da;
	if (new_tr) {
		old_tr = ss->transition;
		ss->transition = new_tr;
	}

	update_queue(ss);

	if (strcmp(tr_name, "cut_transition") != 0) {
		if (new_duration < 100)
			new_duration = 100;

		new_duration += new_speed;
	} else {
		if (new_duration < 50)
			new_duration = 50;
	}

	ss->tr_speed = new_speed;
	ss->tr_name = tr_name;
	ss->slide_time = (float)new_duration / 1000.0f;

	unlock_mutex(ss);

	/* ------------------------------------- */
	/* clean up and restart transition */

	if (old_tr)
		obs_source_release(old_tr);
	free_files(&old_files.da);

	/* ------------------------- */

	const char *res_str = obs_data_get_string(settings, S_CUSTOM_SIZE);
	bool aspect_only = false, use_auto = true;
	int cx_in = 0, cy_in = 0;

	if (strcmp(res_str, T_CUSTOM_SIZE_AUTO) != 0) {
		int ret = sscanf(res_str, "%dx%d", &cx_in, &cy_in);
		if (ret == 2) {
			aspect_only = false;
			use_auto = false;
		} else {
			ret = sscanf(res_str, "%d:%d", &cx_in, &cy_in);
			if (ret == 2) {
				aspect_only = true;
				use_auto = false;
			}
		}
	}

	if (!use_auto) {
		double cx_f = (double)cx;
		double cy_f = (double)cy;

		double old_aspect = cx_f / cy_f;
		double new_aspect = (double)cx_in / (double)cy_in;

		if (aspect_only) {
			if (fabs(old_aspect - new_aspect) > EPSILON) {
				if (new_aspect > old_aspect)
					cx = (uint32_t)(cy_f * new_aspect);
				else
					cy = (uint32_t)(cx_f / new_aspect);
			}
		} else {
			cx = (uint32_t)cx_in;
			cy = (uint32_t)cy_in;
		}
	}

	/* ------------------------- */

	lock_mutex(ss);

	ss->cx = cx;
	ss->cy = cy;
	set_cur_item(ss, 0);
	ss->elapsed = 0.0f;
	obs_transition_set_size(ss->transition, cx, cy);
	obs_transition_set_alignment(ss->transition, OBS_ALIGN_CENTER);
	obs_transition_set_scale_type(ss->transition,
				      OBS_TRANSITION_SCALE_ASPECT);
	if (new_tr)
		obs_source_add_active_child(ss->source, new_tr);
	if (ss->file_paths.num)
		do_transition(ss, false);

	unlock_mutex(ss);

	obs_data_array_release(array);
}

static void ss_play_pause(void *data)
{
	struct slideshow *ss = data;

	lock_mutex(ss);
	ss->paused = !ss->paused;
	ss->manual = ss->paused;
	unlock_mutex(ss);
}

static void ss_restart(void *data)
{
	struct slideshow *ss = data;

	lock_mutex(ss);

	ss->elapsed = 0.0f;
	set_cur_item(ss, 0);

	obs_source_t *source = ss->file_queue.num > 0
				       ? obtain_cached_source(ss, ss->cur_item)
				       : NULL;
	obs_transition_set(ss->transition, source);
	obs_source_release(source);

	ss->stop = false;
	ss->paused = false;

	unlock_mutex(ss);
}

static void ss_stop(void *data)
{
	struct slideshow *ss = data;

	lock_mutex(ss);

	ss->elapsed = 0.0f;
	set_cur_item(ss, 0);

	do_transition(ss, true);
	ss->stop = true;
	ss->paused = false;

	unlock_mutex(ss);
}

static void ss_next_slide(void *data)
{
	struct slideshow *ss = data;

	lock_mutex(ss);

	if (!ss->file_queue.num ||
	    obs_transition_get_time(ss->transition) < 1.0f) {
		unlock_mutex(ss);
		return;
	}

	size_t next = ss->cur_item + 1;
	if (next >= ss->file_queue.num)
		next = 0;
	set_cur_item(ss, next);

	do_transition(ss, false);

	unlock_mutex(ss);
}

static void ss_previous_slide(void *data)
{
	struct slideshow *ss = data;

	lock_mutex(ss);

	if (!ss->file_queue.num ||
	    obs_transition_get_time(ss->transition) < 1.0f) {
		unlock_mutex(ss);
		return;
	}

	if (ss->cur_item == 0)
		set_cur_item(ss, ss->file_queue.num - 1);
	else
		set_cur_item(ss, ss->cur_item - 1);

	do_transition(ss, false);

	unlock_mutex(ss);
}

static void play_pause_hotkey(void *data, obs_hotkey_id id,
			      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_active(ss->source))
		ss_play_pause(ss);
}

static void restart_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey,
			   bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_active(ss->source))
		ss_restart(ss);
}

static void stop_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey,
			bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (pressed && obs_source_active(ss->source))
		ss_stop(ss);
}

static void next_slide_hotkey(void *data, obs_hotkey_id id,
			      obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (!ss->manual)
		return;

	if (pressed && obs_source_active(ss->source))
		ss_next_slide(ss);
}

static void previous_slide_hotkey(void *data, obs_hotkey_id id,
				  obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct slideshow *ss = data;

	if (!ss->manual)
		return;

	if (pressed && obs_source_active(ss->source))
		ss_previous_slide(ss);
}

static void ss_destroy(void *data)
{
	struct slideshow *ss = data;

	if (ss->cache_thread_created) {
		pthread_cancel(ss->cache_thread);
		pthread_join(ss->cache_thread, NULL);
	}

	clear_cache(ss);

	for (size_t i = 0; i < ss->source_cleanup.num; i++) {
		obs_source_release(ss->source_cleanup.array[i]);
	}
	da_free(ss->source_cleanup);

	os_event_destroy(ss->cache_event);
	obs_source_release(ss->transition);
	free_files(&ss->file_paths.da);
	free_files(&ss->file_queue.da);
	pthread_mutex_destroy(&ss->mutex);
	bfree(ss);
}

static void *ss_create(obs_data_t *settings, obs_source_t *source)
{
	struct slideshow *ss = bzalloc(sizeof(*ss));

	ss->source = source;

	ss->manual = false;
	ss->paused = false;
	ss->stop = false;

	ss->play_pause_hotkey = obs_hotkey_register_source(
		source, "SlideShow.PlayPause",
		obs_module_text("SlideShow.PlayPause"), play_pause_hotkey, ss);

	ss->restart_hotkey = obs_hotkey_register_source(
		source, "SlideShow.Restart",
		obs_module_text("SlideShow.Restart"), restart_hotkey, ss);

	ss->stop_hotkey = obs_hotkey_register_source(
		source, "SlideShow.Stop", obs_module_text("SlideShow.Stop"),
		stop_hotkey, ss);

	ss->prev_hotkey = obs_hotkey_register_source(
		source, "SlideShow.NextSlide",
		obs_module_text("SlideShow.NextSlide"), next_slide_hotkey, ss);

	ss->prev_hotkey = obs_hotkey_register_source(
		source, "SlideShow.PreviousSlide",
		obs_module_text("SlideShow.PreviousSlide"),
		previous_slide_hotkey, ss);

	pthread_mutex_init_value(&ss->mutex);
	if (pthread_mutex_init(&ss->mutex, NULL) != 0)
		goto error;

	if (os_event_init(&ss->cache_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto error;
	if (pthread_create(&ss->cache_thread, NULL, cache_thread, ss) != 0)
		goto error;
	ss->cache_thread_created = true;

	obs_source_update(source, NULL);

	UNUSED_PARAMETER(settings);
	return ss;

error:
	ss_destroy(ss);
	return NULL;
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

	lock_mutex(ss);

	if (!ss->transition || !ss->slide_time)
		goto out;

	if (ss->restart_on_activate && !ss->randomize && ss->use_cut) {
		ss->elapsed = 0.0f;
		set_cur_item(ss, 0);
		do_transition(ss, false);
		ss->restart_on_activate = false;
		ss->use_cut = false;
		ss->stop = false;
		goto out;
	}

	if (ss->pause_on_deactivate || ss->manual || ss->stop || ss->paused)
		goto out;

	/* ----------------------------------------------------- */
	/* fade to transparency when the file list becomes empty */
	if (!ss->file_queue.num) {
		obs_source_t *active_transition_source =
			obs_transition_get_active_source(ss->transition);

		if (active_transition_source) {
			obs_source_release(active_transition_source);
			do_transition(ss, true);
		}
	}

	/* ----------------------------------------------------- */
	/* do transition when slide time reached                 */
	ss->elapsed += seconds;

	if (ss->elapsed > ss->slide_time) {
		ss->elapsed -= ss->slide_time;

		if (!ss->loop && ss->cur_item == ss->file_queue.num - 1) {
			if (ss->hide)
				do_transition(ss, true);
			else
				do_transition(ss, false);

			goto out;
		}

		size_t next = ss->cur_item + 1;
		if (next >= ss->file_queue.num) {
			next = 0;
			shuffle_queue(ss);
			clear_cache(ss);
		}
		set_cur_item(ss, next);

		if (ss->file_queue.num)
			do_transition(ss, false);
	} else if (ss->retry_transition) {
		debug("Retrying transition");
		do_transition(ss, false);
	}

out:
	unlock_mutex(ss);
}

static inline bool ss_audio_render_(obs_source_t *transition, uint64_t *ts_out,
				    struct obs_source_audio_mix *audio_output,
				    uint32_t mixers, size_t channels,
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

			memcpy(out, in,
			       AUDIO_OUTPUT_FRAMES * MAX_AUDIO_CHANNELS *
				       sizeof(float));
		}
	}

	*ts_out = source_ts;

	UNUSED_PARAMETER(sample_rate);
	return true;
}

static bool ss_audio_render(void *data, uint64_t *ts_out,
			    struct obs_source_audio_mix *audio_output,
			    uint32_t mixers, size_t channels,
			    size_t sample_rate)
{
	struct slideshow *ss = data;
	obs_source_t *transition = get_transition(ss);
	bool success;

	if (!transition)
		return false;

	success = ss_audio_render_(transition, ts_out, audio_output, mixers,
				   channels, sample_rate);

	obs_source_release(transition);
	return success;
}

static void ss_enum_sources(void *data, obs_source_enum_proc_t cb, void *param)
{
	struct slideshow *ss = data;

	lock_mutex(ss);
	if (ss->transition)
		cb(ss->source, ss->transition, param);
	unlock_mutex(ss);
}

static uint32_t ss_width(void *data)
{
	struct slideshow *ss = data;
	lock_mutex(ss);
	uint32_t result = ss->transition ? ss->cx : 0;
	unlock_mutex(ss);
	return result;
}

static uint32_t ss_height(void *data)
{
	struct slideshow *ss = data;
	lock_mutex(ss);
	uint32_t result = ss->transition ? ss->cy : 0;
	unlock_mutex(ss);
	return result;
}

static void ss_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, S_TRANSITION, "fade");
	obs_data_set_default_int(settings, S_SLIDE_TIME, 8000);
	obs_data_set_default_int(settings, S_TR_SPEED, 700);
	obs_data_set_default_string(settings, S_CUSTOM_SIZE,
				    T_CUSTOM_SIZE_AUTO);
	obs_data_set_default_string(settings, S_BEHAVIOR,
				    S_BEHAVIOR_ALWAYS_PLAY);
	obs_data_set_default_string(settings, S_MODE, S_MODE_AUTO);
	obs_data_set_default_bool(settings, S_LOOP, true);
}

static const char *file_filter =
	"Image files (*.bmp *.tga *.png *.jpeg *.jpg *.gif)";

static const char *aspects[] = {"16:9", "16:10", "4:3", "1:1"};

#define NUM_ASPECTS (sizeof(aspects) / sizeof(const char *))

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

	p = obs_properties_add_list(ppts, S_BEHAVIOR, T_BEHAVIOR,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_BEHAVIOR_ALWAYS_PLAY,
				     S_BEHAVIOR_ALWAYS_PLAY);
	obs_property_list_add_string(p, T_BEHAVIOR_STOP_RESTART,
				     S_BEHAVIOR_STOP_RESTART);
	obs_property_list_add_string(p, T_BEHAVIOR_PAUSE_UNPAUSE,
				     S_BEHAVIOR_PAUSE_UNPAUSE);

	p = obs_properties_add_list(ppts, S_MODE, T_MODE, OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_MODE_AUTO, S_MODE_AUTO);
	obs_property_list_add_string(p, T_MODE_MANUAL, S_MODE_MANUAL);

	p = obs_properties_add_list(ppts, S_TRANSITION, T_TRANSITION,
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_TR_CUT, TR_CUT);
	obs_property_list_add_string(p, T_TR_FADE, TR_FADE);
	obs_property_list_add_string(p, T_TR_SWIPE, TR_SWIPE);
	obs_property_list_add_string(p, T_TR_SLIDE, TR_SLIDE);

	obs_properties_add_int(ppts, S_SLIDE_TIME, T_SLIDE_TIME, 50, 3600000,
			       50);
	obs_properties_add_int(ppts, S_TR_SPEED, T_TR_SPEED, 0, 3600000, 50);
	obs_properties_add_bool(ppts, S_LOOP, T_LOOP);
	obs_properties_add_bool(ppts, S_HIDE, T_HIDE);
	obs_properties_add_bool(ppts, S_RANDOMIZE, T_RANDOMIZE);

	p = obs_properties_add_list(ppts, S_CUSTOM_SIZE, T_CUSTOM_SIZE,
				    OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, T_CUSTOM_SIZE_AUTO, T_CUSTOM_SIZE_AUTO);

	for (size_t i = 0; i < NUM_ASPECTS; i++)
		obs_property_list_add_string(p, aspects[i], aspects[i]);

	char str[32];
	snprintf(str, 32, "%dx%d", cx, cy);
	obs_property_list_add_string(p, str, str);

	if (ss) {
		lock_mutex(ss);
		if (ss->file_paths.num) {
			char **last = da_end(ss->file_paths);
			const char *slash;

			dstr_copy(&path, *last);
			dstr_replace(&path, "\\", "/");
			slash = strrchr(path.array, '/');
			if (slash)
				dstr_resize(&path, slash - path.array + 1);
		}
		unlock_mutex(ss);
	}

	obs_properties_add_editable_list(ppts, S_FILES, T_FILES,
					 OBS_EDITABLE_LIST_TYPE_FILES,
					 file_filter, path.array);
	dstr_free(&path);

	return ppts;
}

static void ss_activate(void *data)
{
	struct slideshow *ss = data;

	if (ss->behavior == BEHAVIOR_STOP_RESTART) {
		ss->restart_on_activate = true;
		ss->use_cut = true;
	} else if (ss->behavior == BEHAVIOR_PAUSE_UNPAUSE) {
		ss->pause_on_deactivate = false;
	}
}

static void ss_deactivate(void *data)
{
	struct slideshow *ss = data;

	if (ss->behavior == BEHAVIOR_PAUSE_UNPAUSE)
		ss->pause_on_deactivate = true;
}

struct obs_source_info slideshow_info = {
	.id = "slideshow",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
			OBS_SOURCE_COMPOSITE,
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
	.icon_type = OBS_ICON_TYPE_SLIDESHOW,
};
