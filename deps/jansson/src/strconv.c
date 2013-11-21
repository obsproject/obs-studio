#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "jansson_private.h"
#include "strbuffer.h"

/* need config.h to get the correct snprintf */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if JSON_HAVE_LOCALECONV
#include <locale.h>

/*
  - This code assumes that the decimal separator is exactly one
    character.

  - If setlocale() is called by another thread between the call to
    localeconv() and the call to sprintf() or strtod(), the result may
    be wrong. setlocale() is not thread-safe and should not be used
    this way. Multi-threaded programs should use uselocale() instead.
*/

static void to_locale(strbuffer_t *strbuffer)
{
    const char *point;
    char *pos;

    point = localeconv()->decimal_point;
    if(*point == '.') {
        /* No conversion needed */
        return;
    }

    pos = strchr(strbuffer->value, '.');
    if(pos)
        *pos = *point;
}

static void from_locale(char *buffer)
{
    const char *point;
    char *pos;

    point = localeconv()->decimal_point;
    if(*point == '.') {
        /* No conversion needed */
        return;
    }

    pos = strchr(buffer, *point);
    if(pos)
        *pos = '.';
}
#endif

int jsonp_strtod(strbuffer_t *strbuffer, double *out)
{
    double value;
    char *end;

#if JSON_HAVE_LOCALECONV
    to_locale(strbuffer);
#endif

    errno = 0;
    value = strtod(strbuffer->value, &end);
    assert(end == strbuffer->value + strbuffer->length);

    if(errno == ERANGE && value != 0) {
        /* Overflow */
        return -1;
    }

    *out = value;
    return 0;
}

int jsonp_dtostr(char *buffer, size_t size, double value)
{
    int ret;
    char *start, *end;
    size_t length;

    ret = snprintf(buffer, size, "%.17g", value);
    if(ret < 0)
        return -1;

    length = (size_t)ret;
    if(length >= size)
        return -1;

#if JSON_HAVE_LOCALECONV
    from_locale(buffer);
#endif

    /* Make sure there's a dot or 'e' in the output. Otherwise
       a real is converted to an integer when decoding */
    if(strchr(buffer, '.') == NULL &&
       strchr(buffer, 'e') == NULL)
    {
        if(length + 3 >= size) {
            /* No space to append ".0" */
            return -1;
        }
        buffer[length] = '.';
        buffer[length + 1] = '0';
        buffer[length + 2] = '\0';
        length += 2;
    }

    /* Remove leading '+' from positive exponent. Also remove leading
       zeros from exponents (added by some printf() implementations) */
    start = strchr(buffer, 'e');
    if(start) {
        start++;
        end = start + 1;

        if(*start == '-')
            start++;

        while(*end == '0')
            end++;

        if(end != start) {
            memmove(start, end, length - (size_t)(end - buffer));
            length -= (size_t)(end - start);
        }
    }

    return (int)length;
}
