#pragma once

#include "base.h"
#include "darray.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct profiler_snapshot profiler_snapshot_t;
typedef struct profiler_snapshot_entry profiler_snapshot_entry_t;
typedef struct profiler_time_entry profiler_time_entry_t;

/* ------------------------------------------------------------------------- */
/* Profiling */

/*!
 * \brief Register a new named root Scope.
 * 
 * Named roots can be used to differentiate between profiling sessions and/or
 * to track how much time actually passed between calls. For example a thread
 * that is expected to run every 16.6ms can use this to split itself from the
 * main program root. It must be opened via #profile_start and closed via 
 * #profile_end.
 * 
 * \param name Scope Name
 * \param expected_time_between_calls Expected Time (in nanoseconds) between calls
 * \sa #profile_start #profile_end
 */
EXPORT void profile_register_root(const char *name,
		uint64_t expected_time_between_calls);

/*!
 * \brief Start profiling a named Scope.
 * 
 * Begins profiling a named Scope, creating it if it didn't exist before. You
 * must either call #profile_end with the Scope name to close it or 
 * #profile_start to begin a child profiling session.
 * 
 * \param name Name of the Scope
 * \sa #profile_end profile_register_root
 */
EXPORT void profile_start(const char *name);

/*!
 * \brief End profiling a named Scope
 * 
 * Ends profiling a named Scope and stores all tracked values for use later.
 * Must be called after calling #profile_start with the same name provided to
 * it.
 * 
 * \param name Name of the Scope
 * \sa profile_start profile_register_root
 */
EXPORT void profile_end(const char *name);

/*!
 * \brief Re-enable the Profiler for the current Thread.
 * 
 * Updates the thread-local flag which is used by profiler functions to check 
 * if the Profiler is enabled. Should be called in the same loop that 
 * #profile_start or #profile_end are to be used in.
 * 
 * \see #profiler_start #profiler_stop
 */
EXPORT void profile_reenable_thread(void);

/* ------------------------------------------------------------------------- */
/* Profiler control */

/*!
 * \brief Enable the Profiler.
 * 
 * After calling this, each thread that intends to use the Profiler should call
 * #profile_reenable_thread to ensure that the thread-local flag is also updated.
 * 
 * \see #profiler_stop #profile_reenable_thread
 */
EXPORT void profiler_start(void);

/*!
 * \brief Disable the Profiler.
 * 
 * Flips the internal switch for the Profiler off and thus turns any Profiler 
 * calls into no-ops. A thread should call #profile_reenable_thread to ensure 
 * it is always aware of the current profiler state.
 *
 * \see #profiler_start #profile_reenable_thread
 */
EXPORT void profiler_stop(void);

