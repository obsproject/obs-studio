/*
 * Copyright (c) 2009-2013 Petri Lehtinen <petri@digip.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include <jansson_config.h>   /* for JSON_INLINE */
#include "jansson_private.h"  /* for container_of() */
#include "hashtable.h"

typedef struct hashtable_list list_t;
typedef struct hashtable_pair pair_t;
typedef struct hashtable_bucket bucket_t;

#define list_to_pair(list_)  container_of(list_, pair_t, list)

/* From http://www.cse.yorku.ca/~oz/hash.html */
static size_t hash_str(const void *ptr)
{
    const char *str = (const char *)ptr;

    size_t hash = 5381;
    size_t c;

    while((c = (size_t)*str))
    {
        hash = ((hash << 5) + hash) + c;
        str++;
    }

    return hash;
}

static JSON_INLINE void list_init(list_t *list)
{
    list->next = list;
    list->prev = list;
}

static JSON_INLINE void list_insert(list_t *list, list_t *node)
{
    node->next = list;
    node->prev = list->prev;
    list->prev->next = node;
    list->prev = node;
}

static JSON_INLINE void list_remove(list_t *list)
{
    list->prev->next = list->next;
    list->next->prev = list->prev;
}

static JSON_INLINE int bucket_is_empty(hashtable_t *hashtable, bucket_t *bucket)
{
    return bucket->first == &hashtable->list && bucket->first == bucket->last;
}

static void insert_to_bucket(hashtable_t *hashtable, bucket_t *bucket,
                             list_t *list)
{
    if(bucket_is_empty(hashtable, bucket))
    {
        list_insert(&hashtable->list, list);
        bucket->first = bucket->last = list;
    }
    else
    {
        list_insert(bucket->first, list);
        bucket->first = list;
    }
}

static const size_t primes[] = {
    5, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
    49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
    12582917, 25165843, 50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};

static JSON_INLINE size_t num_buckets(hashtable_t *hashtable)
{
    return primes[hashtable->num_buckets];
}


static pair_t *hashtable_find_pair(hashtable_t *hashtable, bucket_t *bucket,
                                   const char *key, size_t hash)
{
    list_t *list;
    pair_t *pair;

    if(bucket_is_empty(hashtable, bucket))
        return NULL;

    list = bucket->first;
    while(1)
    {
        pair = list_to_pair(list);
        if(pair->hash == hash && strcmp(pair->key, key) == 0)
            return pair;

        if(list == bucket->last)
            break;

        list = list->next;
    }

    return NULL;
}

/* returns 0 on success, -1 if key was not found */
static int hashtable_do_del(hashtable_t *hashtable,
                            const char *key, size_t hash)
{
    pair_t *pair;
    bucket_t *bucket;
    size_t index;

    index = hash % num_buckets(hashtable);
    bucket = &hashtable->buckets[index];

    pair = hashtable_find_pair(hashtable, bucket, key, hash);
    if(!pair)
        return -1;

    if(&pair->list == bucket->first && &pair->list == bucket->last)
        bucket->first = bucket->last = &hashtable->list;

    else if(&pair->list == bucket->first)
        bucket->first = pair->list.next;

    else if(&pair->list == bucket->last)
        bucket->last = pair->list.prev;

    list_remove(&pair->list);
    json_decref(pair->value);

    jsonp_free(pair);
    hashtable->size--;

    return 0;
}

static void hashtable_do_clear(hashtable_t *hashtable)
{
    list_t *list, *next;
    pair_t *pair;

    for(list = hashtable->list.next; list != &hashtable->list; list = next)
    {
        next = list->next;
        pair = list_to_pair(list);
        json_decref(pair->value);
        jsonp_free(pair);
    }
}

static int hashtable_do_rehash(hashtable_t *hashtable)
{
    list_t *list, *next;
    pair_t *pair;
    size_t i, index, new_size;

    jsonp_free(hashtable->buckets);

    hashtable->num_buckets++;
    new_size = num_buckets(hashtable);

    hashtable->buckets = jsonp_malloc(new_size * sizeof(bucket_t));
    if(!hashtable->buckets)
        return -1;

    for(i = 0; i < num_buckets(hashtable); i++)
    {
        hashtable->buckets[i].first = hashtable->buckets[i].last =
            &hashtable->list;
    }

    list = hashtable->list.next;
    list_init(&hashtable->list);

    for(; list != &hashtable->list; list = next) {
        next = list->next;
        pair = list_to_pair(list);
        index = pair->hash % new_size;
        insert_to_bucket(hashtable, &hashtable->buckets[index], &pair->list);
    }

    return 0;
}


