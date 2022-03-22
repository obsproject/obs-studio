/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#include <jansson_private_config.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "jansson.h"
#include "hashtable.h"
#include "jansson_private.h"
#include "utf.h"

/* Work around nonstandard isnan() and isinf() implementations */
#ifndef isnan
#ifndef __sun
static JSON_INLINE int isnan(double x) { return x != x; }
#endif
#endif
#ifndef isinf
static JSON_INLINE int isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

static JSON_INLINE void json_init(json_t *json, json_type type)
{
    json->type = type;
    json->refcount = 1;
}


/*** object ***/

extern volatile uint32_t hashtable_seed;

json_t *json_object(void)
{
    json_object_t *object = jsonp_malloc(sizeof(json_object_t));
    if(!object)
        return NULL;

    if (!hashtable_seed) {
        /* Autoseed */
        json_object_seed(0);
    }

    json_init(&object->json, JSON_OBJECT);

    if(hashtable_init(&object->hashtable))
    {
        jsonp_free(object);
        return NULL;
    }

    object->visited = 0;

    return &object->json;
}

static void json_delete_object(json_object_t *object)
{
    hashtable_close(&object->hashtable);
    jsonp_free(object);
}

size_t json_object_size(const json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return 0;

    object = json_to_object(json);
    return object->hashtable.size;
}

