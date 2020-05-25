#ifndef _QUEUE_H
#define _QUEUE_H

/*
 * Simple thread safe queue is an experiment with simulated "namespaces" 
 * in C.
 *
 * https://raw.githubusercontent.com/mapuna/threadsafe_queue/master/lib/queue.h
 * 
 * Ref:
 * https://stackoverflow.com/questions/25432371/how-can-one-emulate-the-c-namespace-feature-in-a-c-code
 */
typedef struct q_head q_head;

typedef struct {
  q_head* (* const q_init)();
  void (* const q_free)(q_head *handle);
  void (* const put)(q_head *handle, void *elem);
  void* (* const get)(q_head *handle);
} _queue;

extern _queue const queue;

#endif /* _QUEUE_H */
