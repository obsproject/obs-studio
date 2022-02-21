/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <string.h>
#include <jansson.h>
#include "util.h"

static void test_copy_simple(void)
{
    json_t *value, *copy;

    if(json_copy(NULL))
        fail("copying NULL doesn't return NULL");

    /* true */
    value = json_true();
    copy = json_copy(value);
    if(value != copy)
        fail("copying true failed");
    json_decref(value);
    json_decref(copy);

    /* false */
    value = json_false();
    copy = json_copy(value);
    if(value != copy)
        fail("copying false failed");
    json_decref(value);
    json_decref(copy);

    /* null */
    value = json_null();
    copy = json_copy(value);
    if(value != copy)
        fail("copying null failed");
    json_decref(value);
    json_decref(copy);

    /* string */
    value = json_string("foo");
    if(!value)
        fail("unable to create a string");
    copy = json_copy(value);
    if(!copy)
        fail("unable to copy a string");
    if(copy == value)
        fail("copying a string doesn't copy");
    if(!json_equal(copy, value))
        fail("copying a string produces an inequal copy");
    if(value->refcount != 1 || copy->refcount != 1)
        fail("invalid refcounts");
    json_decref(value);
    json_decref(copy);

    /* integer */
    value = json_integer(543);
    if(!value)
        fail("unable to create an integer");
    copy = json_copy(value);
    if(!copy)
        fail("unable to copy an integer");
    if(copy == value)
        fail("copying an integer doesn't copy");
    if(!json_equal(copy, value))
        fail("copying an integer produces an inequal copy");
    if(value->refcount != 1 || copy->refcount != 1)
        fail("invalid refcounts");
    json_decref(value);
    json_decref(copy);

    /* real */
    value = json_real(123e9);
    if(!value)
        fail("unable to create a real");
    copy = json_copy(value);
    if(!copy)
        fail("unable to copy a real");
    if(copy == value)
        fail("copying a real doesn't copy");
    if(!json_equal(copy, value))
        fail("copying a real produces an inequal copy");
    if(value->refcount != 1 || copy->refcount != 1)
        fail("invalid refcounts");
    json_decref(value);
    json_decref(copy);
}

static void test_deep_copy_simple(void)
{
    json_t *value, *copy;

    if(json_deep_copy(NULL))
        fail("deep copying NULL doesn't return NULL");

    /* true */
    value = json_true();
    copy = json_deep_copy(value);
    if(value != copy)
        fail("deep copying true failed");
    json_decref(value);
    json_decref(copy);

    /* false */
    value = json_false();
    copy = json_deep_copy(value);
    if(value != copy)
        fail("deep copying false failed");
    json_decref(value);
    json_decref(copy);

    /* null */
    value = json_null();
    copy = json_deep_copy(value);
    if(value != copy)
        fail("deep copying null failed");
    json_decref(value);
    json_decref(copy);

    /* string */
    value = json_string("foo");
    if(!value)
        fail("unable to create a string");
    copy = json_deep_copy(value);
    if(!copy)
        fail("unable to deep copy a string");
    if(copy == value)
        fail("deep copying a string doesn't copy");
    if(!json_equal(copy, value))
        fail("deep copying a string produces an inequal copy");
    if(value->refcount != 1 || copy->refcount != 1)
        fail("invalid refcounts");
    json_decref(value);
    json_decref(copy);

    /* integer */
    value = json_integer(543);
    if(!value)
        fail("unable to create an integer");
    copy = json_deep_copy(value);
    if(!copy)
        fail("unable to deep copy an integer");
    if(copy == value)
        fail("deep copying an integer doesn't copy");
    if(!json_equal(copy, value))
        fail("deep copying an integer produces an inequal copy");
    if(value->refcount != 1 || copy->refcount != 1)
        fail("invalid refcounts");
    json_decref(value);
    json_decref(copy);

    /* real */
    value = json_real(123e9);
    if(!value)
        fail("unable to create a real");
    copy = json_deep_copy(value);
    if(!copy)
        fail("unable to deep copy a real");
    if(copy == value)
        fail("deep copying a real doesn't copy");
    if(!json_equal(copy, value))
        fail("deep copying a real produces an inequal copy");
    if(value->refcount != 1 || copy->refcount != 1)
        fail("invalid refcounts");
    json_decref(value);
    json_decref(copy);
}

