/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <jansson.h>
#include <string.h>
#include "util.h"

static void test_clear()
{
    json_t *object, *ten;

    object = json_object();
    ten = json_integer(10);

    if(!object)
        fail("unable to create object");
    if(!ten)
        fail("unable to create integer");

    if(json_object_set(object, "a", ten) ||
       json_object_set(object, "b", ten) ||
       json_object_set(object, "c", ten) ||
       json_object_set(object, "d", ten) ||
       json_object_set(object, "e", ten))
        fail("unable to set value");

    if(json_object_size(object) != 5)
        fail("invalid size");

    json_object_clear(object);

    if(json_object_size(object) != 0)
        fail("invalid size after clear");

    json_decref(ten);
    json_decref(object);
}

static void test_update()
{
    json_t *object, *other, *nine, *ten;

    object = json_object();
    other = json_object();

    nine = json_integer(9);
    ten = json_integer(10);

    if(!object || !other)
        fail("unable to create object");
    if(!nine || !ten)
        fail("unable to create integer");


    /* update an empty object with an empty object */

    if(json_object_update(object, other))
        fail("unable to update an emtpy object with an empty object");

    if(json_object_size(object) != 0)
        fail("invalid size after update");

    if(json_object_size(other) != 0)
        fail("invalid size for updater after update");


    /* update an empty object with a nonempty object */

    if(json_object_set(other, "a", ten) ||
       json_object_set(other, "b", ten) ||
       json_object_set(other, "c", ten) ||
       json_object_set(other, "d", ten) ||
       json_object_set(other, "e", ten))
        fail("unable to set value");

    if(json_object_update(object, other))
        fail("unable to update an empty object");

    if(json_object_size(object) != 5)
        fail("invalid size after update");

    if(json_object_get(object, "a") != ten ||
       json_object_get(object, "b") != ten ||
       json_object_get(object, "c") != ten ||
       json_object_get(object, "d") != ten ||
       json_object_get(object, "e") != ten)
        fail("update works incorrectly");


    /* perform the same update again */

    if(json_object_update(object, other))
        fail("unable to update a non-empty object");

    if(json_object_size(object) != 5)
        fail("invalid size after update");

    if(json_object_get(object, "a") != ten ||
       json_object_get(object, "b") != ten ||
       json_object_get(object, "c") != ten ||
       json_object_get(object, "d") != ten ||
       json_object_get(object, "e") != ten)
        fail("update works incorrectly");


    /* update a nonempty object with a nonempty object with both old
       and new keys */

    if(json_object_clear(other))
        fail("clear failed");

    if(json_object_set(other, "a", nine) ||
       json_object_set(other, "b", nine) ||
       json_object_set(other, "f", nine) ||
       json_object_set(other, "g", nine) ||
       json_object_set(other, "h", nine))
        fail("unable to set value");

    if(json_object_update(object, other))
        fail("unable to update a nonempty object");

    if(json_object_size(object) != 8)
        fail("invalid size after update");

    if(json_object_get(object, "a") != nine ||
       json_object_get(object, "b") != nine ||
       json_object_get(object, "f") != nine ||
       json_object_get(object, "g") != nine ||
       json_object_get(object, "h") != nine)
        fail("update works incorrectly");

    json_decref(nine);
    json_decref(ten);
    json_decref(other);
    json_decref(object);
}

static void test_set_many_keys()
{
    json_t *object, *value;
    const char *keys = "abcdefghijklmnopqrstuvwxyz";
    char buf[2];
    size_t i;

    object = json_object();
    if (!object)
        fail("unable to create object");

    value = json_string("a");
    if (!value)
        fail("unable to create string");

    buf[1] = '\0';
    for (i = 0; i < strlen(keys); i++) {
        buf[0] = keys[i];
        if (json_object_set(object, buf, value))
            fail("unable to set object key");
    }

    json_decref(object);
    json_decref(value);
}

static void test_conditional_updates()
{
    json_t *object, *other;

    object = json_pack("{sisi}", "foo", 1, "bar", 2);
    other = json_pack("{sisi}", "foo", 3, "baz", 4);

    if(json_object_update_existing(object, other))
        fail("json_object_update_existing failed");

    if(json_object_size(object) != 2)
        fail("json_object_update_existing added new items");

    if(json_integer_value(json_object_get(object, "foo")) != 3)
        fail("json_object_update_existing failed to update existing key");

    if(json_integer_value(json_object_get(object, "bar")) != 2)
        fail("json_object_update_existing updated wrong key");

    json_decref(object);

    object = json_pack("{sisi}", "foo", 1, "bar", 2);

    if(json_object_update_missing(object, other))
        fail("json_object_update_missing failed");

    if(json_object_size(object) != 3)
        fail("json_object_update_missing didn't add new items");

    if(json_integer_value(json_object_get(object, "foo")) != 1)
        fail("json_object_update_missing updated existing key");

    if(json_integer_value(json_object_get(object, "bar")) != 2)
        fail("json_object_update_missing updated wrong key");

    if(json_integer_value(json_object_get(object, "baz")) != 4)
        fail("json_object_update_missing didn't add new items");

    json_decref(object);
    json_decref(other);
}

