#pragma once

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct os_task_queue;
typedef struct os_task_queue os_task_queue_t;

typedef void (*os_task_t)(void *param);

EXPORT os_task_queue_t *os_task_queue_create(void);
EXPORT bool os_task_queue_queue_task(os_task_queue_t *tt, os_task_t task,
				     void *param);
EXPORT void os_task_queue_destroy(os_task_queue_t *tt);
EXPORT bool os_task_queue_wait(os_task_queue_t *tt);
EXPORT bool os_task_queue_inside(os_task_queue_t *tt);

#ifdef __cplusplus
}
#endif
