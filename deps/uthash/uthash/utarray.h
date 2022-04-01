/*
Copyright (c) 2008-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
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

/* a dynamic array implementation using macros
 */
#ifndef UTARRAY_H
#define UTARRAY_H

#define UTARRAY_VERSION 2.3.0

#include <stddef.h>  /* size_t */
#include <string.h>  /* memset, etc */
#include <stdlib.h>  /* exit */

#ifdef __GNUC__
#define UTARRAY_UNUSED __attribute__((__unused__))
#else
#define UTARRAY_UNUSED
#endif

#ifdef oom
#error "The name of macro 'oom' has been changed to 'utarray_oom'. Please update your code."
#define utarray_oom() oom()
#endif

#ifndef utarray_oom
#define utarray_oom() exit(-1)
#endif

typedef void (ctor_f)(void *dst, const void *src);
typedef void (dtor_f)(void *elt);
typedef void (init_f)(void *elt);
typedef struct {
    size_t sz;
    init_f *init;
    ctor_f *copy;
    dtor_f *dtor;
} UT_icd;

typedef struct {
    unsigned i,n;/* i: index of next available slot, n: num slots */
    UT_icd icd;  /* initializer, copy and destructor functions */
    char *d;     /* n slots of size icd->sz*/
} UT_array;

#define utarray_init(a,_icd) do {                                             \
  memset(a,0,sizeof(UT_array));                                               \
  (a)->icd = *(_icd);                                                         \
} while(0)

#define utarray_done(a) do {                                                  \
  if ((a)->n) {                                                               \
    if ((a)->icd.dtor) {                                                      \
      unsigned _ut_i;                                                         \
      for(_ut_i=0; _ut_i < (a)->i; _ut_i++) {                                 \
        (a)->icd.dtor(utarray_eltptr(a,_ut_i));                               \
      }                                                                       \
    }                                                                         \
    free((a)->d);                                                             \
  }                                                                           \
  (a)->n=0;                                                                   \
} while(0)

#define utarray_new(a,_icd) do {                                              \
  (a) = (UT_array*)malloc(sizeof(UT_array));                                  \
  if ((a) == NULL) {                                                          \
    utarray_oom();                                                            \
  }                                                                           \
  utarray_init(a,_icd);                                                       \
} while(0)

#define utarray_free(a) do {                                                  \
  utarray_done(a);                                                            \
  free(a);                                                                    \
} while(0)

#define utarray_reserve(a,by) do {                                            \
  if (((a)->i+(by)) > (a)->n) {                                               \
    char *utarray_tmp;                                                        \
    while (((a)->i+(by)) > (a)->n) { (a)->n = ((a)->n ? (2*(a)->n) : 8); }    \
    utarray_tmp=(char*)realloc((a)->d, (a)->n*(a)->icd.sz);                   \
    if (utarray_tmp == NULL) {                                                \
      utarray_oom();                                                          \
    }                                                                         \
    (a)->d=utarray_tmp;                                                       \
  }                                                                           \
} while(0)

#define utarray_push_back(a,p) do {                                           \
  utarray_reserve(a,1);                                                       \
  if ((a)->icd.copy) { (a)->icd.copy( _utarray_eltptr(a,(a)->i++), p); }      \
  else { memcpy(_utarray_eltptr(a,(a)->i++), p, (a)->icd.sz); };              \
} while(0)

#define utarray_pop_back(a) do {                                              \
  if ((a)->icd.dtor) { (a)->icd.dtor( _utarray_eltptr(a,--((a)->i))); }       \
  else { (a)->i--; }                                                          \
} while(0)

#define utarray_extend_back(a) do {                                           \
  utarray_reserve(a,1);                                                       \
  if ((a)->icd.init) { (a)->icd.init(_utarray_eltptr(a,(a)->i)); }            \
  else { memset(_utarray_eltptr(a,(a)->i),0,(a)->icd.sz); }                   \
  (a)->i++;                                                                   \
} while(0)

#define utarray_len(a) ((a)->i)

#define utarray_eltptr(a,j) (((j) < (a)->i) ? _utarray_eltptr(a,j) : NULL)
#define _utarray_eltptr(a,j) ((void*)((a)->d + ((a)->icd.sz * (j))))

#define utarray_insert(a,p,j) do {                                            \
  if ((j) > (a)->i) utarray_resize(a,j);                                      \
  utarray_reserve(a,1);                                                       \
  if ((j) < (a)->i) {                                                         \
    memmove( _utarray_eltptr(a,(j)+1), _utarray_eltptr(a,j),                  \
             ((a)->i - (j))*((a)->icd.sz));                                   \
  }                                                                           \
  if ((a)->icd.copy) { (a)->icd.copy( _utarray_eltptr(a,j), p); }             \
  else { memcpy(_utarray_eltptr(a,j), p, (a)->icd.sz); };                     \
  (a)->i++;                                                                   \
} while(0)

