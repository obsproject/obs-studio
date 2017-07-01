/*
 * Copyright (c) 2009-2016 Petri Lehtinen <petri@digip.org>
 * Copyright (c) 2010-2012 Graeme Smecher <graeme.smecher@mail.mcgill.ca>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <string.h>
#include <jansson.h>
#include <stdio.h>
#include "util.h"

static void run_tests()
{
    json_t *j, *j2;
    int i1, i2, i3;
    json_int_t I1;
    int rv;
    size_t z;
    double f;
    char *s;

    json_error_t error;

    /*
     * Simple, valid json_pack cases
     */

    /* true */
    rv = json_unpack(json_true(), "b", &i1);
    if(rv || !i1)
        fail("json_unpack boolean failed");

    /* false */
    rv = json_unpack(json_false(), "b", &i1);
    if(rv || i1)
        fail("json_unpack boolean failed");

    /* null */
    if(json_unpack(json_null(), "n"))
        fail("json_unpack null failed");

    /* integer */
    j = json_integer(42);
    rv = json_unpack(j, "i", &i1);
    if(rv || i1 != 42)
        fail("json_unpack integer failed");
    json_decref(j);

    /* json_int_t */
    j = json_integer(5555555);
    rv = json_unpack(j, "I", &I1);
    if(rv || I1 != 5555555)
        fail("json_unpack json_int_t failed");
    json_decref(j);

    /* real */
    j = json_real(1.7);
    rv = json_unpack(j, "f", &f);
    if(rv || f != 1.7)
        fail("json_unpack real failed");
    json_decref(j);

    /* number */
    j = json_integer(12345);
    rv = json_unpack(j, "F", &f);
    if(rv || f != 12345.0)
        fail("json_unpack (real or) integer failed");
    json_decref(j);

    j = json_real(1.7);
    rv = json_unpack(j, "F", &f);
    if(rv || f != 1.7)
        fail("json_unpack real (or integer) failed");
    json_decref(j);

    /* string */
    j = json_string("foo");
    rv = json_unpack(j, "s", &s);
    if(rv || strcmp(s, "foo"))
        fail("json_unpack string failed");
    json_decref(j);

    /* string with length (size_t) */
    j = json_string("foo");
    rv = json_unpack(j, "s%", &s, &z);
    if(rv || strcmp(s, "foo") || z != 3)
        fail("json_unpack string with length (size_t) failed");
    json_decref(j);

    /* empty object */
    j = json_object();
    if(json_unpack(j, "{}"))
        fail("json_unpack empty object failed");
    json_decref(j);

    /* empty list */
    j = json_array();
    if(json_unpack(j, "[]"))
        fail("json_unpack empty list failed");
    json_decref(j);

    /* non-incref'd object */
    j = json_object();
    rv = json_unpack(j, "o", &j2);
    if(rv || j2 != j || j->refcount != 1)
        fail("json_unpack object failed");
    json_decref(j);

    /* incref'd object */
    j = json_object();
    rv = json_unpack(j, "O", &j2);
    if(rv || j2 != j || j->refcount != 2)
        fail("json_unpack object failed");
    json_decref(j);
    json_decref(j);

    /* simple object */
    j = json_pack("{s:i}", "foo", 42);
    rv = json_unpack(j, "{s:i}", "foo", &i1);
    if(rv || i1 != 42)
        fail("json_unpack simple object failed");
    json_decref(j);

    /* simple array */
    j = json_pack("[iii]", 1, 2, 3);
    rv = json_unpack(j, "[i,i,i]", &i1, &i2, &i3);
    if(rv || i1 != 1 || i2 != 2 || i3 != 3)
        fail("json_unpack simple array failed");
    json_decref(j);

    /* object with many items & strict checking */
    j = json_pack("{s:i, s:i, s:i}", "a", 1, "b", 2, "c", 3);
    rv = json_unpack(j, "{s:i, s:i, s:i}", "a", &i1, "b", &i2, "c", &i3);
    if(rv || i1 != 1 || i2 != 2 || i3 != 3)
        fail("json_unpack object with many items failed");
    json_decref(j);

    /*
     * Invalid cases
     */

    j = json_integer(42);
    if(!json_unpack_ex(j, &error, 0, "z"))
        fail("json_unpack succeeded with invalid format character");
    check_error("Unexpected format character 'z'", "<format>", 1, 1, 1);

    if(!json_unpack_ex(NULL, &error, 0, "[i]"))
        fail("json_unpack succeeded with NULL root");
    check_error("NULL root value", "<root>", -1, -1, 0);
    json_decref(j);

    /* mismatched open/close array/object */
    j = json_pack("[]");
    if(!json_unpack_ex(j, &error, 0, "[}"))
        fail("json_unpack failed to catch mismatched ']'");
    check_error("Unexpected format character '}'", "<format>", 1, 2, 2);
    json_decref(j);

    j = json_pack("{}");
    if(!json_unpack_ex(j, &error, 0, "{]"))
        fail("json_unpack failed to catch mismatched '}'");
    check_error("Expected format 's', got ']'", "<format>", 1, 2, 2);
    json_decref(j);

    /* missing close array */
    j = json_pack("[]");
    if(!json_unpack_ex(j, &error, 0, "["))
        fail("json_unpack failed to catch missing ']'");
    check_error("Unexpected end of format string", "<format>", 1, 2, 2);
    json_decref(j);

    /* missing close object */
    j = json_pack("{}");
    if(!json_unpack_ex(j, &error, 0, "{"))
        fail("json_unpack failed to catch missing '}'");
    check_error("Unexpected end of format string", "<format>", 1, 2, 2);
    json_decref(j);

    /* garbage after format string */
    j = json_pack("[i]", 42);
    if(!json_unpack_ex(j, &error, 0, "[i]a", &i1))
        fail("json_unpack failed to catch garbage after format string");
    check_error("Garbage after format string", "<format>", 1, 4, 4);
    json_decref(j);

    j = json_integer(12345);
    if(!json_unpack_ex(j, &error, 0, "ia", &i1))
        fail("json_unpack failed to catch garbage after format string");
    check_error("Garbage after format string", "<format>", 1, 2, 2);
    json_decref(j);

    /* NULL format string */
    j = json_pack("[]");
    if(!json_unpack_ex(j, &error, 0, NULL))
        fail("json_unpack failed to catch null format string");
    check_error("NULL or empty format string", "<format>", -1, -1, 0);
    json_decref(j);

    /* NULL string pointer */
    j = json_string("foobie");
    if(!json_unpack_ex(j, &error, 0, "s", NULL))
        fail("json_unpack failed to catch null string pointer");
    check_error("NULL string argument", "<args>", 1, 1, 1);
    json_decref(j);

    /* invalid types */
    j = json_integer(42);
    j2 = json_string("foo");
    if(!json_unpack_ex(j, &error, 0, "s"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected string, got integer", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j, &error, 0, "n"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected null, got integer", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j, &error, 0, "b"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected true or false, got integer", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j2, &error, 0, "i"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected integer, got string", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j2, &error, 0, "I"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected integer, got string", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j, &error, 0, "f"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected real, got integer", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j2, &error, 0, "F"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected real or integer, got string", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j, &error, 0, "[i]"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected array, got integer", "<validation>", 1, 1, 1);

    if(!json_unpack_ex(j, &error, 0, "{si}", "foo"))
        fail("json_unpack failed to catch invalid type");
    check_error("Expected object, got integer", "<validation>", 1, 1, 1);

    json_decref(j);
    json_decref(j2);

    /* Array index out of range */
    j = json_pack("[i]", 1);
    if(!json_unpack_ex(j, &error, 0, "[ii]", &i1, &i2))
        fail("json_unpack failed to catch index out of array bounds");
    check_error("Array index 1 out of range", "<validation>", 1, 3, 3);
    json_decref(j);

    /* NULL object key */
    j = json_pack("{si}", "foo", 42);
    if(!json_unpack_ex(j, &error, 0, "{si}", NULL, &i1))
        fail("json_unpack failed to catch null string pointer");
    check_error("NULL object key", "<args>", 1, 2, 2);
    json_decref(j);

    /* Object key not found */
    j = json_pack("{si}", "foo", 42);
    if(!json_unpack_ex(j, &error, 0, "{si}", "baz", &i1))
        fail("json_unpack failed to catch null string pointer");
    check_error("Object item not found: baz", "<validation>", 1, 3, 3);
    json_decref(j);

    /*
     * Strict validation
     */

    j = json_pack("[iii]", 1, 2, 3);
    rv = json_unpack(j, "[iii!]", &i1, &i2, &i3);
    if(rv || i1 != 1 || i2 != 2 || i3 != 3)
        fail("json_unpack array with strict validation failed");
    json_decref(j);

    j = json_pack("[iii]", 1, 2, 3);
    if(!json_unpack_ex(j, &error, 0, "[ii!]", &i1, &i2))
        fail("json_unpack array with strict validation failed");
    check_error("1 array item(s) left unpacked", "<validation>", 1, 5, 5);
    json_decref(j);

    /* Like above, but with JSON_STRICT instead of '!' format */
    j = json_pack("[iii]", 1, 2, 3);
    if(!json_unpack_ex(j, &error, JSON_STRICT, "[ii]", &i1, &i2))
        fail("json_unpack array with strict validation failed");
    check_error("1 array item(s) left unpacked", "<validation>", 1, 4, 4);
    json_decref(j);

    j = json_pack("{s:s, s:i}", "foo", "bar", "baz", 42);
    rv = json_unpack(j, "{sssi!}", "foo", &s, "baz", &i1);
    if(rv || strcmp(s, "bar") != 0 || i1 != 42)
        fail("json_unpack object with strict validation failed");
    json_decref(j);

    /* Unpack the same item twice */
    j = json_pack("{s:s, s:i, s:b}", "foo", "bar", "baz", 42, "quux", 1);
    if(!json_unpack_ex(j, &error, 0, "{s:s,s:s!}", "foo", &s, "foo", &s))
        fail("json_unpack object with strict validation failed");
    {
        const char *possible_errors[] = {
            "2 object item(s) left unpacked: baz, quux",
            "2 object item(s) left unpacked: quux, baz"
        };
        check_errors(possible_errors, 2, "<validation>", 1, 10, 10);
    }
    json_decref(j);

    j = json_pack("[i,{s:i,s:n},[i,i]]", 1, "foo", 2, "bar", 3, 4);
    if(json_unpack_ex(j, NULL, JSON_STRICT | JSON_VALIDATE_ONLY,
                      "[i{sisn}[ii]]", "foo", "bar"))
        fail("json_unpack complex value with strict validation failed");
    json_decref(j);

    /* ! and * must be last */
    j = json_pack("[ii]", 1, 2);
    if(!json_unpack_ex(j, &error, 0, "[i!i]", &i1, &i2))
        fail("json_unpack failed to catch ! in the middle of an array");
    check_error("Expected ']' after '!', got 'i'", "<format>", 1, 4, 4);

    if(!json_unpack_ex(j, &error, 0, "[i*i]", &i1, &i2))
        fail("json_unpack failed to catch * in the middle of an array");
    check_error("Expected ']' after '*', got 'i'", "<format>", 1, 4, 4);
    json_decref(j);

    j = json_pack("{sssi}", "foo", "bar", "baz", 42);
    if(!json_unpack_ex(j, &error, 0, "{ss!si}", "foo", &s, "baz", &i1))
        fail("json_unpack failed to catch ! in the middle of an object");
    check_error("Expected '}' after '!', got 's'", "<format>", 1, 5, 5);

    if(!json_unpack_ex(j, &error, 0, "{ss*si}", "foo", &s, "baz", &i1))
        fail("json_unpack failed to catch ! in the middle of an object");
    check_error("Expected '}' after '*', got 's'", "<format>", 1, 5, 5);
    json_decref(j);

    /* Error in nested object */
    j = json_pack("{s{snsn}}", "foo", "bar", "baz");
    if(!json_unpack_ex(j, &error, 0, "{s{sn!}}", "foo", "bar"))
        fail("json_unpack nested object with strict validation failed");
    check_error("1 object item(s) left unpacked: baz", "<validation>", 1, 7, 7);
    json_decref(j);

    /* Error in nested array */
    j = json_pack("[[ii]]", 1, 2);
    if(!json_unpack_ex(j, &error, 0, "[[i!]]", &i1))
        fail("json_unpack nested array with strict validation failed");
    check_error("1 array item(s) left unpacked", "<validation>", 1, 5, 5);
    json_decref(j);

    /* Optional values */
    j = json_object();
    i1 = 0;
    if(json_unpack(j, "{s?i}", "foo", &i1))
        fail("json_unpack failed for optional key");
    if(i1 != 0)
        fail("json_unpack unpacked an optional key");
    json_decref(j);

    i1 = 0;
    j = json_pack("{si}", "foo", 42);
    if(json_unpack(j, "{s?i}", "foo", &i1))
        fail("json_unpack failed for an optional value");
    if(i1 != 42)
        fail("json_unpack failed to unpack an optional value");
    json_decref(j);

    j = json_object();
    i1 = i2 = i3 = 0;
    if(json_unpack(j, "{s?[ii]s?{s{si}}}",
                   "foo", &i1, &i2,
                   "bar", "baz", "quux", &i3))
        fail("json_unpack failed for complex optional values");
    if(i1 != 0 || i2 != 0 || i3 != 0)
        fail("json_unpack unexpectedly unpacked something");
    json_decref(j);

    j = json_pack("{s{si}}", "foo", "bar", 42);
    if(json_unpack(j, "{s?{s?i}}", "foo", "bar", &i1))
        fail("json_unpack failed for complex optional values");
    if(i1 != 42)
        fail("json_unpack failed to unpack");
    json_decref(j);

    /* Combine ? and ! */
    j = json_pack("{si}", "foo", 42);
    i1 = i2 = 0;
    if(json_unpack(j, "{sis?i!}", "foo", &i1, "bar", &i2))
        fail("json_unpack failed for optional values with strict mode");
    if(i1 != 42)
        fail("json_unpack failed to unpack");
    if(i2 != 0)
        fail("json_unpack failed to unpack");
    json_decref(j);

    /* But don't compensate a missing key with an optional one. */
    j = json_pack("{sisi}", "foo", 42, "baz", 43);
    i1 = i2 = i3 = 0;
    if(!json_unpack_ex(j, &error, 0, "{sis?i!}", "foo", &i1, "bar", &i2))
        fail("json_unpack failed for optional values with strict mode and compensation");
    check_error("1 object item(s) left unpacked: baz", "<validation>", 1, 8, 8);
    json_decref(j);
}