static void test_circular()
{
    json_t *object1, *object2;

    object1 = json_object();
    object2 = json_object();
    if(!object1 || !object2)
        fail("unable to create object");

    /* the simple case is checked */
    if(json_object_set(object1, "a", object1) == 0)
        fail("able to set self");

    /* create circular references */
    if(json_object_set(object1, "a", object2) ||
       json_object_set(object2, "a", object1))
        fail("unable to set value");

    /* circularity is detected when dumping */
    if(json_dumps(object1, 0) != NULL)
        fail("able to dump circulars");

    /* decref twice to deal with the circular references */
    json_decref(object1);
    json_decref(object2);
    json_decref(object1);
}

static void test_set_nocheck()
{
    json_t *object, *string;

    object = json_object();
    string = json_string("bar");

    if(!object)
        fail("unable to create object");
    if(!string)
        fail("unable to create string");

    if(json_object_set_nocheck(object, "foo", string))
        fail("json_object_set_nocheck failed");
    if(json_object_get(object, "foo") != string)
        fail("json_object_get after json_object_set_nocheck failed");

    /* invalid UTF-8 in key */
    if(json_object_set_nocheck(object, "a\xefz", string))
        fail("json_object_set_nocheck failed for invalid UTF-8");
    if(json_object_get(object, "a\xefz") != string)
        fail("json_object_get after json_object_set_nocheck failed");

    if(json_object_set_new_nocheck(object, "bax", json_integer(123)))
        fail("json_object_set_new_nocheck failed");
    if(json_integer_value(json_object_get(object, "bax")) != 123)
        fail("json_object_get after json_object_set_new_nocheck failed");

    /* invalid UTF-8 in key */
    if(json_object_set_new_nocheck(object, "asdf\xfe", json_integer(321)))
        fail("json_object_set_new_nocheck failed for invalid UTF-8");
    if(json_integer_value(json_object_get(object, "asdf\xfe")) != 321)
        fail("json_object_get after json_object_set_new_nocheck failed");

    json_decref(string);
    json_decref(object);
}

static void test_iterators()
{
    json_t *object, *foo, *bar, *baz;
    void *iter;

    if(json_object_iter(NULL))
        fail("able to iterate over NULL");

    if(json_object_iter_next(NULL, NULL))
        fail("able to increment an iterator on a NULL object");

    object = json_object();
    foo = json_string("foo");
    bar = json_string("bar");
    baz = json_string("baz");
    if(!object || !foo || !bar || !baz)
        fail("unable to create values");

    if(json_object_iter_next(object, NULL))
        fail("able to increment a NULL iterator");

    if(json_object_set(object, "a", foo) ||
       json_object_set(object, "b", bar) ||
       json_object_set(object, "c", baz))
        fail("unable to populate object");

    iter = json_object_iter(object);
    if(!iter)
        fail("unable to get iterator");
    if (strcmp(json_object_iter_key(iter), "a") != 0)
        fail("iterating doesn't yield keys in order");
    if (json_object_iter_value(iter) != foo)
        fail("iterating doesn't yield values in order");

    iter = json_object_iter_next(object, iter);
    if(!iter)
        fail("unable to increment iterator");
    if (strcmp(json_object_iter_key(iter), "b") != 0)
        fail("iterating doesn't yield keys in order");
    if (json_object_iter_value(iter) != bar)
        fail("iterating doesn't yield values in order");

    iter = json_object_iter_next(object, iter);
    if(!iter)
        fail("unable to increment iterator");
    if (strcmp(json_object_iter_key(iter), "c") != 0)
        fail("iterating doesn't yield keys in order");
    if (json_object_iter_value(iter) != baz)
        fail("iterating doesn't yield values in order");

    if(json_object_iter_next(object, iter) != NULL)
        fail("able to iterate over the end");

    if(json_object_iter_at(object, "foo"))
        fail("json_object_iter_at() succeeds for non-existent key");

    iter = json_object_iter_at(object, "b");
    if(!iter)
        fail("json_object_iter_at() fails for an existing key");

    if(strcmp(json_object_iter_key(iter), "b"))
        fail("iterating failed: wrong key");
    if(json_object_iter_value(iter) != bar)
        fail("iterating failed: wrong value");

    if(json_object_iter_set(object, iter, baz))
        fail("unable to set value at iterator");

    if(strcmp(json_object_iter_key(iter), "b"))
        fail("json_object_iter_key() fails after json_object_iter_set()");
    if(json_object_iter_value(iter) != baz)
        fail("json_object_iter_value() fails after json_object_iter_set()");
    if(json_object_get(object, "b") != baz)
        fail("json_object_get() fails after json_object_iter_set()");

    json_decref(object);
    json_decref(foo);
    json_decref(bar);
    json_decref(baz);
}

