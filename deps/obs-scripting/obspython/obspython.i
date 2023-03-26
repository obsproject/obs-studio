%module(threads="1") obspython
%nothread;
%{
#define SWIG_FILE_WITH_INIT
#define DEPRECATED_START
#define DEPRECATED_END
#include <graphics/graphics.h>
#include <graphics/vec4.h>
#include <graphics/vec3.h>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>
#include <graphics/matrix3.h>
#include <graphics/quat.h>
#include <obs.h>
#include <obs-hotkey.h>
#include <obs-source.h>
#include <obs-data.h>
#include <obs-properties.h>
#include <obs-interaction.h>
#include <callback/calldata.h>
#include <callback/decl.h>
#include <callback/proc.h>
#include <callback/signal.h>
#include <util/bmem.h>
#include <util/base.h>
#include "obspython.h"
#include <util/platform.h>

#if defined(ENABLE_UI)
#include "obs-frontend-api.h"
#endif

/* Redefine SWIG_PYTHON_INITIALIZE_THREADS if:
 * - Python version is 3.7 or later because PyEval_InitThreads() became deprecated and unnecessary
 * - SWIG version is not 4.1 or later because SWIG_PYTHON_INITIALIZE_THREADS will be define correctly
 *   with Python 3.7 and later */
#if PY_VERSION_HEX >= 0x03070000 && SWIGVERSION < 0x040100
#undef SWIG_PYTHON_INITIALIZE_THREADS
#define SWIG_PYTHON_INITIALIZE_THREADS
#endif
%}

%feature("python:annotations", "c");
%feature("autodoc", "2");

#define DEPRECATED_START
#define DEPRECATED_END
#define OBS_DEPRECATED
#define OBS_EXTERNAL_DEPRECATED
#define EXPORT

%rename(blog) wrap_blog;
%inline %{
static inline void wrap_blog(int log_level, const char *message)
{
        blog(log_level, "%s", message);
}
%}

%include "stdint.i"

/* Used to free when using %newobject functions.  E.G.:
 * %newobject obs_module_get_config_path; */
%typemap(newfree) char * "bfree($1);";

%ignore blog;
%ignore blogva;
%ignore bcrash;
%ignore base_set_crash_handler;
%ignore obs_source_info;
%ignore obs_register_source_s(const struct obs_source_info *info, size_t size);
%ignore obs_output_set_video(obs_output_t *output, video_t *video);
%ignore obs_output_video(const obs_output_t *output);
%ignore obs_add_tick_callback;
%ignore obs_remove_tick_callback;
%ignore obs_add_main_render_callback;
%ignore obs_remove_main_render_callback;
%ignore obs_enum_sources;
%ignore obs_properties_add_button;
%ignore obs_property_set_modified_callback;
%ignore signal_handler_connect;
%ignore signal_handler_disconnect;
%ignore signal_handler_connect_global;
%ignore signal_handler_disconnect_global;
%ignore signal_handler_remove_current;
%ignore obs_hotkey_register_frontend;
%ignore obs_hotkey_register_encoder;
%ignore obs_hotkey_register_output;
%ignore obs_hotkey_register_service;
%ignore obs_hotkey_register_source;
%ignore obs_hotkey_pair_register_frontend;
%ignore obs_hotkey_pair_register_encoder;
%ignore obs_hotkey_pair_register_output;
%ignore obs_hotkey_pair_register_service;
%ignore obs_hotkey_pair_register_source;

/* The function gs_debug_marker_begin_format has a va_args.
 * By default, SWIG just drop it and replace it with a single NULL pointer.
 * Source: http://swig.org/Doc4.0/Varargs.html#Varargs_nn4
 *
 * But the generated wrapper will make the compiler emit a warning
 * because varargs is an unused parameter.
 * So in the check step, varargs will be treated like any unused parameter. */
%typemap(check) (const float color[4], const char *format, ...) {
	(void)varargs;
}

%include "graphics/graphics.h"
%include "graphics/vec4.h"
%include "graphics/vec3.h"
%include "graphics/vec2.h"
%include "graphics/matrix4.h"
%include "graphics/matrix3.h"
%include "graphics/quat.h"
%include "obspython.h"
%include "obs-data.h"
%include "obs-source.h"
%include "obs-properties.h"
%include "obs-interaction.h"
%include "obs-hotkey.h"
%include "obs.h"
%include "callback/calldata.h"
%include "callback/proc.h"
%include "callback/signal.h"
%include "util/bmem.h"
%include "util/base.h"
%include "util/platform.h"

#if defined(ENABLE_UI)
%include "obs-frontend-api.h"
#endif

/* declare these manually because mutex + GIL = deadlocks */
%thread;
void obs_enter_graphics(void); //Should only block on entering mutex
%nothread;
%include "obs.h"