json_t *json_object_get(const json_t *json, const char *key)
{
    json_object_t *object;

    if(!key || !json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_get(&object->hashtable, key);
}

int json_object_set_new_nocheck(json_t *json, const char *key, json_t *value)
{
    json_object_t *object;

    if(!value)
        return -1;

    if(!key || !json_is_object(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    object = json_to_object(json);

    if(hashtable_set(&object->hashtable, key, value))
    {
        json_decref(value);
        return -1;
    }

    return 0;
}

int json_object_set_new(json_t *json, const char *key, json_t *value)
{
    if(!key || !utf8_check_string(key, strlen(key)))
    {
        json_decref(value);
        return -1;
    }

    return json_object_set_new_nocheck(json, key, value);
}

int json_object_del(json_t *json, const char *key)
{
    json_object_t *object;

    if(!key || !json_is_object(json))
        return -1;

    object = json_to_object(json);
    return hashtable_del(&object->hashtable, key);
}

int json_object_clear(json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    hashtable_clear(&object->hashtable);

    return 0;
}

int json_object_update(json_t *object, json_t *other)
{
    const char *key;
    json_t *value;

    if(!json_is_object(object) || !json_is_object(other))
        return -1;

    json_object_foreach(other, key, value) {
        if(json_object_set_nocheck(object, key, value))
            return -1;
    }

    return 0;
}

int json_object_update_existing(json_t *object, json_t *other)
{
    const char *key;
    json_t *value;

    if(!json_is_object(object) || !json_is_object(other))
        return -1;

    json_object_foreach(other, key, value) {
        if(json_object_get(object, key))
            json_object_set_nocheck(object, key, value);
    }

    return 0;
}

int json_object_update_missing(json_t *object, json_t *other)
{
    const char *key;
    json_t *value;

    if(!json_is_object(object) || !json_is_object(other))
        return -1;

    json_object_foreach(other, key, value) {
        if(!json_object_get(object, key))
            json_object_set_nocheck(object, key, value);
    }

    return 0;
}

void *json_object_iter(json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_iter(&object->hashtable);
}

void *json_object_iter_at(json_t *json, const char *key)
{
    json_object_t *object;

    if(!key || !json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_iter_at(&object->hashtable, key);
}

void *json_object_iter_next(json_t *json, void *iter)
{
    json_object_t *object;

    if(!json_is_object(json) || iter == NULL)
        return NULL;

    object = json_to_object(json);
    return hashtable_iter_next(&object->hashtable, iter);
}

const char *json_object_iter_key(void *iter)
{
    if(!iter)
        return NULL;

    return hashtable_iter_key(iter);
}

json_t *json_object_iter_value(void *iter)
{
    if(!iter)
        return NULL;

    return (json_t *)hashtable_iter_value(iter);
}

int json_object_iter_set_new(json_t *json, void *iter, json_t *value)
{
    if(!json_is_object(json) || !iter || !value)
        return -1;

    hashtable_iter_set(iter, value);
    return 0;
}

void *json_object_key_to_iter(const char *key)
{
    if(!key)
        return NULL;

    return hashtable_key_to_iter(key);
}

static int json_object_equal(json_t *object1, json_t *object2)
{
    const char *key;
    json_t *value1, *value2;

    if(json_object_size(object1) != json_object_size(object2))
        return 0;

    json_object_foreach(object1, key, value1) {
        value2 = json_object_get(object2, key);

        if(!json_equal(value1, value2))
            return 0;
    }

    return 1;
}

static json_t *json_object_copy(json_t *object)
{
    json_t *result;

    const char *key;
    json_t *value;

    result = json_object();
    if(!result)
        return NULL;

    json_object_foreach(object, key, value)
        json_object_set_nocheck(result, key, value);

    return result;
}

static json_t *json_object_deep_copy(const json_t *object)
{
    json_t *result;
    void *iter;

    result = json_object();
    if(!result)
        return NULL;

    /* Cannot use json_object_foreach because object has to be cast
       non-const */
    iter = json_object_iter((json_t *)object);
    while(iter) {
        const char *key;
        const json_t *value;
        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);

        json_object_set_new_nocheck(result, key, json_deep_copy(value));
        iter = json_object_iter_next((json_t *)object, iter);
    }

    return result;
}


/*** array ***/

json_t *json_array(void)
{
    json_array_t *array = jsonp_malloc(sizeof(json_array_t));
    if(!array)
        return NULL;
    json_init(&array->json, JSON_ARRAY);

    array->entries = 0;
    array->size = 8;

    array->table = jsonp_malloc(array->size * sizeof(json_t *));
    if(!array->table) {
        jsonp_free(array);
        return NULL;
    }

    array->visited = 0;

    return &array->json;
}

static void json_delete_array(json_array_t *array)
{
    size_t i;

    for(i = 0; i < array->entries; i++)
        json_decref(array->table[i]);

    jsonp_free(array->table);
    jsonp_free(array);
}

size_t json_array_size(const json_t *json)
{
    if(!json_is_array(json))
        return 0;

    return json_to_array(json)->entries;
}

json_t *json_array_get(const json_t *json, size_t index)
{
    json_array_t *array;
    if(!json_is_array(json))
        return NULL;
    array = json_to_array(json);

    if(index >= array->entries)
        return NULL;

    return array->table[index];
}

int json_array_set_new(json_t *json, size_t index, json_t *value)
{
    json_array_t *array;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(index >= array->entries)
    {
        json_decref(value);
        return -1;
    }

    json_decref(array->table[index]);
    array->table[index] = value;

    return 0;
}

static void array_move(json_array_t *array, size_t dest,
                       size_t src, size_t count)
{
    memmove(&array->table[dest], &array->table[src], count * sizeof(json_t *));
}

static void array_copy(json_t **dest, size_t dpos,
                       json_t **src, size_t spos,
                       size_t count)
{
    memcpy(&dest[dpos], &src[spos], count * sizeof(json_t *));
}

static json_t **json_array_grow(json_array_t *array,
                                size_t amount,
                                int copy)
{
    size_t new_size;
    json_t **old_table, **new_table;

    if(array->entries + amount <= array->size)
        return array->table;

    old_table = array->table;

    new_size = max(array->size + amount, array->size * 2);
    new_table = jsonp_malloc(new_size * sizeof(json_t *));
    if(!new_table)
        return NULL;

    array->size = new_size;
    array->table = new_table;

    if(copy) {
        array_copy(array->table, 0, old_table, 0, array->entries);
        jsonp_free(old_table);
        return array->table;
    }

    return old_table;
}

int json_array_append_new(json_t *json, json_t *value)
{
    json_array_t *array;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(!json_array_grow(array, 1, 1)) {
        json_decref(value);
        return -1;
    }

    array->table[array->entries] = value;
    array->entries++;

    return 0;
}

int json_array_insert_new(json_t *json, size_t index, json_t *value)
{
    json_array_t *array;
    json_t **old_table;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value) {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(index > array->entries) {
        json_decref(value);
        return -1;
    }

    old_table = json_array_grow(array, 1, 0);
    if(!old_table) {
        json_decref(value);
        return -1;
    }

    if(old_table != array->table) {
        array_copy(array->table, 0, old_table, 0, index);
        array_copy(array->table, index + 1, old_table, index,
                   array->entries - index);
        jsonp_free(old_table);
    }
    else
        array_move(array, index + 1, index, array->entries - index);

    array->table[index] = value;
    array->entries++;

    return 0;
}

int json_array_remove(json_t *json, size_t index)
{
    json_array_t *array;

    if(!json_is_array(json))
        return -1;
    array = json_to_array(json);

    if(index >= array->entries)
        return -1;

    json_decref(array->table[index]);

    /* If we're removing the last element, nothing has to be moved */
    if(index < array->entries - 1)
        array_move(array, index, index + 1, array->entries - index - 1);

    array->entries--;

    return 0;
}

int json_array_clear(json_t *json)
{
    json_array_t *array;
    size_t i;

    if(!json_is_array(json))
        return -1;
    array = json_to_array(json);

    for(i = 0; i < array->entries; i++)
        json_decref(array->table[i]);

    array->entries = 0;
    return 0;
}

int json_array_extend(json_t *json, json_t *other_json)
{
    json_array_t *array, *other;
    size_t i;

    if(!json_is_array(json) || !json_is_array(other_json))
        return -1;
    array = json_to_array(json);
    other = json_to_array(other_json);

    if(!json_array_grow(array, other->entries, 1))
        return -1;

    for(i = 0; i < other->entries; i++)
        json_incref(other->table[i]);

    array_copy(array->table, array->entries, other->table, 0, other->entries);

    array->entries += other->entries;
    return 0;
}

static int json_array_equal(json_t *array1, json_t *array2)
{
    size_t i, size;

    size = json_array_size(array1);
    if(size != json_array_size(array2))
        return 0;

    for(i = 0; i < size; i++)
    {
        json_t *value1, *value2;

        value1 = json_array_get(array1, i);
        value2 = json_array_get(array2, i);

        if(!json_equal(value1, value2))
            return 0;
    }

    return 1;
}

static json_t *json_array_copy(json_t *array)
{
    json_t *result;
    size_t i;

    result = json_array();
    if(!result)
        return NULL;

    for(i = 0; i < json_array_size(array); i++)
        json_array_append(result, json_array_get(array, i));

    return result;
}

static json_t *json_array_deep_copy(const json_t *array)
{
    json_t *result;
    size_t i;

    result = json_array();
    if(!result)
        return NULL;

    for(i = 0; i < json_array_size(array); i++)
        json_array_append_new(result, json_deep_copy(json_array_get(array, i)));

    return result;
}

/*** string ***/

static json_t *string_create(const char *value, size_t len, int own)
{
    char *v;
    json_string_t *string;

    if(!value)
        return NULL;

    if(own)
        v = (char *)value;
    else {
        v = jsonp_strndup(value, len);
        if(!v)
            return NULL;
    }

    string = jsonp_malloc(sizeof(json_string_t));
    if(!string) {
        if(!own)
            jsonp_free(v);
        return NULL;
    }
    json_init(&string->json, JSON_STRING);
    string->value = v;
    string->length = len;

    return &string->json;
}

json_t *json_string_nocheck(const char *value)
{
    if(!value)
        return NULL;

    return string_create(value, strlen(value), 0);
}

json_t *json_stringn_nocheck(const char *value, size_t len)
{
    return string_create(value, len, 0);
}

/* this is private; "steal" is not a public API concept */
json_t *jsonp_stringn_nocheck_own(const char *value, size_t len)
{
    return string_create(value, len, 1);
}

json_t *json_string(const char *value)
{
    if(!value)
        return NULL;

    return json_stringn(value, strlen(value));
}

json_t *json_stringn(const char *value, size_t len)
{
    if(!value || !utf8_check_string(value, len))
        return NULL;

    return json_stringn_nocheck(value, len);
}

const char *json_string_value(const json_t *json)
{
    if(!json_is_string(json))
        return NULL;

    return json_to_string(json)->value;
}

size_t json_string_length(const json_t *json)
{
    if(!json_is_string(json))
        return 0;

    return json_to_string(json)->length;
}

int json_string_set_nocheck(json_t *json, const char *value)
{
    if(!value)
        return -1;

    return json_string_setn_nocheck(json, value, strlen(value));
}

int json_string_setn_nocheck(json_t *json, const char *value, size_t len)
{
    char *dup;
    json_string_t *string;

    if(!json_is_string(json) || !value)
        return -1;

    dup = jsonp_strndup(value, len);
    if(!dup)
        return -1;

    string = json_to_string(json);
    jsonp_free(string->value);
    string->value = dup;
    string->length = len;

    return 0;
}

int json_string_set(json_t *json, const char *value)
{
    if(!value)
        return -1;

    return json_string_setn(json, value, strlen(value));
}

int json_string_setn(json_t *json, const char *value, size_t len)
{
    if(!value || !utf8_check_string(value, len))
        return -1;

    return json_string_setn_nocheck(json, value, len);
}

static void json_delete_string(json_string_t *string)
{
    jsonp_free(string->value);
    jsonp_free(string);
}

static int json_string_equal(json_t *string1, json_t *string2)
{
    json_string_t *s1, *s2;

    if(!json_is_string(string1) || !json_is_string(string2))
        return 0;

    s1 = json_to_string(string1);
    s2 = json_to_string(string2);
    return s1->length == s2->length && !memcmp(s1->value, s2->value, s1->length);
}

static json_t *json_string_copy(const json_t *string)
{
    json_string_t *s;

    if(!json_is_string(string))
        return NULL;

    s = json_to_string(string);
    return json_stringn_nocheck(s->value, s->length);
}


/*** integer ***/

json_t *json_integer(json_int_t value)
{
    json_integer_t *integer = jsonp_malloc(sizeof(json_integer_t));
    if(!integer)
        return NULL;
    json_init(&integer->json, JSON_INTEGER);

    integer->value = value;
    return &integer->json;
}

json_int_t json_integer_value(const json_t *json)
{
    if(!json_is_integer(json))
        return 0;

    return json_to_integer(json)->value;
}

int json_integer_set(json_t *json, json_int_t value)
{
    if(!json_is_integer(json))
        return -1;

    json_to_integer(json)->value = value;

    return 0;
}

static void json_delete_integer(json_integer_t *integer)
{
    jsonp_free(integer);
}

static int json_integer_equal(json_t *integer1, json_t *integer2)
{
    return json_integer_value(integer1) == json_integer_value(integer2);
}

static json_t *json_integer_copy(const json_t *integer)
{
    return json_integer(json_integer_value(integer));
}


/*** real ***/

json_t *json_real(double value)
{
    json_real_t *real;

    if(isnan(value) || isinf(value))
        return NULL;

    real = jsonp_malloc(sizeof(json_real_t));
    if(!real)
        return NULL;
    json_init(&real->json, JSON_REAL);

    real->value = value;
    return &real->json;
}

double json_real_value(const json_t *json)
{
    if(!json_is_real(json))
        return 0;

    return json_to_real(json)->value;
}

int json_real_set(json_t *json, double value)
{
    if(!json_is_real(json) || isnan(value) || isinf(value))
        return -1;

    json_to_real(json)->value = value;

    return 0;
}

static void json_delete_real(json_real_t *real)
{
    jsonp_free(real);
}

static int json_real_equal(json_t *real1, json_t *real2)
{
    return json_real_value(real1) == json_real_value(real2);
}

static json_t *json_real_copy(const json_t *real)
{
    return json_real(json_real_value(real));
}


/*** number ***/

double json_number_value(const json_t *json)
{
    if(json_is_integer(json))
        return (double)json_integer_value(json);
    else if(json_is_real(json))
        return json_real_value(json);
    else
        return 0.0;
}


/*** simple values ***/

json_t *json_true(void)
{
    static json_t the_true = {JSON_TRUE, (size_t)-1};
    return &the_true;
}


json_t *json_false(void)
{
    static json_t the_false = {JSON_FALSE, (size_t)-1};
    return &the_false;
}


json_t *json_null(void)
{
    static json_t the_null = {JSON_NULL, (size_t)-1};
    return &the_null;
}


/*** deletion ***/

void json_delete(json_t *json)
{
    if (!json)
        return;

    switch(json_typeof(json)) {
        case JSON_OBJECT:
            json_delete_object(json_to_object(json));
            break;
        case JSON_ARRAY:
            json_delete_array(json_to_array(json));
            break;
        case JSON_STRING:
            json_delete_string(json_to_string(json));
            break;
        case JSON_INTEGER:
            json_delete_integer(json_to_integer(json));
            break;
        case JSON_REAL:
            json_delete_real(json_to_real(json));
            break;
        default:
            return;
    }

    /* json_delete is not called for true, false or null */
}


/*** equality ***/

int json_equal(json_t *json1, json_t *json2)
{
    if(!json1 || !json2)
        return 0;

    if(json_typeof(json1) != json_typeof(json2))
        return 0;

    /* this covers true, false and null as they are singletons */
    if(json1 == json2)
        return 1;

    switch(json_typeof(json1)) {
        case JSON_OBJECT:
            return json_object_equal(json1, json2);
        case JSON_ARRAY:
            return json_array_equal(json1, json2);
        case JSON_STRING:
            return json_string_equal(json1, json2);
        case JSON_INTEGER:
            return json_integer_equal(json1, json2);
        case JSON_REAL:
            return json_real_equal(json1, json2);
        default:
            return 0;
    }
}


/*** copying ***/

json_t *json_copy(json_t *json)
{
    if(!json)
        return NULL;

    switch(json_typeof(json)) {
        case JSON_OBJECT:
            return json_object_copy(json);
        case JSON_ARRAY:
            return json_array_copy(json);
        case JSON_STRING:
            return json_string_copy(json);
        case JSON_INTEGER:
            return json_integer_copy(json);
        case JSON_REAL:
            return json_real_copy(json);
        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_NULL:
            return json;
        default:
            return NULL;
    }

    return NULL;
}

json_t *json_deep_copy(const json_t *json)
{
    if(!json)
        return NULL;

    switch(json_typeof(json)) {
        case JSON_OBJECT:
            return json_object_deep_copy(json);
        case JSON_ARRAY:
            return json_array_deep_copy(json);
            /* for the rest of the types, deep copying doesn't differ from
               shallow copying */
        case JSON_STRING:
            return json_string_copy(json);
        case JSON_INTEGER:
            return json_integer_copy(json);
        case JSON_REAL:
            return json_real_copy(json);
        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_NULL:
            return (json_t *)json;
        default:
            return NULL;
    }

    return NULL;
}