/*!
 * \brief Print the given Profiler Snapshot to the Log.
 * 
 * <b>Example Output</b>
 * \code
== Profiler Results =============================
run_program_init: 4339.58 ms
 ┣OBSApp::AppInit: 1.618 ms
 ┃ ┗OBSApp::InitLocale: 0.767 ms
 ┗OBSApp::OBSInit: 4293.64 ms
   ┣obs_startup: 1.254 ms
   ┗OBSBasic::OBSInit: 4193.72 ms
	 ┣OBSBasic::InitBasicConfig: 0.306 ms
	 ┣OBSBasic::ResetAudio: 0.205 ms
	 ┣OBSBasic::ResetVideo: 745.175 ms
	 ┣OBSBasic::InitOBSCallbacks: 0.009 ms
	 ┣OBSBasic::InitHotkeys: 0.045 ms
	 ┣obs_load_all_modules: 2745.34 ms
	 ┃ ┣obs_init_module(coreaudio-encoder.dll): 4.974 ms
	 ┃ ┣obs_init_module(enc-amf.dll): 2266.96 ms
	 ┃ ┣obs_init_module(frontend-tools.dll): 1.36 ms
	 ┃ ┣obs_init_module(image-source.dll): 0.002 ms
	 ┃ ┣obs_init_module(obs-browser.dll): 0.772 ms
	 ┃ ┣obs_init_module(obs-ffmpeg.dll): 11.318 ms
	 ┃ ┣obs_init_module(obs-filters.dll): 0.007 ms
	 ┃ ┣obs_init_module(obs-outputs.dll): 0.004 ms
	 ┃ ┣obs_init_module(obs-qsv11.dll): 151.131 ms
	 ┃ ┣obs_init_module(obs-text.dll): 2.126 ms
	 ┃ ┣obs_init_module(obs-transitions.dll): 0.006 ms
	 ┃ ┣obs_init_module(obs-x264.dll): 0.002 ms
	 ┃ ┣obs_init_module(rtmp-services.dll): 0.529 ms
	 ┃ ┣obs_init_module(text-freetype2.dll): 0.052 ms
	 ┃ ┣obs_init_module(win-capture.dll): 0.068 ms
	 ┃ ┣obs_init_module(win-decklink.dll): 2.635 ms
	 ┃ ┣obs_init_module(win-dshow.dll): 2.433 ms
	 ┃ ┣obs_init_module(win-mf.dll): 216.128 ms
	 ┃ ┣obs_init_module(win-wasapi.dll): 0.005 ms
	 ┃ ┗reset_win32_symbol_paths: 0.094 ms
	 ┣OBSBasic::ResetOutputs: 31.766 ms
	 ┣OBSBasic::CreateHotkeys: 0.692 ms
	 ┣OBSBasic::InitService: 0.214 ms
	 ┣OBSBasic::InitPrimitives: 0.522 ms
	 ┗OBSBasic::Load: 589.449 ms
obs_hotkey_thread(25 ms): min=0.002 ms, median=0.006 ms, max=12.892 ms, 99th percentile=0.254 ms, 100% below 25 ms
audio_thread(Audio): min=0 ms, median=0.028 ms, max=16.255 ms, 99th percentile=2.813 ms
obs_video_thread(16.6667 ms): min=0.165 ms, median=3.106 ms, max=534.168 ms, 99th percentile=11.732 ms, 99.7407% below 16.667 ms
 ┣tick_sources: min=0 ms, median=0.016 ms, max=533.31 ms, 99th percentile=0.349 ms
 ┣render_displays: min=0.001 ms, median=1.879 ms, max=24.784 ms, 99th percentile=7.354 ms
 ┗output_frame: min=0.161 ms, median=0.951 ms, max=67.956 ms, 99th percentile=8.492 ms
   ┣gs_context(video->graphics): min=0.1 ms, median=0.34 ms, max=67.472 ms, 99th percentile=7.668 ms
   ┃ ┣render_video: min=0.027 ms, median=0.05 ms, max=46.568 ms, 99th percentile=1.249 ms
   ┃ ┃ ┣render_main_texture: min=0.003 ms, median=0.013 ms, max=6.146 ms, 99th percentile=0.35 ms
   ┃ ┃ ┣render_output_texture: min=0.003 ms, median=0.008 ms, max=4.951 ms, 99th percentile=0.102 ms
   ┃ ┃ ┣render_convert_texture: min=0.001 ms, median=0.009 ms, max=4.79 ms, 99th percentile=0.106 ms
   ┃ ┃ ┗stage_output_texture: min=0 ms, median=0.013 ms, max=46.53 ms, 99th percentile=0.724 ms
   ┃ ┣download_frame: min=0 ms, median=0.008 ms, max=67.383 ms, 99th percentile=4.539 ms
   ┃ ┗gs_flush: min=0.011 ms, median=0.246 ms, max=23.416 ms, 99th percentile=4.757 ms
   ┗output_video_data: min=0.368 ms, median=0.56 ms, max=10.435 ms, 99th percentile=1.976 ms
video_thread(video): min=0 ms, median=0.001 ms, max=6.035 ms, 99th percentile=0.009 ms
=================================================
 * \endcode
 * 
 * \param snap Snapshot
 * \see #profiler_print_time_between_calls
 */
EXPORT void profiler_print(profiler_snapshot_t *snap);

