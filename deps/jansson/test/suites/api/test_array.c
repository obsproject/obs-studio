/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <jansson.h>
#include "util.h"

static void test_misc(void)
{
    json_t *array, *five, *seven, *value;
    size_t i;

    array = json_array();
    five = json_integer(5);
    seven = json_integer(7);

    if(!array)
        fail("unable to create array");
    if(!five || !seven)
        fail("unable to create integer");

    if(json_array_size(array) != 0)
        fail("empty array has nonzero size");

    if(!json_array_append(array, NULL))
        fail("able to append NULL");

    if(json_array_append(array, five))
        fail("unable to append");

    if(json_array_size(array) != 1)
        fail("wrong array size");

    value = json_array_get(array, 0);
    if(!value)
        fail("unable to get item");
    if(value != five)
        fail("got wrong value");

    if(json_array_append(array, seven))
        fail("unable to append value");

    if(json_array_size(array) != 2)
        fail("wrong array size");

    value = json_array_get(array, 1);
    if(!value)
        fail("unable to get item");
    if(value != seven)
        fail("got wrong value");

    if(json_array_set(array, 0, seven))
        fail("unable to set value");

    if(!json_array_set(array, 0, NULL))
        fail("able to set NULL");

    if(json_array_size(array) != 2)
        fail("wrong array size");

    value = json_array_get(array, 0);
    if(!value)
        fail("unable to get item");
    if(value != seven)
        fail("got wrong value");

    if(json_array_get(array, 2) != NULL)
        fail("able to get value out of bounds");

    if(!json_array_set(array, 2, seven))
        fail("able to set value out of bounds");

    for(i = 2; i < 30; i++) {
        if(json_array_append(array, seven))
            fail("unable to append value");

        if(json_array_size(array) != i + 1)
            fail("wrong array size");
    }

    for(i = 0; i < 30; i++) {
        value = json_array_get(array, i);
        if(!value)
            fail("unable to get item");
        if(value != seven)
            fail("got wrong value");
    }

    if(json_array_set_new(array, 15, json_integer(123)))
        fail("unable to set new value");

    value = json_array_get(array, 15);
    if(!json_is_integer(value) || json_integer_value(value) != 123)
        fail("json_array_set_new works incorrectly");

    if(!json_array_set_new(array, 15, NULL))
        fail("able to set_new NULL value");

    if(json_array_append_new(array, json_integer(321)))
        fail("unable to append new value");

    value = json_array_get(array, json_array_size(array) - 1);
    if(!json_is_integer(value) || json_integer_value(value) != 321)
        fail("json_array_append_new works incorrectly");

    if(!json_array_append_new(array, NULL))
        fail("able to append_new NULL value");

    json_decref(five);
    json_decref(seven);
    json_decref(array);
}

static void test_insert(void)
{
    json_t *array, *five, *seven, *eleven, *value;
    int i;

    array = json_array();
    five = json_integer(5);
    seven = json_integer(7);
    eleven = json_integer(11);

    if(!array)
        fail("unable to create array");
    if(!five || !seven || !eleven)
        fail("unable to create integer");


    if(!json_array_insert(array, 1, five))
        fail("able to insert value out of bounds");


    if(json_array_insert(array, 0, five))
        fail("unable to insert value in an empty array");

    if(json_array_get(array, 0) != five)
        fail("json_array_insert works incorrectly");

    if(json_array_size(array) != 1)
        fail("array size is invalid after insertion");


    if(json_array_insert(array, 1, seven))
        fail("unable to insert value at the end of an array");

    if(json_array_get(array, 0) != five)
        fail("json_array_insert works incorrectly");

    if(json_array_get(array, 1) != seven)
        fail("json_array_insert works incorrectly");

    if(json_array_size(array) != 2)
        fail("array size is invalid after insertion");


    if(json_array_insert(array, 1, eleven))
        fail("unable to insert value in the middle of an array");

    if(json_array_get(array, 0) != five)
        fail("json_array_insert works incorrectly");

    if(json_array_get(array, 1) != eleven)
        fail("json_array_insert works incorrectly");

    if(json_array_get(array, 2) != seven)
        fail("json_array_insert works incorrectly");

    if(json_array_size(array) != 3)
        fail("array size is invalid after insertion");


    if(json_array_insert_new(array, 2, json_integer(123)))
        fail("unable to insert value in the middle of an array");

    value = json_array_get(array, 2);
    if(!json_is_integer(value) || json_integer_value(value) != 123)
        fail("json_array_insert_new works incorrectly");

    if(json_array_size(array) != 4)
        fail("array size is invalid after insertion");


    for(i = 0; i < 20; i++) {
        if(json_array_insert(array, 0, seven))
            fail("unable to insert value at the begining of an array");
    }

    for(i = 0; i < 20; i++) {
        if(json_array_get(array, i) != seven)
            fail("json_aray_insert works incorrectly");
    }

    if(json_array_size(array) != 24)
        fail("array size is invalid after loop insertion");

    json_decref(five);
    json_decref(seven);
    json_decref(eleven);
    json_decref(array);
}

