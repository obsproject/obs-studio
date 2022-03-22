/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdlib.h>
#include "jansson.h"

struct hashtable_list {
    struct hashtable_list *prev;
    struct hashtable_list *next;
};

/* "pair" may be a bit confusing a name, but think of it as a
   key-value pair. In this case, it just encodes some extra data,
   too */
struct hashtable_pair {
    struct hashtable_list list;
    struct hashtable_list ordered_list;
    size_t hash;
    json_t *value;
    char key[1];
};

struct hashtable_bucket {
    struct hashtable_list *first;
    struct hashtable_list *last;
};

typedef struct hashtable {
    size_t size;
    struct hashtable_bucket *buckets;
    size_t order;  /* hashtable has pow(2, order) buckets */
    struct hashtable_list list;
    struct hashtable_list ordered_list;
} hashtable_t;


#define hashtable_key_to_iter(key_) \
    (&(container_of(key_, struct hashtable_pair, key)->ordered_list))


/**
 * hashtable_init - Initialize a hashtable object
 *
 * @hashtable: The (statically allocated) hashtable object
 *
 * Initializes a statically allocated hashtable object. The object
 * should be cleared with hashtable_close when it's no longer used.
 *
 * Returns 0 on success, -1 on error (out of memory).
 */
int hashtable_init(hashtable_t *hashtable);

/**
 * hashtable_close - Release all resources used by a hashtable object
 *
 * @hashtable: The hashtable
 *
 * Destroys a statically allocated hashtable object.
 */
void hashtable_close(hashtable_t *hashtable);

/**
 * hashtable_set - Add/modify value in hashtable
 *
 * @hashtable: The hashtable object
 * @key: The key
 * @serial: For addition order of keys
 * @value: The value
 *
 * If a value with the given key already exists, its value is replaced
 * with the new value. Value is "stealed" in the sense that hashtable
 * doesn't increment its refcount but decreases the refcount when the
 * value is no longer needed.
 *
 * Returns 0 on success, -1 on failure (out of memory).
 */
int hashtable_set(hashtable_t *hashtable, const char *key, json_t *value);

/**
 * hashtable_get - Get a value associated with a key
 *
 * @hashtable: The hashtable object
 * @key: The key
 *
 * Returns value if it is found, or NULL otherwise.
 */
void *hashtable_get(hashtable_t *hashtable, const char *key);

/**
 * hashtable_del - Remove a value from the hashtable
 *
 * @hashtable: The hashtable object
 * @key: The key
 *
 * Returns 0 on success, or -1 if the key was not found.
 */
int hashtable_del(hashtable_t *hashtable, const char *key);

/**
 * hashtable_clear - Clear hashtable
 *
 * @hashtable: The hashtable object
 *
 * Removes all items from the hashtable.
 */
void hashtable_clear(hashtable_t *hashtable);

/**
 * hashtable_iter - Iterate over hashtable
 *
 * @hashtable: The hashtable object
 *
 * Returns an opaque iterator to the first element in the hashtable.
 * The iterator should be passed to hashtable_iter_* functions.
 * The hashtable items are not iterated over in any particular order.
 *
 * There's no need to free the iterator in any way. The iterator is
 * valid as long as the item that is referenced by the iterator is not
 * deleted. Other values may be added or deleted. In particular,
 * hashtable_iter_next() may be called on an iterator, and after that
 * the key/value pair pointed by the old iterator may be deleted.
 */
void *hashtable_iter(hashtable_t *hashtable);

/**
 * hashtable_iter_at - Return an iterator at a specific key
 *
 * @hashtable: The hashtable object
 * @key: The key that the iterator should point to
 *
 * Like hashtable_iter() but returns an iterator pointing to a
 * specific key.
 */
void *hashtable_iter_at(hashtable_t *hashtable, const char *key);

/**
 * hashtable_iter_next - Advance an iterator
 *
 * @hashtable: The hashtable object
 * @iter: The iterator
 *
 * Returns a new iterator pointing to the next element in the
 * hashtable or NULL if the whole hastable has been iterated over.
 */
void *hashtable_iter_next(hashtable_t *hashtable, void *iter);

/**
 * hashtable_iter_key - Retrieve the key pointed by an iterator
 *
 * @iter: The iterator
 */
void *hashtable_iter_key(void *iter);

/**
 * hashtable_iter_value - Retrieve the value pointed by an iterator
 *
 * @iter: The iterator
 */
void *hashtable_iter_value(void *iter);

/**
 * hashtable_iter_set - Set the value pointed by an iterator
 *
 * @iter: The iterator
 * @value: The value to set
 */
void hashtable_iter_set(void *iter, json_t *value);

#endif