static void test_misc()
{
    json_t *object, *string, *other_string, *value;

    object = json_object();
    string = json_string("test");
    other_string = json_string("other");

    if(!object)
        fail("unable to create object");
    if(!string || !other_string)
        fail("unable to create string");

    if(json_object_get(object, "a"))
        fail("value for nonexisting key");

    if(json_object_set(object, "a", string))
        fail("unable to set value");

    if(!json_object_set(object, NULL, string))
        fail("able to set NULL key");

    if(json_object_del(object, "a"))
        fail("unable to del the only key");

    if(json_object_set(object, "a", string))
        fail("unable to set value");

    if(!json_object_set(object, "a", NULL))
        fail("able to set NULL value");

    /* invalid UTF-8 in key */
    if(!json_object_set(object, "a\xefz", string))
        fail("able to set invalid unicode key");

    value = json_object_get(object, "a");
    if(!value)
        fail("no value for existing key");
    if(value != string)
        fail("got different value than what was added");

    /* "a", "lp" and "px" collide in a five-bucket hashtable */
    if(json_object_set(object, "b", string) ||
       json_object_set(object, "lp", string) ||
       json_object_set(object, "px", string))
        fail("unable to set value");

    value = json_object_get(object, "a");
    if(!value)
        fail("no value for existing key");
    if(value != string)
        fail("got different value than what was added");

    if(json_object_set(object, "a", other_string))
        fail("unable to replace an existing key");

    value = json_object_get(object, "a");
    if(!value)
        fail("no value for existing key");
    if(value != other_string)
        fail("got different value than what was set");

    if(!json_object_del(object, "nonexisting"))
        fail("able to delete a nonexisting key");

    if(json_object_del(object, "px"))
        fail("unable to delete an existing key");

    if(json_object_del(object, "a"))
        fail("unable to delete an existing key");

    if(json_object_del(object, "lp"))
        fail("unable to delete an existing key");


    /* add many keys to initiate rehashing */

    if(json_object_set(object, "a", string))
        fail("unable to set value");

    if(json_object_set(object, "lp", string))
        fail("unable to set value");

    if(json_object_set(object, "px", string))
        fail("unable to set value");

    if(json_object_set(object, "c", string))
        fail("unable to set value");

    if(json_object_set(object, "d", string))
        fail("unable to set value");

    if(json_object_set(object, "e", string))
        fail("unable to set value");


    if(json_object_set_new(object, "foo", json_integer(123)))
        fail("unable to set new value");

    value = json_object_get(object, "foo");
    if(!json_is_integer(value) || json_integer_value(value) != 123)
        fail("json_object_set_new works incorrectly");

    if(!json_object_set_new(object, NULL, json_integer(432)))
        fail("able to set_new NULL key");

    if(!json_object_set_new(object, "foo", NULL))
        fail("able to set_new NULL value");

    json_decref(string);
    json_decref(other_string);
    json_decref(object);
}

static void test_preserve_order()
{
    json_t *object;
    char *result;

    const char *expected = "{\"foobar\": 1, \"bazquux\": 6, \"lorem ipsum\": 3, \"sit amet\": 5, \"helicopter\": 7}";

    object = json_object();

    json_object_set_new(object, "foobar", json_integer(1));
    json_object_set_new(object, "bazquux", json_integer(2));
    json_object_set_new(object, "lorem ipsum", json_integer(3));
    json_object_set_new(object, "dolor", json_integer(4));
    json_object_set_new(object, "sit amet", json_integer(5));

    /* changing a value should preserve the order */
    json_object_set_new(object, "bazquux", json_integer(6));

    /* deletion shouldn't change the order of others */
    json_object_del(object, "dolor");

    /* add a new item just to make sure */
    json_object_set_new(object, "helicopter", json_integer(7));

    result = json_dumps(object, JSON_PRESERVE_ORDER);

    if(strcmp(expected, result) != 0) {
        fprintf(stderr, "%s != %s", expected, result);
        fail("JSON_PRESERVE_ORDER doesn't work");
    }

    free(result);
    json_decref(object);
}

static void test_object_foreach()
{
    const char *key;
    json_t *object1, *object2, *value;

    object1 = json_pack("{sisisi}", "foo", 1, "bar", 2, "baz", 3);
    object2 = json_object();

    json_object_foreach(object1, key, value)
        json_object_set(object2, key, value);

    if(!json_equal(object1, object2))
        fail("json_object_foreach failed to iterate all key-value pairs");

    json_decref(object1);
    json_decref(object2);
}

static void test_object_foreach_safe()
{
    const char *key;
    void *tmp;
    json_t *object, *value;

    object = json_pack("{sisisi}", "foo", 1, "bar", 2, "baz", 3);

    json_object_foreach_safe(object, tmp, key, value) {
        json_object_del(object, key);
    }

    if(json_object_size(object) != 0)
        fail("json_object_foreach_safe failed to iterate all key-value pairs");

    json_decref(object);
}

static void run_tests()
{
    test_misc();
    test_clear();
    test_update();
    test_set_many_keys();
    test_conditional_updates();
    test_circular();
    test_set_nocheck();
    test_iterators();
    test_preserve_order();
    test_object_foreach();
    test_object_foreach_safe();
}
