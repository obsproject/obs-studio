/*
Copyright (c) 2015-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* a ring-buffer implementation using macros
 */
#ifndef UTRINGBUFFER_H
#define UTRINGBUFFER_H

#define UTRINGBUFFER_VERSION 2.3.0

#include <stdlib.h>
#include <string.h>
#include "utarray.h"  // for "UT_icd"

typedef struct {
    unsigned i;       /* index of next available slot; wraps at n */
    unsigned n;       /* capacity */
    unsigned char f;  /* full */
    UT_icd icd;       /* initializer, copy and destructor functions */
    char *d;          /* n slots of size icd->sz */
} UT_ringbuffer;

#define utringbuffer_init(a, _n, _icd) do {                               \
  memset(a, 0, sizeof(UT_ringbuffer));                                    \
  (a)->icd = *(_icd);                                                     \
  (a)->n = (_n);                                                          \
  if ((a)->n) { (a)->d = (char*)malloc((a)->n * (_icd)->sz); }            \
} while(0)

#define utringbuffer_clear(a) do {                                        \
  if ((a)->icd.dtor) {                                                    \
    if ((a)->f) {                                                         \
      unsigned _ut_i;                                                     \
      for (_ut_i = 0; _ut_i < (a)->n; ++_ut_i) {                          \
        (a)->icd.dtor(utringbuffer_eltptr(a, _ut_i));                     \
      }                                                                   \
    } else {                                                              \
      unsigned _ut_i;                                                     \
      for (_ut_i = 0; _ut_i < (a)->i; ++_ut_i) {                          \
        (a)->icd.dtor(utringbuffer_eltptr(a, _ut_i));                     \
      }                                                                   \
    }                                                                     \
  }                                                                       \
  (a)->i = 0;                                                             \
  (a)->f = 0;                                                             \
} while(0)

#define utringbuffer_done(a) do {                                         \
  utringbuffer_clear(a);                                                  \
  free((a)->d); (a)->d = NULL;                                            \
  (a)->n = 0;                                                             \
} while(0)

#define utringbuffer_new(a,n,_icd) do {                                   \
  a = (UT_ringbuffer*)malloc(sizeof(UT_ringbuffer));                      \
  utringbuffer_init(a, n, _icd);                                          \
} while(0)

#define utringbuffer_free(a) do {                                         \
  utringbuffer_done(a);                                                   \
  free(a);                                                                \
} while(0)

#define utringbuffer_push_back(a,p) do {                                                \
  if ((a)->icd.dtor && (a)->f) { (a)->icd.dtor(_utringbuffer_internalptr(a,(a)->i)); }  \
  if ((a)->icd.copy) { (a)->icd.copy( _utringbuffer_internalptr(a,(a)->i), p); }        \
  else { memcpy(_utringbuffer_internalptr(a,(a)->i), p, (a)->icd.sz); };                \
  if (++(a)->i == (a)->n) { (a)->i = 0; (a)->f = 1; }                                   \
} while(0)

#define utringbuffer_len(a) ((a)->f ? (a)->n : (a)->i)
#define utringbuffer_empty(a) ((a)->i == 0 && !(a)->f)
#define utringbuffer_full(a) ((a)->f != 0)

#define _utringbuffer_real_idx(a,j) ((a)->f ? ((j) + (a)->i) % (a)->n : (j))
#define _utringbuffer_internalptr(a,j) ((void*)((a)->d + ((a)->icd.sz * (j))))
#define utringbuffer_eltptr(a,j) ((0 <= (j) && (j) < utringbuffer_len(a)) ? _utringbuffer_internalptr(a,_utringbuffer_real_idx(a,j)) : NULL)

#define _utringbuffer_fake_idx(a,j) ((a)->f ? ((j) + (a)->n - (a)->i) % (a)->n : (j))
#define _utringbuffer_internalidx(a,e) (((char*)(e) >= (a)->d) ? (((char*)(e) - (a)->d)/(a)->icd.sz) : -1)
#define utringbuffer_eltidx(a,e) _utringbuffer_fake_idx(a, _utringbuffer_internalidx(a,e))

#define utringbuffer_front(a) utringbuffer_eltptr(a,0)
#define utringbuffer_next(a,e) ((e)==NULL ? utringbuffer_front(a) : utringbuffer_eltptr(a, utringbuffer_eltidx(a,e)+1))
#define utringbuffer_prev(a,e) ((e)==NULL ? utringbuffer_back(a) : utringbuffer_eltptr(a, utringbuffer_eltidx(a,e)-1))
#define utringbuffer_back(a) (utringbuffer_empty(a) ? NULL : utringbuffer_eltptr(a, utringbuffer_len(a) - 1))

#endif /* UTRINGBUFFER_H */