#define utarray_inserta(a,w,j) do {                                           \
  if (utarray_len(w) == 0) break;                                             \
  if ((j) > (a)->i) utarray_resize(a,j);                                      \
  utarray_reserve(a,utarray_len(w));                                          \
  if ((j) < (a)->i) {                                                         \
    memmove(_utarray_eltptr(a,(j)+utarray_len(w)),                            \
            _utarray_eltptr(a,j),                                             \
            ((a)->i - (j))*((a)->icd.sz));                                    \
  }                                                                           \
  if ((a)->icd.copy) {                                                        \
    unsigned _ut_i;                                                           \
    for(_ut_i=0;_ut_i<(w)->i;_ut_i++) {                                       \
      (a)->icd.copy(_utarray_eltptr(a, (j) + _ut_i), _utarray_eltptr(w, _ut_i)); \
    }                                                                         \
  } else {                                                                    \
    memcpy(_utarray_eltptr(a,j), _utarray_eltptr(w,0),                        \
           utarray_len(w)*((a)->icd.sz));                                     \
  }                                                                           \
  (a)->i += utarray_len(w);                                                   \
} while(0)

#define utarray_resize(dst,num) do {                                          \
  unsigned _ut_i;                                                             \
  if ((dst)->i > (unsigned)(num)) {                                           \
    if ((dst)->icd.dtor) {                                                    \
      for (_ut_i = (num); _ut_i < (dst)->i; ++_ut_i) {                        \
        (dst)->icd.dtor(_utarray_eltptr(dst, _ut_i));                         \
      }                                                                       \
    }                                                                         \
  } else if ((dst)->i < (unsigned)(num)) {                                    \
    utarray_reserve(dst, (num) - (dst)->i);                                   \
    if ((dst)->icd.init) {                                                    \
      for (_ut_i = (dst)->i; _ut_i < (unsigned)(num); ++_ut_i) {              \
        (dst)->icd.init(_utarray_eltptr(dst, _ut_i));                         \
      }                                                                       \
    } else {                                                                  \
      memset(_utarray_eltptr(dst, (dst)->i), 0, (dst)->icd.sz*((num) - (dst)->i)); \
    }                                                                         \
  }                                                                           \
  (dst)->i = (num);                                                           \
} while(0)

#define utarray_concat(dst,src) do {                                          \
  utarray_inserta(dst, src, utarray_len(dst));                                \
} while(0)

#define utarray_erase(a,pos,len) do {                                         \
  if ((a)->icd.dtor) {                                                        \
    unsigned _ut_i;                                                           \
    for (_ut_i = 0; _ut_i < (len); _ut_i++) {                                 \
      (a)->icd.dtor(utarray_eltptr(a, (pos) + _ut_i));                        \
    }                                                                         \
  }                                                                           \
  if ((a)->i > ((pos) + (len))) {                                             \
    memmove(_utarray_eltptr(a, pos), _utarray_eltptr(a, (pos) + (len)),       \
            ((a)->i - ((pos) + (len))) * (a)->icd.sz);                        \
  }                                                                           \
  (a)->i -= (len);                                                            \
} while(0)

#define utarray_renew(a,u) do {                                               \
  if (a) utarray_clear(a);                                                    \
  else utarray_new(a, u);                                                     \
} while(0)

#define utarray_clear(a) do {                                                 \
  if ((a)->i > 0) {                                                           \
    if ((a)->icd.dtor) {                                                      \
      unsigned _ut_i;                                                         \
      for(_ut_i=0; _ut_i < (a)->i; _ut_i++) {                                 \
        (a)->icd.dtor(_utarray_eltptr(a, _ut_i));                             \
      }                                                                       \
    }                                                                         \
    (a)->i = 0;                                                               \
  }                                                                           \
} while(0)

#define utarray_sort(a,cmp) do {                                              \
  qsort((a)->d, (a)->i, (a)->icd.sz, cmp);                                    \
} while(0)

#define utarray_find(a,v,cmp) bsearch((v),(a)->d,(a)->i,(a)->icd.sz,cmp)

#define utarray_front(a) (((a)->i) ? (_utarray_eltptr(a,0)) : NULL)
#define utarray_next(a,e) (((e)==NULL) ? utarray_front(a) : (((a)->i != utarray_eltidx(a,e)+1) ? _utarray_eltptr(a,utarray_eltidx(a,e)+1) : NULL))
#define utarray_prev(a,e) (((e)==NULL) ? utarray_back(a) : ((utarray_eltidx(a,e) != 0) ? _utarray_eltptr(a,utarray_eltidx(a,e)-1) : NULL))
#define utarray_back(a) (((a)->i) ? (_utarray_eltptr(a,(a)->i-1)) : NULL)
#define utarray_eltidx(a,e) (((char*)(e) - (a)->d) / (a)->icd.sz)

/* last we pre-define a few icd for common utarrays of ints and strings */
static void utarray_str_cpy(void *dst, const void *src) {
  char *const *srcc = (char *const *)src;
  char **dstc = (char**)dst;
  *dstc = (*srcc == NULL) ? NULL : strdup(*srcc);
}
static void utarray_str_dtor(void *elt) {
  char **eltc = (char**)elt;
  if (*eltc != NULL) free(*eltc);
}
static const UT_icd ut_str_icd UTARRAY_UNUSED = {sizeof(char*),NULL,utarray_str_cpy,utarray_str_dtor};
static const UT_icd ut_int_icd UTARRAY_UNUSED = {sizeof(int),NULL,NULL,NULL};
static const UT_icd ut_ptr_icd UTARRAY_UNUSED = {sizeof(void*),NULL,NULL,NULL};


#endif /* UTARRAY_H */