/*!
 * \brief Print the time between calls in the given Profiler to the Log
 *
 * <b>Example Output</b>
 * \code
== Profiler Time Between Calls ==================
obs_hotkey_thread(25 ms): min=24.524 ms, median=25.493 ms, max=63.275 ms, 53.8532% within ±2% of 25 ms (0% lower, 46.1468% higher)
obs_video_thread(16.6667 ms): min=1.309 ms, median=16.667 ms, max=534.178 ms, 81.7467% within ±2% of 16.667 ms (9.22716% lower, 9.02616% higher)
=================================================
 * \endcode
 * 
 * \param snap Snapshot
 * \see #profiler_print
 */
EXPORT void profiler_print_time_between_calls(profiler_snapshot_t *snap);

/*!
 * \brief Disable the Profiler and free all associated memory.
 * 
 * Calling this will disable the profiler (if it is enabled) and then free all
 * associated memory with the Profiler. Snapshots must be manually freed using
 * #profile_snapshot_free as they are not part of the associated memory.
 * 
 */
EXPORT void profiler_free(void);

/* ------------------------------------------------------------------------- */
/* Profiler name storage */

typedef struct profiler_name_store profiler_name_store_t;

/*!
 * \brief Create a new Name Storage.
 * 
 * A Name Storage is a structure that holds names which will be freed when 
 * #profiler_name_store_free is called.
 * 
 * @return The new Name Storage
 */
EXPORT profiler_name_store_t *profiler_name_store_create(void);

/*!
 * \brief Free an exisiting Name Storage.
 * 
 * Frees all associated memory with the object and invalidates all stored names.
 * 
 * \param store The Name Storage
 */
EXPORT void profiler_name_store_free(profiler_name_store_t *store);

// \cond
#ifndef _MSC_VER
#define PRINTFATTR(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define PRINTFATTR(f, a)
#endif
// \endcond

/*!
 * \brief Store a new formatted Name
 * 
 * Stores a new formatted Name and returns a pointer to it.
 * Returned pointer is valid until #profiler_name_store_free is called.
 * 
 * \param store Name Storage
 * \param format Format String (in printf format)
 * \param ... Parameters for the Format String
 * @return Formatted Name
 */
PRINTFATTR(2, 3)
EXPORT const char *profile_store_name(profiler_name_store_t *store,
		const char *format, ...);

// \cond
#undef PRINTFATTR
// \endcond

/* ------------------------------------------------------------------------- */
/* Profiler data access */

struct profiler_time_entry {
	uint64_t time_delta;
	uint64_t count;
};

typedef DARRAY(profiler_time_entry_t) profiler_time_entries_t;

/*!
 * \brief Callback Definition
 * 
 * 
 * \param context
 * \param entry
 * @return 
 */