static void test_copy_array(void)
{
    const char *json_array_text = "[1, \"foo\", 3.141592, {\"foo\": \"bar\"}]";

    json_t *array, *copy;
    size_t i;

    array = json_loads(json_array_text, 0, NULL);
    if(!array)
        fail("unable to parse an array");

    copy = json_copy(array);
    if(!copy)
        fail("unable to copy an array");
    if(copy == array)
        fail("copying an array doesn't copy");
    if(!json_equal(copy, array))
        fail("copying an array produces an inequal copy");

    for(i = 0; i < json_array_size(copy); i++)
    {
        if(json_array_get(array, i) != json_array_get(copy, i))
            fail("copying an array modifies its elements");
    }

    json_decref(array);
    json_decref(copy);
}

static void test_deep_copy_array(void)
{
    const char *json_array_text = "[1, \"foo\", 3.141592, {\"foo\": \"bar\"}]";

    json_t *array, *copy;
    size_t i;

    array = json_loads(json_array_text, 0, NULL);
    if(!array)
        fail("unable to parse an array");

    copy = json_deep_copy(array);
    if(!copy)
        fail("unable to deep copy an array");
    if(copy == array)
        fail("deep copying an array doesn't copy");
    if(!json_equal(copy, array))
        fail("deep copying an array produces an inequal copy");

    for(i = 0; i < json_array_size(copy); i++)
    {
        if(json_array_get(array, i) == json_array_get(copy, i))
            fail("deep copying an array doesn't copy its elements");
    }

    json_decref(array);
    json_decref(copy);
}

static void test_copy_object(void)
{
    const char *json_object_text =
        "{\"foo\": \"bar\", \"a\": 1, \"b\": 3.141592, \"c\": [1,2,3,4]}";

    const char *keys[] = {"foo", "a", "b", "c"};
    int i;

    json_t *object, *copy;
    void *iter;

    object = json_loads(json_object_text, 0, NULL);
    if(!object)
        fail("unable to parse an object");

    copy = json_copy(object);
    if(!copy)
        fail("unable to copy an object");
    if(copy == object)
        fail("copying an object doesn't copy");
    if(!json_equal(copy, object))
        fail("copying an object produces an inequal copy");

    i = 0;
    iter = json_object_iter(object);
    while(iter)
    {
        const char *key;
        json_t *value1, *value2;

        key = json_object_iter_key(iter);
        value1 = json_object_iter_value(iter);
        value2 = json_object_get(copy, key);

        if(value1 != value2)
            fail("copying an object modifies its items");

        if (strcmp(key, keys[i]) != 0)
            fail("copying an object doesn't preserve key order");

        iter = json_object_iter_next(object, iter);
        i++;
    }

    json_decref(object);
    json_decref(copy);
}

static void test_deep_copy_object(void)
{
    const char *json_object_text =
        "{\"foo\": \"bar\", \"a\": 1, \"b\": 3.141592, \"c\": [1,2,3,4]}";

    const char *keys[] = {"foo", "a", "b", "c"};
    int i;

    json_t *object, *copy;
    void *iter;

    object = json_loads(json_object_text, 0, NULL);
    if(!object)
        fail("unable to parse an object");

    copy = json_deep_copy(object);
    if(!copy)
        fail("unable to deep copy an object");
    if(copy == object)
        fail("deep copying an object doesn't copy");
    if(!json_equal(copy, object))
        fail("deep copying an object produces an inequal copy");

    i = 0;
    iter = json_object_iter(object);
    while(iter)
    {
        const char *key;
        json_t *value1, *value2;

        key = json_object_iter_key(iter);
        value1 = json_object_iter_value(iter);
        value2 = json_object_get(copy, key);

        if(value1 == value2)
            fail("deep copying an object doesn't copy its items");

        if (strcmp(key, keys[i]) != 0)
            fail("deep copying an object doesn't preserve key order");

        iter = json_object_iter_next(object, iter);
        i++;
    }

    json_decref(object);
    json_decref(copy);
}

static void run_tests()
{
    test_copy_simple();
    test_deep_copy_simple();
    test_copy_array();
    test_deep_copy_array();
    test_copy_object();
    test_deep_copy_object();
}