static void test_remove(void)
{
    json_t *array, *five, *seven;
    int i;

    array = json_array();
    five = json_integer(5);
    seven = json_integer(7);

    if(!array)
        fail("unable to create array");
    if(!five)
        fail("unable to create integer");
    if(!seven)
        fail("unable to create integer");


    if(!json_array_remove(array, 0))
        fail("able to remove an unexisting index");


    if(json_array_append(array, five))
        fail("unable to append");

    if(!json_array_remove(array, 1))
        fail("able to remove an unexisting index");

    if(json_array_remove(array, 0))
        fail("unable to remove");

    if(json_array_size(array) != 0)
        fail("array size is invalid after removing");


    if(json_array_append(array, five) ||
       json_array_append(array, seven) ||
       json_array_append(array, five) ||
       json_array_append(array, seven))
        fail("unable to append");

    if(json_array_remove(array, 2))
        fail("unable to remove");

    if(json_array_size(array) != 3)
        fail("array size is invalid after removing");

    if(json_array_get(array, 0) != five ||
       json_array_get(array, 1) != seven ||
       json_array_get(array, 2) != seven)
        fail("remove works incorrectly");

    json_decref(array);

    array = json_array();
    for(i = 0; i < 4; i++) {
        json_array_append(array, five);
        json_array_append(array, seven);
    }
    if(json_array_size(array) != 8)
        fail("unable to append 8 items to array");

    /* Remove an element from a "full" array. */
    json_array_remove(array, 5);

    json_decref(five);
    json_decref(seven);
    json_decref(array);
}

static void test_clear(void)
{
    json_t *array, *five, *seven;
    int i;

    array = json_array();
    five = json_integer(5);
    seven = json_integer(7);

    if(!array)
        fail("unable to create array");
    if(!five || !seven)
        fail("unable to create integer");

    for(i = 0; i < 10; i++) {
        if(json_array_append(array, five))
            fail("unable to append");
    }
    for(i = 0; i < 10; i++) {
        if(json_array_append(array, seven))
            fail("unable to append");
    }

    if(json_array_size(array) != 20)
        fail("array size is invalid after appending");

    if(json_array_clear(array))
        fail("unable to clear");

    if(json_array_size(array) != 0)
        fail("array size is invalid after clearing");

    json_decref(five);
    json_decref(seven);
    json_decref(array);
}

static void test_extend(void)
{
    json_t *array1, *array2, *five, *seven;
    int i;

    array1 = json_array();
    array2 = json_array();
    five = json_integer(5);
    seven = json_integer(7);

    if(!array1 || !array2)
        fail("unable to create array");
    if(!five || !seven)
        fail("unable to create integer");

    for(i = 0; i < 10; i++) {
        if(json_array_append(array1, five))
            fail("unable to append");
    }
    for(i = 0; i < 10; i++) {
        if(json_array_append(array2, seven))
            fail("unable to append");
    }

    if(json_array_size(array1) != 10 || json_array_size(array2) != 10)
        fail("array size is invalid after appending");

    if(json_array_extend(array1, array2))
        fail("unable to extend");

    for(i = 0; i < 10; i++) {
        if(json_array_get(array1, i) != five)
            fail("invalid array contents after extending");
    }
    for(i = 10; i < 20; i++) {
        if(json_array_get(array1, i) != seven)
            fail("invalid array contents after extending");
    }

    json_decref(five);
    json_decref(seven);
    json_decref(array1);
    json_decref(array2);
}

static void test_circular()
{
    json_t *array1, *array2;

    /* the simple cases are checked */

    array1 = json_array();
    if(!array1)
        fail("unable to create array");

    if(json_array_append(array1, array1) == 0)
        fail("able to append self");

    if(json_array_insert(array1, 0, array1) == 0)
        fail("able to insert self");

    if(json_array_append_new(array1, json_true()))
        fail("failed to append true");

    if(json_array_set(array1, 0, array1) == 0)
        fail("able to set self");

    json_decref(array1);


    /* create circular references */

    array1 = json_array();
    array2 = json_array();
    if(!array1 || !array2)
        fail("unable to create array");

    if(json_array_append(array1, array2) ||
       json_array_append(array2, array1))
        fail("unable to append");

    /* circularity is detected when dumping */
    if(json_dumps(array1, 0) != NULL)
        fail("able to dump circulars");

    /* decref twice to deal with the circular references */
    json_decref(array1);
    json_decref(array2);
    json_decref(array1);
}

static void test_array_foreach()
{
    size_t index;
    json_t *array1, *array2, *value;

    array1 = json_pack("[sisisi]", "foo", 1, "bar", 2, "baz", 3);
    array2 = json_array();

    json_array_foreach(array1, index, value) {
        json_array_append(array2, value);
    }
    
    if(!json_equal(array1, array2))
        fail("json_array_foreach failed to iterate all elements");

    json_decref(array1);
    json_decref(array2);
}


static void run_tests()
{
    test_misc();
    test_insert();
    test_remove();
    test_clear();
    test_extend();
    test_circular();
    test_array_foreach();
}
