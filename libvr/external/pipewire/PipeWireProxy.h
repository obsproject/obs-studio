#pragma once

#include <stdbool.h>

namespace libvr {

// Opaque handles representing PipeWire types
typedef struct pw_loop pw_loop;
typedef struct pw_context pw_context;
typedef struct pw_core pw_core;
typedef struct pw_thread_loop pw_thread_loop;
typedef struct pw_stream pw_stream;
typedef struct pw_properties pw_properties;

// Function pointers
typedef struct pw_properties* (*t_pw_properties_new)(const char *key, ...) __attribute__ ((sentinel));
typedef struct pw_thread_loop* (*t_pw_thread_loop_new)(const char *name, const struct pw_properties *props);
typedef int (*t_pw_thread_loop_start)(struct pw_thread_loop *loop);
typedef void (*t_pw_thread_loop_stop)(struct pw_thread_loop *loop);
typedef void (*t_pw_thread_loop_destroy)(struct pw_thread_loop *loop);
typedef struct pw_loop* (*t_pw_thread_loop_get_loop)(struct pw_thread_loop *loop);

typedef void (*t_pw_init)(int *argc, char **argv[]);
// ... add more as needed for source implementation

bool LoadPipeWireFunctions();

// Wrapper accessors
extern t_pw_init wrapper_pw_init;
extern t_pw_thread_loop_new wrapper_pw_thread_loop_new;
extern t_pw_thread_loop_start wrapper_pw_thread_loop_start;

} // namespace libvr
