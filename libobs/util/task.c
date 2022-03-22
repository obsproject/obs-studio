#include "task.h"
#include "bmem.h"
#include "threading.h"
#include "circlebuf.h"

struct os_task_queue {
	pthread_t thread;
	os_sem_t *sem;
	long id;

	bool waiting;
	bool tasks_processed;
	os_event_t *wait_event;

	pthread_mutex_t mutex;
	struct circlebuf tasks;
};

struct os_task_info {
	os_task_t task;
	void *param;
};

static THREAD_LOCAL bool exit_thread = false;
static THREAD_LOCAL long thread_id = 0;
static volatile long thread_id_counter = 1;

static void *tiny_tubular_task_thread(void *param);

os_task_queue_t *os_task_queue_create()
{
	struct os_task_queue *tq = bzalloc(sizeof(*tq));
	tq->id = os_atomic_inc_long(&thread_id_counter);

	if (pthread_mutex_init(&tq->mutex, NULL) != 0)
		goto fail1;
	if (os_sem_init(&tq->sem, 0) != 0)
		goto fail2;
	if (os_event_init(&tq->wait_event, OS_EVENT_TYPE_AUTO) != 0)
		goto fail3;
	if (pthread_create(&tq->thread, NULL, tiny_tubular_task_thread, tq) !=
	    0)
		goto fail4;

	return tq;

fail4:
	os_event_destroy(tq->wait_event);
fail3:
	os_sem_destroy(tq->sem);
fail2:
	pthread_mutex_destroy(&tq->mutex);
fail1:
	bfree(tq);
	return NULL;
}

bool os_task_queue_queue_task(os_task_queue_t *tq, os_task_t task, void *param)
{
	struct os_task_info ti = {
		task,
		param,
	};

	if (!tq)
		return false;

	pthread_mutex_lock(&tq->mutex);
	circlebuf_push_back(&tq->tasks, &ti, sizeof(ti));
	pthread_mutex_unlock(&tq->mutex);
	os_sem_post(tq->sem);
	return true;
}

static void wait_for_thread(void *data)
{
	os_task_queue_t *tq = data;
	os_event_signal(tq->wait_event);
}

static void stop_thread(void *unused)
{
	exit_thread = true;
	UNUSED_PARAMETER(unused);
}

void os_task_queue_destroy(os_task_queue_t *tq)
{
	if (!tq)
		return;

	os_task_queue_queue_task(tq, stop_thread, NULL);
	pthread_join(tq->thread, NULL);
	os_event_destroy(tq->wait_event);
	os_sem_destroy(tq->sem);
	pthread_mutex_destroy(&tq->mutex);
	circlebuf_free(&tq->tasks);
	bfree(tq);
}

bool os_task_queue_wait(os_task_queue_t *tq)
{
	if (!tq)
		return false;

	struct os_task_info ti = {
		wait_for_thread,
		tq,
	};

	pthread_mutex_lock(&tq->mutex);
	tq->waiting = true;
	tq->tasks_processed = false;
	circlebuf_push_back(&tq->tasks, &ti, sizeof(ti));
	pthread_mutex_unlock(&tq->mutex);

	os_sem_post(tq->sem);
	os_event_wait(tq->wait_event);

	pthread_mutex_lock(&tq->mutex);
	bool tasks_processed = tq->tasks_processed;
	pthread_mutex_unlock(&tq->mutex);

	return tasks_processed;
}

bool os_task_queue_inside(os_task_queue_t *tq)
{
	return tq->id == thread_id;
}

static void *tiny_tubular_task_thread(void *param)
{
	struct os_task_queue *tq = param;
	thread_id = tq->id;

	os_set_thread_name(__FUNCTION__);

	while (!exit_thread && os_sem_wait(tq->sem) == 0) {
		struct os_task_info ti;

		pthread_mutex_lock(&tq->mutex);
		circlebuf_pop_front(&tq->tasks, &ti, sizeof(ti));
		if (tq->tasks.size && ti.task == wait_for_thread) {
			circlebuf_push_back(&tq->tasks, &ti, sizeof(ti));
			circlebuf_pop_front(&tq->tasks, &ti, sizeof(ti));
		}
		if (tq->tasks.size && ti.task == stop_thread) {
			circlebuf_push_back(&tq->tasks, &ti, sizeof(ti));
			circlebuf_pop_front(&tq->tasks, &ti, sizeof(ti));
		}
		if (tq->waiting) {
			if (ti.task == wait_for_thread) {
				tq->waiting = false;
			} else {
				tq->tasks_processed = true;
			}
		}
		pthread_mutex_unlock(&tq->mutex);

		ti.task(ti.param);
	}

	return NULL;
}
