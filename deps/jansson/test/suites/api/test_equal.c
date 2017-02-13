/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <jansson.h>
#include "util.h"

static void test_equal_simple()
{
    json_t *value1, *value2;

    if(json_equal(NULL, NULL))
        fail("json_equal fails for two NULLs");

    value1 = json_true();
    if(json_equal(value1, NULL) || json_equal(NULL, value1))
        fail("json_equal fails for NULL");

    /* this covers true, false and null as they are singletons */
    if(!json_equal(value1, value1))
        fail("identical objects are not equal");
    json_decref(value1);

    /* integer */
    value1 = json_integer(1);
    value2 = json_integer(1);
    if(!value1 || !value2)
        fail("unable to create integers");
    if(!json_equal(value1, value2))
        fail("json_equal fails for two equal integers");
    json_decref(value2);

    value2 = json_integer(2);
    if(!value2)
        fail("unable to create an integer");
    if(json_equal(value1, value2))
        fail("json_equal fails for two inequal integers");

    json_decref(value1);
    json_decref(value2);

    /* real */
    value1 = json_real(1.2);
    value2 = json_real(1.2);
    if(!value1 || !value2)
        fail("unable to create reals");
    if(!json_equal(value1, value2))
        fail("json_equal fails for two equal reals");
    json_decref(value2);

    value2 = json_real(3.141592);
    if(!value2)
        fail("unable to create an real");
    if(json_equal(value1, value2))
        fail("json_equal fails for two inequal reals");

    json_decref(value1);
    json_decref(value2);

    /* string */
    value1 = json_string("foo");
    value2 = json_string("foo");
    if(!value1 || !value2)
        fail("unable to create strings");
    if(!json_equal(value1, value2))
        fail("json_equal fails for two equal strings");
    json_decref(value2);

    value2 = json_string("bar");
    if(!value2)
        fail("unable to create an string");
    if(json_equal(value1, value2))
        fail("json_equal fails for two inequal strings");

    json_decref(value1);
    json_decref(value2);
}

static void test_equal_array()
{
    json_t *array1, *array2;

    array1 = json_array();
    array2 = json_array();
    if(!array1 || !array2)
        fail("unable to create arrays");

    if(!json_equal(array1, array2))
        fail("json_equal fails for two empty arrays");

    json_array_append_new(array1, json_integer(1));
    json_array_append_new(array2, json_integer(1));
    json_array_append_new(array1, json_string("foo"));
    json_array_append_new(array2, json_string("foo"));
    json_array_append_new(array1, json_integer(2));
    json_array_append_new(array2, json_integer(2));
    if(!json_equal(array1, array2))
        fail("json_equal fails for two equal arrays");

    json_array_remove(array2, 2);
    if(json_equal(array1, array2))
        fail("json_equal fails for two inequal arrays");

    json_array_append_new(array2, json_integer(3));
    if(json_equal(array1, array2))
        fail("json_equal fails for two inequal arrays");

    json_decref(array1);
    json_decref(array2);
}

static void test_equal_object()
{
    json_t *object1, *object2;

    object1 = json_object();
    object2 = json_object();
    if(!object1 || !object2)
        fail("unable to create objects");

    if(!json_equal(object1, object2))
        fail("json_equal fails for two empty objects");

    json_object_set_new(object1, "a", json_integer(1));
    json_object_set_new(object2, "a", json_integer(1));
    json_object_set_new(object1, "b", json_string("foo"));
    json_object_set_new(object2, "b", json_string("foo"));
    json_object_set_new(object1, "c", json_integer(2));
    json_object_set_new(object2, "c", json_integer(2));
    if(!json_equal(object1, object2))
        fail("json_equal fails for two equal objects");

    json_object_del(object2, "c");
    if(json_equal(object1, object2))
        fail("json_equal fails for two inequal objects");

    json_object_set_new(object2, "c", json_integer(3));
    if(json_equal(object1, object2))
        fail("json_equal fails for two inequal objects");

    json_object_del(object2, "c");
    json_object_set_new(object2, "d", json_integer(2));
    if(json_equal(object1, object2))
        fail("json_equal fails for two inequal objects");

    json_decref(object1);
    json_decref(object2);
}

static void test_equal_complex()
{
    json_t *value1, *value2;

    const char *complex_json =
"{"
"    \"integer\": 1, "
"    \"real\": 3.141592, "
"    \"string\": \"foobar\", "
"    \"true\": true, "
"    \"object\": {"
"        \"array-in-object\": [1,true,\"foo\",{}],"
"        \"object-in-object\": {\"foo\": \"bar\"}"
"    },"
"    \"array\": [\"foo\", false, null, 1.234]"
"}";

    value1 = json_loads(complex_json, 0, NULL);
    value2 = json_loads(complex_json, 0, NULL);
    if(!value1 || !value2)
        fail("unable to parse JSON");
    if(!json_equal(value1, value2))
        fail("json_equal fails for two inequal strings");

    json_decref(value1);
    json_decref(value2);

    /* TODO: There's no negative test case here */
}

static void run_tests()
{
    test_equal_simple();
    test_equal_array();
    test_equal_object();
    test_equal_complex();
}
