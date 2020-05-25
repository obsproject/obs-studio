#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

typedef struct queue_element {
    void *next;
    void *value;
} queue_element;

struct q_head {
    queue_element *head;
    queue_element *tail;
    pthread_mutex_t *mutex;
};

/* API */
static q_head *q_init();
static void q_free();
static void put(q_head *header, void *elem);
static void *get(q_head *header);

/* Implementation */
static q_head *q_init() {
    q_head *handle = malloc(sizeof(*handle));
    handle->head = NULL;
    handle->tail = NULL;
    
    pthread_mutex_t *mutex = malloc(sizeof(*mutex));
    handle->mutex = mutex;
    return handle;
}

static void q_free(q_head *header) {
    free(header->mutex);
    free(header);
    header = NULL;
}

static void put(q_head *header, void *elem) {
    // Create new element
    queue_element *element = malloc(sizeof(*element));
    element->value = elem;
    element->next = NULL;
    
    pthread_mutex_lock(header->mutex);
    // Is list empty
    if (header->head == NULL) {
        header->head = element;
        header->tail = element;
    } else {
        // Rewire
        queue_element *old_tail = header->tail;
        old_tail->next = element;
        header->tail = element;
    }
    pthread_mutex_unlock(header->mutex);
}

static void* get(q_head *header) {
    pthread_mutex_lock(header->mutex);
    queue_element *head = header->head;
    
    if (head == NULL) { /* is empty? */
        pthread_mutex_unlock(header->mutex);
        return NULL;
    } else {
        header->head = head->next; /* Rewire the linked list */
        void *value = head->value;
        free(head);
        pthread_mutex_unlock(header->mutex);
        return value;
    }
}

_queue const queue = {
    q_init,
    q_free,
    put,
    get
};