int hashtable_init(hashtable_t *hashtable)
{
    size_t i;

    hashtable->size = 0;
    hashtable->num_buckets = 0;  /* index to primes[] */
    hashtable->buckets = jsonp_malloc(num_buckets(hashtable) * sizeof(bucket_t));
    if(!hashtable->buckets)
        return -1;

    list_init(&hashtable->list);

    for(i = 0; i < num_buckets(hashtable); i++)
    {
        hashtable->buckets[i].first = hashtable->buckets[i].last =
            &hashtable->list;
    }

    return 0;
}

void hashtable_close(hashtable_t *hashtable)
{
    hashtable_do_clear(hashtable);
    jsonp_free(hashtable->buckets);
}

int hashtable_set(hashtable_t *hashtable,
                  const char *key, size_t serial,
                  json_t *value)
{
    pair_t *pair;
    bucket_t *bucket;
    size_t hash, index;

    /* rehash if the load ratio exceeds 1 */
    if(hashtable->size >= num_buckets(hashtable))
        if(hashtable_do_rehash(hashtable))
            return -1;

    hash = hash_str(key);
    index = hash % num_buckets(hashtable);
    bucket = &hashtable->buckets[index];
    pair = hashtable_find_pair(hashtable, bucket, key, hash);

    if(pair)
    {
        json_decref(pair->value);
        pair->value = value;
    }
    else
    {
        /* offsetof(...) returns the size of pair_t without the last,
           flexible member. This way, the correct amount is
           allocated. */

        size_t len = strlen(key);
        if(len >= (size_t)-1 - offsetof(pair_t, key)) {
            /* Avoid an overflow if the key is very long */
            return -1;
        }

        pair = jsonp_malloc(offsetof(pair_t, key) + len + 1);
        if(!pair)
            return -1;

        pair->hash = hash;
        pair->serial = serial;
        strcpy(pair->key, key);
        pair->value = value;
        list_init(&pair->list);

        insert_to_bucket(hashtable, bucket, &pair->list);

        hashtable->size++;
    }
    return 0;
}

void *hashtable_get(hashtable_t *hashtable, const char *key)
{
    pair_t *pair;
    size_t hash;
    bucket_t *bucket;

    hash = hash_str(key);
    bucket = &hashtable->buckets[hash % num_buckets(hashtable)];

    pair = hashtable_find_pair(hashtable, bucket, key, hash);
    if(!pair)
        return NULL;

    return pair->value;
}

int hashtable_del(hashtable_t *hashtable, const char *key)
{
    size_t hash = hash_str(key);
    return hashtable_do_del(hashtable, key, hash);
}

void hashtable_clear(hashtable_t *hashtable)
{
    size_t i;

    hashtable_do_clear(hashtable);

    for(i = 0; i < num_buckets(hashtable); i++)
    {
        hashtable->buckets[i].first = hashtable->buckets[i].last =
            &hashtable->list;
    }

    list_init(&hashtable->list);
    hashtable->size = 0;
}

void *hashtable_iter(hashtable_t *hashtable)
{
    return hashtable_iter_next(hashtable, &hashtable->list);
}

void *hashtable_iter_at(hashtable_t *hashtable, const char *key)
{
    pair_t *pair;
    size_t hash;
    bucket_t *bucket;

    hash = hash_str(key);
    bucket = &hashtable->buckets[hash % num_buckets(hashtable)];

    pair = hashtable_find_pair(hashtable, bucket, key, hash);
    if(!pair)
        return NULL;

    return &pair->list;
}

void *hashtable_iter_next(hashtable_t *hashtable, void *iter)
{
    list_t *list = (list_t *)iter;
    if(list->next == &hashtable->list)
        return NULL;
    return list->next;
}

void *hashtable_iter_key(void *iter)
{
    pair_t *pair = list_to_pair((list_t *)iter);
    return pair->key;
}

size_t hashtable_iter_serial(void *iter)
{
    pair_t *pair = list_to_pair((list_t *)iter);
    return pair->serial;
}

void *hashtable_iter_value(void *iter)
{
    pair_t *pair = list_to_pair((list_t *)iter);
    return pair->value;
}

void hashtable_iter_set(void *iter, json_t *value)
{
    pair_t *pair = list_to_pair((list_t *)iter);

    json_decref(pair->value);
    pair->value = value;
}