typedef bool (*profiler_entry_enum_func)(void *context,
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Create a Snapshot of the current Profiling Session
 * 
 * Creates a static copy (Snapshot) of the current Profiling Session data, allowing fast and
 * unsynchronized access to all entries in it. You should call #profile_snapshot_free once
 * you have completed all work on the snapshot.
 * Warning: This requires that the Profiler is synchronized, so stuttering may be experienced.
 * 
 * @return The created Snapshot
 */
EXPORT profiler_snapshot_t *profile_snapshot_create(void);

/*!
 * \brief Free a created Snapshot
 * 
 * Frees the Snapshot and the associated memory with it, invalidating any references to it and
 * its contents.
 * 
 * \param snap Snapshot to free
 */
EXPORT void profile_snapshot_free(profiler_snapshot_t *snap);

/*!
 * \brief Dump a Snapshot to a CSV formatted file
 * 
 * 
 * \param snap Snapshot to dump as CSV
 * \param filename Relative or full path to the file
 * @return True if successful, otherwise false
 */
EXPORT bool profiler_snapshot_dump_csv(const profiler_snapshot_t *snap,
		const char *filename);
/*!
 * \brief Dump a Snapshot to a GZip-compressed CSV formatted file
 * 
 * 
 * \param snap Snapshot to dump
 * \param filename Relative or full path to the file
 * @return True if successful, otherwise false
 */
EXPORT bool profiler_snapshot_dump_csv_gz(const profiler_snapshot_t *snap,
		const char *filename);

/*!
 * \brief Count the Root-entries in a Snapshot
 * 
 * 
 * \param snap Snapshot
 * @return Amount of Root-Entries
 */
EXPORT size_t profiler_snapshot_num_roots(profiler_snapshot_t *snap);

/*!
 * \brief Enumerate all Root-Entries using a Callback
 * 
 * 
 * \param snap Snapshot to enumerate over
 * \param func Callback function
 * \param context Custom context (passed to Callback function)
 */
EXPORT void profiler_snapshot_enumerate_roots(profiler_snapshot_t *snap,
		profiler_entry_enum_func func, void *context);

typedef bool (*profiler_name_filter_func)(void *data, const char *name,
		bool *remove);
/*!
 * \brief Filter Root-Entries by Name
 * 
 * 
 * \param snap Snapshot to filter
 * \param func Callback function
 * \param data Custom context (passed to Callback function)
 */
EXPORT void profiler_snapshot_filter_roots(profiler_snapshot_t *snap,
		profiler_name_filter_func func, void *data);

/*!
 * \brief Count the Children a Snapshot Entry has
 * 
 * 
 * \param entry Snapshot Entry
 * @return Amount of Children
 */
EXPORT size_t profiler_snapshot_num_children(profiler_snapshot_entry_t *entry);

/*!
 * \brief Enumerate all Children for a Snapshot Entry
 * 
 * 
 * \param entry Snapshot Entry
 * \param func Callback function
 * \param context Context passed to the Callback function
 */
EXPORT void profiler_snapshot_enumerate_children(
		profiler_snapshot_entry_t *entry,
		profiler_entry_enum_func func, void *context);

/*!
 * \brief Name of a Snapshot Entry
 * 
 * 
 * \param entry Snapshot Entry
 * @return Name for the Entry
 */
EXPORT const char *profiler_snapshot_entry_name(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get all Time entries for a Snapshot Entry
 * 
 * 
 * \param entry Snapshot Entry
 * @return All time entries for the Snapshot Entry
 */
EXPORT profiler_time_entries_t *profiler_snapshot_entry_times(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get the minimum amount of time passed for this Entry
 * 
 * 
 * \param entry Snapshot Entry
 * @return Minimum time (in nanoseconds) for a call
 */
EXPORT uint64_t profiler_snapshot_entry_min_time(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get the maximum amount of time passed for this Entry
 * 
 * 
 * \param entry Snapshot Entry
 * @return Maximum time (in nanoseconds) for a call
 */
EXPORT uint64_t profiler_snapshot_entry_max_time(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get how often the Entry has been profiled
 * 
 * 
 * \param entry Snapshot Entry
 * @return Amount of times this Entry has been profiled
 */
EXPORT uint64_t profiler_snapshot_entry_overall_count(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Retrieve all between-call timing information for an Entry
 * 
 * 
 * \param entry Snapshot Entry
 * @return All between-call timing information Entries
 */
EXPORT profiler_time_entries_t *profiler_snapshot_entry_times_between_calls(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get the expected time in Nanoseconds between calls
 * 
 * 
 * \param entry Snapshot Entry
 * @return Expected time (in nanoseconds) between calls.
 */
EXPORT uint64_t profiler_snapshot_entry_expected_time_between_calls(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get the minimum time in nanoseconds between calls
 * 
 * 
 * \param entry Snapshot Entry
 * @return Minimum time (in nanoseconds) between calls.
 */
EXPORT uint64_t profiler_snapshot_entry_min_time_between_calls(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get the maximum time in Nanoseconds between calls
 * 
 * 
 * \param entry Snapshot Entry
 * @return Maximum time (in nanoseconds) between calls.
 */
EXPORT uint64_t profiler_snapshot_entry_max_time_between_calls(
		profiler_snapshot_entry_t *entry);

/*!
 * \brief Get the amount of timing information available
 * 
 * 
 * \param entry
 * @return Amount of timing information
 */
EXPORT uint64_t profiler_snapshot_entry_overall_between_calls_count(
		profiler_snapshot_entry_t *entry);

#ifdef __cplusplus
}
#endif

/*! \example profiler_app-usage.c
* This is an example on how to use the Profiler in you own application.
*/
