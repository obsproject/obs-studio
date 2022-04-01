/*
Copyright (c) 2007-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
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

#ifndef UTLIST_H
#define UTLIST_H

#define UTLIST_VERSION 2.3.0

#include <assert.h>

/*
 * This file contains macros to manipulate singly and doubly-linked lists.
 *
 * 1. LL_ macros:  singly-linked lists.
 * 2. DL_ macros:  doubly-linked lists.
 * 3. CDL_ macros: circular doubly-linked lists.
 *
 * To use singly-linked lists, your structure must have a "next" pointer.
 * To use doubly-linked lists, your structure must "prev" and "next" pointers.
 * Either way, the pointer to the head of the list must be initialized to NULL.
 *
 * ----------------.EXAMPLE -------------------------
 * struct item {
 *      int id;
 *      struct item *prev, *next;
 * }
 *
 * struct item *list = NULL:
 *
 * int main() {
 *      struct item *item;
 *      ... allocate and populate item ...
 *      DL_APPEND(list, item);
 * }
 * --------------------------------------------------
 *
 * For doubly-linked lists, the append and delete macros are O(1)
 * For singly-linked lists, append and delete are O(n) but prepend is O(1)
 * The sort macro is O(n log(n)) for all types of single/double/circular lists.
 */

/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ source) this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#if !defined(LDECLTYPE) && !defined(NO_DECLTYPE)
#if defined(_MSC_VER)   /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define LDECLTYPE(x) decltype(x)
#else                   /* VS2008 or older (or VS2010 in C mode) */
#define NO_DECLTYPE
#endif
#elif defined(__BORLANDC__) || defined(__ICCARM__) || defined(__LCC__) || defined(__WATCOMC__)
#define NO_DECLTYPE
#else                   /* GNU, Sun and other compilers */
#define LDECLTYPE(x) __typeof(x)
#endif
#endif

/* for VS2008 we use some workarounds to get around the lack of decltype,
 * namely, we always reassign our tmp variable to the list head if we need
 * to dereference its prev/next pointers, and save/restore the real head.*/
#ifdef NO_DECLTYPE
#define IF_NO_DECLTYPE(x) x
#define LDECLTYPE(x) char*
#define UTLIST_SV(elt,list) _tmp = (char*)(list); {char **_alias = (char**)&(list); *_alias = (elt); }
#define UTLIST_NEXT(elt,list,next) ((char*)((list)->next))
#define UTLIST_NEXTASGN(elt,list,to,next) { char **_alias = (char**)&((list)->next); *_alias=(char*)(to); }
/* #define UTLIST_PREV(elt,list,prev) ((char*)((list)->prev)) */
#define UTLIST_PREVASGN(elt,list,to,prev) { char **_alias = (char**)&((list)->prev); *_alias=(char*)(to); }
#define UTLIST_RS(list) { char **_alias = (char**)&(list); *_alias=_tmp; }
#define UTLIST_CASTASGN(a,b) { char **_alias = (char**)&(a); *_alias=(char*)(b); }
#else
#define IF_NO_DECLTYPE(x)
#define UTLIST_SV(elt,list)
#define UTLIST_NEXT(elt,list,next) ((elt)->next)
#define UTLIST_NEXTASGN(elt,list,to,next) ((elt)->next)=(to)
/* #define UTLIST_PREV(elt,list,prev) ((elt)->prev) */
#define UTLIST_PREVASGN(elt,list,to,prev) ((elt)->prev)=(to)
#define UTLIST_RS(list)
#define UTLIST_CASTASGN(a,b) (a)=(b)
#endif

/******************************************************************************
 * The sort macro is an adaptation of Simon Tatham's O(n log(n)) mergesort    *
 * Unwieldy variable names used here to avoid shadowing passed-in variables.  *
 *****************************************************************************/
#define LL_SORT(list, cmp)                                                                     \
    LL_SORT2(list, cmp, next)

#define LL_SORT2(list, cmp, next)                                                              \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  IF_NO_DECLTYPE(LDECLTYPE(list) _tmp;)                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      UTLIST_CASTASGN(_ls_p,list);                                                             \
      (list) = NULL;                                                                           \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          UTLIST_SV(_ls_q,list); _ls_q = UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list);        \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; UTLIST_SV(_ls_q,list); _ls_q =                                      \
              UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list); _ls_qsize--;                      \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; UTLIST_SV(_ls_p,list); _ls_p =                                      \
              UTLIST_NEXT(_ls_p,list,next); UTLIST_RS(list); _ls_psize--;                      \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; UTLIST_SV(_ls_p,list); _ls_p =                                      \
              UTLIST_NEXT(_ls_p,list,next); UTLIST_RS(list); _ls_psize--;                      \
          } else {                                                                             \
            _ls_e = _ls_q; UTLIST_SV(_ls_q,list); _ls_q =                                      \
              UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list); _ls_qsize--;                      \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            UTLIST_SV(_ls_tail,list); UTLIST_NEXTASGN(_ls_tail,list,_ls_e,next); UTLIST_RS(list); \
          } else {                                                                             \
            UTLIST_CASTASGN(list,_ls_e);                                                       \
          }                                                                                    \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      if (_ls_tail) {                                                                          \
        UTLIST_SV(_ls_tail,list); UTLIST_NEXTASGN(_ls_tail,list,NULL,next); UTLIST_RS(list);   \
      }                                                                                        \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  }                                                                                            \
} while (0)


#define DL_SORT(list, cmp)                                                                     \
    DL_SORT2(list, cmp, prev, next)

#define DL_SORT2(list, cmp, prev, next)                                                        \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  IF_NO_DECLTYPE(LDECLTYPE(list) _tmp;)                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      UTLIST_CASTASGN(_ls_p,list);                                                             \
      (list) = NULL;                                                                           \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          UTLIST_SV(_ls_q,list); _ls_q = UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list);        \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while ((_ls_psize > 0) || ((_ls_qsize > 0) && _ls_q)) {                                \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; UTLIST_SV(_ls_q,list); _ls_q =                                      \
              UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list); _ls_qsize--;                      \
          } else if ((_ls_qsize == 0) || (!_ls_q)) {                                           \
            _ls_e = _ls_p; UTLIST_SV(_ls_p,list); _ls_p =                                      \
              UTLIST_NEXT(_ls_p,list,next); UTLIST_RS(list); _ls_psize--;                      \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; UTLIST_SV(_ls_p,list); _ls_p =                                      \
              UTLIST_NEXT(_ls_p,list,next); UTLIST_RS(list); _ls_psize--;                      \
          } else {                                                                             \
            _ls_e = _ls_q; UTLIST_SV(_ls_q,list); _ls_q =                                      \
              UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list); _ls_qsize--;                      \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            UTLIST_SV(_ls_tail,list); UTLIST_NEXTASGN(_ls_tail,list,_ls_e,next); UTLIST_RS(list); \
          } else {                                                                             \
            UTLIST_CASTASGN(list,_ls_e);                                                       \
          }                                                                                    \
          UTLIST_SV(_ls_e,list); UTLIST_PREVASGN(_ls_e,list,_ls_tail,prev); UTLIST_RS(list);   \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      UTLIST_CASTASGN((list)->prev, _ls_tail);                                                 \
      UTLIST_SV(_ls_tail,list); UTLIST_NEXTASGN(_ls_tail,list,NULL,next); UTLIST_RS(list);     \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  }                                                                                            \
} while (0)

#define CDL_SORT(list, cmp)                                                                    \
    CDL_SORT2(list, cmp, prev, next)

#define CDL_SORT2(list, cmp, prev, next)                                                       \
do {                                                                                           \
  LDECLTYPE(list) _ls_p;                                                                       \
  LDECLTYPE(list) _ls_q;                                                                       \
  LDECLTYPE(list) _ls_e;                                                                       \
  LDECLTYPE(list) _ls_tail;                                                                    \
  LDECLTYPE(list) _ls_oldhead;                                                                 \
  LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      UTLIST_CASTASGN(_ls_p,list);                                                             \
      UTLIST_CASTASGN(_ls_oldhead,list);                                                       \
      (list) = NULL;                                                                           \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          UTLIST_SV(_ls_q,list);                                                               \
          if (UTLIST_NEXT(_ls_q,list,next) == _ls_oldhead) {                                   \
            _ls_q = NULL;                                                                      \
          } else {                                                                             \
            _ls_q = UTLIST_NEXT(_ls_q,list,next);                                              \
          }                                                                                    \
          UTLIST_RS(list);                                                                     \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; UTLIST_SV(_ls_q,list); _ls_q =                                      \
              UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list); _ls_qsize--;                      \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; UTLIST_SV(_ls_p,list); _ls_p =                                      \
              UTLIST_NEXT(_ls_p,list,next); UTLIST_RS(list); _ls_psize--;                      \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; UTLIST_SV(_ls_p,list); _ls_p =                                      \
              UTLIST_NEXT(_ls_p,list,next); UTLIST_RS(list); _ls_psize--;                      \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else {                                                                             \
            _ls_e = _ls_q; UTLIST_SV(_ls_q,list); _ls_q =                                      \
              UTLIST_NEXT(_ls_q,list,next); UTLIST_RS(list); _ls_qsize--;                      \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            UTLIST_SV(_ls_tail,list); UTLIST_NEXTASGN(_ls_tail,list,_ls_e,next); UTLIST_RS(list); \
          } else {                                                                             \
            UTLIST_CASTASGN(list,_ls_e);                                                       \
          }                                                                                    \
          UTLIST_SV(_ls_e,list); UTLIST_PREVASGN(_ls_e,list,_ls_tail,prev); UTLIST_RS(list);   \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      UTLIST_CASTASGN((list)->prev,_ls_tail);                                                  \
      UTLIST_CASTASGN(_tmp,list);                                                              \
      UTLIST_SV(_ls_tail,list); UTLIST_NEXTASGN(_ls_tail,list,_tmp,next); UTLIST_RS(list);     \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  }                                                                                            \
} while (0)

/******************************************************************************
 * singly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define LL_PREPEND(head,add)                                                                   \
    LL_PREPEND2(head,add,next)

#define LL_PREPEND2(head,add,next)                                                             \
do {                                                                                           \
  (add)->next = (head);                                                                        \
  (head) = (add);                                                                              \
} while (0)

#define LL_CONCAT(head1,head2)                                                                 \
    LL_CONCAT2(head1,head2,next)

#define LL_CONCAT2(head1,head2,next)                                                           \
do {                                                                                           \
  LDECLTYPE(head1) _tmp;                                                                       \
  if (head1) {                                                                                 \
    _tmp = (head1);                                                                            \
    while (_tmp->next) { _tmp = _tmp->next; }                                                  \
    _tmp->next=(head2);                                                                        \
  } else {                                                                                     \
    (head1)=(head2);                                                                           \
  }                                                                                            \
} while (0)

#define LL_APPEND(head,add)                                                                    \
    LL_APPEND2(head,add,next)

#define LL_APPEND2(head,add,next)                                                              \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  (add)->next=NULL;                                                                            \
  if (head) {                                                                                  \
    _tmp = (head);                                                                             \
    while (_tmp->next) { _tmp = _tmp->next; }                                                  \
    _tmp->next=(add);                                                                          \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
} while (0)

#define LL_INSERT_INORDER(head,add,cmp)                                                        \
    LL_INSERT_INORDER2(head,add,cmp,next)

#define LL_INSERT_INORDER2(head,add,cmp,next)                                                  \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  if (head) {                                                                                  \
    LL_LOWER_BOUND2(head, _tmp, add, cmp, next);                                               \
    LL_APPEND_ELEM2(head, _tmp, add, next);                                                    \
  } else {                                                                                     \
    (head) = (add);                                                                            \
    (head)->next = NULL;                                                                       \
  }                                                                                            \
} while (0)

#define LL_LOWER_BOUND(head,elt,like,cmp)                                                      \
    LL_LOWER_BOUND2(head,elt,like,cmp,next)

#define LL_LOWER_BOUND2(head,elt,like,cmp,next)                                                \
  do {                                                                                         \
    if ((head) == NULL || (cmp(head, like)) >= 0) {                                            \
      (elt) = NULL;                                                                            \
    } else {                                                                                   \
      for ((elt) = (head); (elt)->next != NULL; (elt) = (elt)->next) {                         \
        if (cmp((elt)->next, like) >= 0) {                                                     \
          break;                                                                               \
        }                                                                                      \
      }                                                                                        \
    }                                                                                          \
  } while (0)

#define LL_DELETE(head,del)                                                                    \
    LL_DELETE2(head,del,next)

#define LL_DELETE2(head,del,next)                                                              \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->next;                                                                       \
  } else {                                                                                     \
    _tmp = (head);                                                                             \
    while (_tmp->next && (_tmp->next != (del))) {                                              \
      _tmp = _tmp->next;                                                                       \
    }                                                                                          \
    if (_tmp->next) {                                                                          \
      _tmp->next = (del)->next;                                                                \
    }                                                                                          \
  }                                                                                            \
} while (0)

#define LL_COUNT(head,el,counter)                                                              \
    LL_COUNT2(head,el,counter,next)                                                            \

#define LL_COUNT2(head,el,counter,next)                                                        \
do {                                                                                           \
  (counter) = 0;                                                                               \
  LL_FOREACH2(head,el,next) { ++(counter); }                                                   \
} while (0)

#define LL_FOREACH(head,el)                                                                    \
    LL_FOREACH2(head,el,next)

#define LL_FOREACH2(head,el,next)                                                              \
    for ((el) = (head); el; (el) = (el)->next)

#define LL_FOREACH_SAFE(head,el,tmp)                                                           \
    LL_FOREACH_SAFE2(head,el,tmp,next)

#define LL_FOREACH_SAFE2(head,el,tmp,next)                                                     \
  for ((el) = (head); (el) && ((tmp) = (el)->next, 1); (el) = (tmp))

#define LL_SEARCH_SCALAR(head,out,field,val)                                                   \
    LL_SEARCH_SCALAR2(head,out,field,val,next)

#define LL_SEARCH_SCALAR2(head,out,field,val,next)                                             \
do {                                                                                           \
    LL_FOREACH2(head,out,next) {                                                               \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while (0)

#define LL_SEARCH(head,out,elt,cmp)                                                            \
    LL_SEARCH2(head,out,elt,cmp,next)

#define LL_SEARCH2(head,out,elt,cmp,next)                                                      \
do {                                                                                           \
    LL_FOREACH2(head,out,next) {                                                               \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while (0)

#define LL_REPLACE_ELEM2(head, el, add, next)                                                  \
do {                                                                                           \
 LDECLTYPE(head) _tmp;                                                                         \
 assert((head) != NULL);                                                                       \
 assert((el) != NULL);                                                                         \
 assert((add) != NULL);                                                                        \
 (add)->next = (el)->next;                                                                     \
 if ((head) == (el)) {                                                                         \
  (head) = (add);                                                                              \
 } else {                                                                                      \
  _tmp = (head);                                                                               \
  while (_tmp->next && (_tmp->next != (el))) {                                                 \
   _tmp = _tmp->next;                                                                          \
  }                                                                                            \
  if (_tmp->next) {                                                                            \
    _tmp->next = (add);                                                                        \
  }                                                                                            \
 }                                                                                             \
} while (0)

#define LL_REPLACE_ELEM(head, el, add)                                                         \
    LL_REPLACE_ELEM2(head, el, add, next)

#define LL_PREPEND_ELEM2(head, el, add, next)                                                  \
do {                                                                                           \
 if (el) {                                                                                     \
  LDECLTYPE(head) _tmp;                                                                        \
  assert((head) != NULL);                                                                      \
  assert((add) != NULL);                                                                       \
  (add)->next = (el);                                                                          \
  if ((head) == (el)) {                                                                        \
   (head) = (add);                                                                             \
  } else {                                                                                     \
   _tmp = (head);                                                                              \
   while (_tmp->next && (_tmp->next != (el))) {                                                \
    _tmp = _tmp->next;                                                                         \
   }                                                                                           \
   if (_tmp->next) {                                                                           \
     _tmp->next = (add);                                                                       \
   }                                                                                           \
  }                                                                                            \
 } else {                                                                                      \
  LL_APPEND2(head, add, next);                                                                 \
 }                                                                                             \
} while (0)                                                                                    \

#define LL_PREPEND_ELEM(head, el, add)                                                         \
    LL_PREPEND_ELEM2(head, el, add, next)

#define LL_APPEND_ELEM2(head, el, add, next)                                                   \
do {                                                                                           \
 if (el) {                                                                                     \
  assert((head) != NULL);                                                                      \
  assert((add) != NULL);                                                                       \
  (add)->next = (el)->next;                                                                    \
  (el)->next = (add);                                                                          \
 } else {                                                                                      \
  LL_PREPEND2(head, add, next);                                                                \
 }                                                                                             \
} while (0)                                                                                    \

#define LL_APPEND_ELEM(head, el, add)                                                          \
    LL_APPEND_ELEM2(head, el, add, next)

#ifdef NO_DECLTYPE
/* Here are VS2008 / NO_DECLTYPE replacements for a few functions */

#undef LL_CONCAT2
#define LL_CONCAT2(head1,head2,next)                                                           \
do {                                                                                           \
  char *_tmp;                                                                                  \
  if (head1) {                                                                                 \
    _tmp = (char*)(head1);                                                                     \
    while ((head1)->next) { (head1) = (head1)->next; }                                         \
    (head1)->next = (head2);                                                                   \
    UTLIST_RS(head1);                                                                          \
  } else {                                                                                     \
    (head1)=(head2);                                                                           \
  }                                                                                            \
} while (0)

#undef LL_APPEND2
#define LL_APPEND2(head,add,next)                                                              \
do {                                                                                           \
  if (head) {                                                                                  \
    (add)->next = head;     /* use add->next as a temp variable */                             \
    while ((add)->next->next) { (add)->next = (add)->next->next; }                             \
    (add)->next->next=(add);                                                                   \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
  (add)->next=NULL;                                                                            \
} while (0)

#undef LL_INSERT_INORDER2
#define LL_INSERT_INORDER2(head,add,cmp,next)                                                  \
do {                                                                                           \
  if ((head) == NULL || (cmp(head, add)) >= 0) {                                               \
    (add)->next = (head);                                                                      \
    (head) = (add);                                                                            \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while ((head)->next != NULL && (cmp((head)->next, add)) < 0) {                             \
      (head) = (head)->next;                                                                   \
    }                                                                                          \
    (add)->next = (head)->next;                                                                \
    (head)->next = (add);                                                                      \
    UTLIST_RS(head);                                                                           \
  }                                                                                            \
} while (0)

#undef LL_DELETE2
#define LL_DELETE2(head,del,next)                                                              \
do {                                                                                           \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->next;                                                                       \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while ((head)->next && ((head)->next != (del))) {                                          \
      (head) = (head)->next;                                                                   \
    }                                                                                          \
    if ((head)->next) {                                                                        \
      (head)->next = ((del)->next);                                                            \
    }                                                                                          \
    UTLIST_RS(head);                                                                           \
  }                                                                                            \
} while (0)

#undef LL_REPLACE_ELEM2
#define LL_REPLACE_ELEM2(head, el, add, next)                                                  \
do {                                                                                           \
  assert((head) != NULL);                                                                      \
  assert((el) != NULL);                                                                        \
  assert((add) != NULL);                                                                       \
  if ((head) == (el)) {                                                                        \
    (head) = (add);                                                                            \
  } else {                                                                                     \
    (add)->next = head;                                                                        \
    while ((add)->next->next && ((add)->next->next != (el))) {                                 \
      (add)->next = (add)->next->next;                                                         \
    }                                                                                          \
    if ((add)->next->next) {                                                                   \
      (add)->next->next = (add);                                                               \
    }                                                                                          \
  }                                                                                            \
  (add)->next = (el)->next;                                                                    \
} while (0)

#undef LL_PREPEND_ELEM2
#define LL_PREPEND_ELEM2(head, el, add, next)                                                  \
do {                                                                                           \
  if (el) {                                                                                    \
    assert((head) != NULL);                                                                    \
    assert((add) != NULL);                                                                     \
    if ((head) == (el)) {                                                                      \
      (head) = (add);                                                                          \
    } else {                                                                                   \
      (add)->next = (head);                                                                    \
      while ((add)->next->next && ((add)->next->next != (el))) {                               \
        (add)->next = (add)->next->next;                                                       \
      }                                                                                        \
      if ((add)->next->next) {                                                                 \
        (add)->next->next = (add);                                                             \
      }                                                                                        \
    }                                                                                          \
    (add)->next = (el);                                                                        \
  } else {                                                                                     \
    LL_APPEND2(head, add, next);                                                               \
  }                                                                                            \
} while (0)                                                                                    \

#endif /* NO_DECLTYPE */

/******************************************************************************
 * doubly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define DL_PREPEND(head,add)                                                                   \
    DL_PREPEND2(head,add,prev,next)

#define DL_PREPEND2(head,add,prev,next)                                                        \
do {                                                                                           \
 (add)->next = (head);                                                                         \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (head)->prev = (add);                                                                       \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
 }                                                                                             \
 (head) = (add);                                                                               \
} while (0)

#define DL_APPEND(head,add)                                                                    \
    DL_APPEND2(head,add,prev,next)

#define DL_APPEND2(head,add,prev,next)                                                         \
do {                                                                                           \
  if (head) {                                                                                  \
      (add)->prev = (head)->prev;                                                              \
      (head)->prev->next = (add);                                                              \
      (head)->prev = (add);                                                                    \
      (add)->next = NULL;                                                                      \
  } else {                                                                                     \
      (head)=(add);                                                                            \
      (head)->prev = (head);                                                                   \
      (head)->next = NULL;                                                                     \
  }                                                                                            \
} while (0)

#define DL_INSERT_INORDER(head,add,cmp)                                                        \
    DL_INSERT_INORDER2(head,add,cmp,prev,next)

#define DL_INSERT_INORDER2(head,add,cmp,prev,next)                                             \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  if (head) {                                                                                  \
    DL_LOWER_BOUND2(head, _tmp, add, cmp, next);                                               \
    DL_APPEND_ELEM2(head, _tmp, add, prev, next);                                              \
  } else {                                                                                     \
    (head) = (add);                                                                            \
    (head)->prev = (head);                                                                     \
    (head)->next = NULL;                                                                       \
  }                                                                                            \
} while (0)

#define DL_LOWER_BOUND(head,elt,like,cmp)                                                      \
    DL_LOWER_BOUND2(head,elt,like,cmp,next)

#define DL_LOWER_BOUND2(head,elt,like,cmp,next)                                                \
do {                                                                                           \
  if ((head) == NULL || (cmp(head, like)) >= 0) {                                              \
    (elt) = NULL;                                                                              \
  } else {                                                                                     \
    for ((elt) = (head); (elt)->next != NULL; (elt) = (elt)->next) {                           \
      if ((cmp((elt)->next, like)) >= 0) {                                                     \
        break;                                                                                 \
      }                                                                                        \
    }                                                                                          \
  }                                                                                            \
} while (0)

#define DL_CONCAT(head1,head2)                                                                 \
    DL_CONCAT2(head1,head2,prev,next)

#define DL_CONCAT2(head1,head2,prev,next)                                                      \
do {                                                                                           \
  LDECLTYPE(head1) _tmp;                                                                       \
  if (head2) {                                                                                 \
    if (head1) {                                                                               \
        UTLIST_CASTASGN(_tmp, (head2)->prev);                                                  \
        (head2)->prev = (head1)->prev;                                                         \
        (head1)->prev->next = (head2);                                                         \
        UTLIST_CASTASGN((head1)->prev, _tmp);                                                  \
    } else {                                                                                   \
        (head1)=(head2);                                                                       \
    }                                                                                          \
  }                                                                                            \
} while (0)

#define DL_DELETE(head,del)                                                                    \
    DL_DELETE2(head,del,prev,next)

#define DL_DELETE2(head,del,prev,next)                                                         \
do {                                                                                           \
  assert((head) != NULL);                                                                      \
  assert((del)->prev != NULL);                                                                 \
  if ((del)->prev == (del)) {                                                                  \
      (head)=NULL;                                                                             \
  } else if ((del)==(head)) {                                                                  \
      (del)->next->prev = (del)->prev;                                                         \
      (head) = (del)->next;                                                                    \
  } else {                                                                                     \
      (del)->prev->next = (del)->next;                                                         \
      if ((del)->next) {                                                                       \
          (del)->next->prev = (del)->prev;                                                     \
      } else {                                                                                 \
          (head)->prev = (del)->prev;                                                          \
      }                                                                                        \
  }                                                                                            \
} while (0)

#define DL_COUNT(head,el,counter)                                                              \
    DL_COUNT2(head,el,counter,next)                                                            \

#define DL_COUNT2(head,el,counter,next)                                                        \
do {                                                                                           \
  (counter) = 0;                                                                               \
  DL_FOREACH2(head,el,next) { ++(counter); }                                                   \
} while (0)

#define DL_FOREACH(head,el)                                                                    \
    DL_FOREACH2(head,el,next)

#define DL_FOREACH2(head,el,next)                                                              \
    for ((el) = (head); el; (el) = (el)->next)

/* this version is safe for deleting the elements during iteration */
#define DL_FOREACH_SAFE(head,el,tmp)                                                           \
    DL_FOREACH_SAFE2(head,el,tmp,next)

#define DL_FOREACH_SAFE2(head,el,tmp,next)                                                     \
  for ((el) = (head); (el) && ((tmp) = (el)->next, 1); (el) = (tmp))

/* these are identical to their singly-linked list counterparts */
#define DL_SEARCH_SCALAR LL_SEARCH_SCALAR
#define DL_SEARCH LL_SEARCH
#define DL_SEARCH_SCALAR2 LL_SEARCH_SCALAR2
#define DL_SEARCH2 LL_SEARCH2

#define DL_REPLACE_ELEM2(head, el, add, prev, next)                                            \
do {                                                                                           \
 assert((head) != NULL);                                                                       \
 assert((el) != NULL);                                                                         \
 assert((add) != NULL);                                                                        \
 if ((head) == (el)) {                                                                         \
  (head) = (add);                                                                              \
  (add)->next = (el)->next;                                                                    \
  if ((el)->next == NULL) {                                                                    \
   (add)->prev = (add);                                                                        \
  } else {                                                                                     \
   (add)->prev = (el)->prev;                                                                   \
   (add)->next->prev = (add);                                                                  \
  }                                                                                            \
 } else {                                                                                      \
  (add)->next = (el)->next;                                                                    \
  (add)->prev = (el)->prev;                                                                    \
  (add)->prev->next = (add);                                                                   \
  if ((el)->next == NULL) {                                                                    \
   (head)->prev = (add);                                                                       \
  } else {                                                                                     \
   (add)->next->prev = (add);                                                                  \
  }                                                                                            \
 }                                                                                             \
} while (0)

#define DL_REPLACE_ELEM(head, el, add)                                                         \
    DL_REPLACE_ELEM2(head, el, add, prev, next)

#define DL_PREPEND_ELEM2(head, el, add, prev, next)                                            \
do {                                                                                           \
 if (el) {                                                                                     \
  assert((head) != NULL);                                                                      \
  assert((add) != NULL);                                                                       \
  (add)->next = (el);                                                                          \
  (add)->prev = (el)->prev;                                                                    \
  (el)->prev = (add);                                                                          \
  if ((head) == (el)) {                                                                        \
   (head) = (add);                                                                             \
  } else {                                                                                     \
   (add)->prev->next = (add);                                                                  \
  }                                                                                            \
 } else {                                                                                      \
  DL_APPEND2(head, add, prev, next);                                                           \
 }                                                                                             \
} while (0)                                                                                    \

#define DL_PREPEND_ELEM(head, el, add)                                                         \
    DL_PREPEND_ELEM2(head, el, add, prev, next)

#define DL_APPEND_ELEM2(head, el, add, prev, next)                                             \
do {                                                                                           \
 if (el) {                                                                                     \
  assert((head) != NULL);                                                                      \
  assert((add) != NULL);                                                                       \
  (add)->next = (el)->next;                                                                    \
  (add)->prev = (el);                                                                          \
  (el)->next = (add);                                                                          \
  if ((add)->next) {                                                                           \
   (add)->next->prev = (add);                                                                  \
  } else {                                                                                     \
   (head)->prev = (add);                                                                       \
  }                                                                                            \
 } else {                                                                                      \
  DL_PREPEND2(head, add, prev, next);                                                          \
 }                                                                                             \
} while (0)                                                                                    \

#define DL_APPEND_ELEM(head, el, add)                                                          \
   DL_APPEND_ELEM2(head, el, add, prev, next)

#ifdef NO_DECLTYPE
/* Here are VS2008 / NO_DECLTYPE replacements for a few functions */

#undef DL_INSERT_INORDER2
#define DL_INSERT_INORDER2(head,add,cmp,prev,next)                                             \
do {                                                                                           \
  if ((head) == NULL) {                                                                        \
    (add)->prev = (add);                                                                       \
    (add)->next = NULL;                                                                        \
    (head) = (add);                                                                            \
  } else if ((cmp(head, add)) >= 0) {                                                          \
    (add)->prev = (head)->prev;                                                                \
    (add)->next = (head);                                                                      \
    (head)->prev = (add);                                                                      \
    (head) = (add);                                                                            \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while ((head)->next && (cmp((head)->next, add)) < 0) {                                     \
      (head) = (head)->next;                                                                   \
    }                                                                                          \
    (add)->prev = (head);                                                                      \
    (add)->next = (head)->next;                                                                \
    (head)->next = (add);                                                                      \
    UTLIST_RS(head);                                                                           \
    if ((add)->next) {                                                                         \
      (add)->next->prev = (add);                                                               \
    } else {                                                                                   \
      (head)->prev = (add);                                                                    \
    }                                                                                          \
  }                                                                                            \
} while (0)
#endif /* NO_DECLTYPE */

/******************************************************************************
 * circular doubly linked list macros                                         *
 *****************************************************************************/
#define CDL_APPEND(head,add)                                                                   \
    CDL_APPEND2(head,add,prev,next)

#define CDL_APPEND2(head,add,prev,next)                                                        \
do {                                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (add)->next = (head);                                                                       \
   (head)->prev = (add);                                                                       \
   (add)->prev->next = (add);                                                                  \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
   (add)->next = (add);                                                                        \
   (head) = (add);                                                                             \
 }                                                                                             \
} while (0)

#define CDL_PREPEND(head,add)                                                                  \
    CDL_PREPEND2(head,add,prev,next)

#define CDL_PREPEND2(head,add,prev,next)                                                       \
do {                                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (add)->next = (head);                                                                       \
   (head)->prev = (add);                                                                       \
   (add)->prev->next = (add);                                                                  \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
   (add)->next = (add);                                                                        \
 }                                                                                             \
 (head) = (add);                                                                               \
} while (0)

#define CDL_INSERT_INORDER(head,add,cmp)                                                       \
    CDL_INSERT_INORDER2(head,add,cmp,prev,next)

#define CDL_INSERT_INORDER2(head,add,cmp,prev,next)                                            \
do {                                                                                           \
  LDECLTYPE(head) _tmp;                                                                        \
  if (head) {                                                                                  \
    CDL_LOWER_BOUND2(head, _tmp, add, cmp, next);                                              \
    CDL_APPEND_ELEM2(head, _tmp, add, prev, next);                                             \
  } else {                                                                                     \
    (head) = (add);                                                                            \
    (head)->next = (head);                                                                     \
    (head)->prev = (head);                                                                     \
  }                                                                                            \
} while (0)

#define CDL_LOWER_BOUND(head,elt,like,cmp)                                                     \
    CDL_LOWER_BOUND2(head,elt,like,cmp,next)

#define CDL_LOWER_BOUND2(head,elt,like,cmp,next)                                               \
do {                                                                                           \
  if ((head) == NULL || (cmp(head, like)) >= 0) {                                              \
    (elt) = NULL;                                                                              \
  } else {                                                                                     \
    for ((elt) = (head); (elt)->next != (head); (elt) = (elt)->next) {                         \
      if ((cmp((elt)->next, like)) >= 0) {                                                     \
        break;                                                                                 \
      }                                                                                        \
    }                                                                                          \
  }                                                                                            \
} while (0)

#define CDL_DELETE(head,del)                                                                   \
    CDL_DELETE2(head,del,prev,next)

#define CDL_DELETE2(head,del,prev,next)                                                        \
do {                                                                                           \
  if (((head)==(del)) && ((head)->next == (head))) {                                           \
      (head) = NULL;                                                                           \
  } else {                                                                                     \
     (del)->next->prev = (del)->prev;                                                          \
     (del)->prev->next = (del)->next;                                                          \
     if ((del) == (head)) (head)=(del)->next;                                                  \
  }                                                                                            \
} while (0)

#define CDL_COUNT(head,el,counter)                                                             \
    CDL_COUNT2(head,el,counter,next)                                                           \

#define CDL_COUNT2(head, el, counter,next)                                                     \
do {                                                                                           \
  (counter) = 0;                                                                               \
  CDL_FOREACH2(head,el,next) { ++(counter); }                                                  \
} while (0)

#define CDL_FOREACH(head,el)                                                                   \
    CDL_FOREACH2(head,el,next)

#define CDL_FOREACH2(head,el,next)                                                             \
    for ((el)=(head);el;(el)=(((el)->next==(head)) ? NULL : (el)->next))

#define CDL_FOREACH_SAFE(head,el,tmp1,tmp2)                                                    \
    CDL_FOREACH_SAFE2(head,el,tmp1,tmp2,prev,next)

#define CDL_FOREACH_SAFE2(head,el,tmp1,tmp2,prev,next)                                         \
  for ((el) = (head), (tmp1) = (head) ? (head)->prev : NULL;                                   \
       (el) && ((tmp2) = (el)->next, 1);                                                       \
       (el) = ((el) == (tmp1) ? NULL : (tmp2)))

#define CDL_SEARCH_SCALAR(head,out,field,val)                                                  \
    CDL_SEARCH_SCALAR2(head,out,field,val,next)

#define CDL_SEARCH_SCALAR2(head,out,field,val,next)                                            \
do {                                                                                           \
    CDL_FOREACH2(head,out,next) {                                                              \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while (0)

#define CDL_SEARCH(head,out,elt,cmp)                                                           \
    CDL_SEARCH2(head,out,elt,cmp,next)

#define CDL_SEARCH2(head,out,elt,cmp,next)                                                     \
do {                                                                                           \
    CDL_FOREACH2(head,out,next) {                                                              \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while (0)

#define CDL_REPLACE_ELEM2(head, el, add, prev, next)                                           \
do {                                                                                           \
 assert((head) != NULL);                                                                       \
 assert((el) != NULL);                                                                         \
 assert((add) != NULL);                                                                        \
 if ((el)->next == (el)) {                                                                     \
  (add)->next = (add);                                                                         \
  (add)->prev = (add);                                                                         \
  (head) = (add);                                                                              \
 } else {                                                                                      \
  (add)->next = (el)->next;                                                                    \
  (add)->prev = (el)->prev;                                                                    \
  (add)->next->prev = (add);                                                                   \
  (add)->prev->next = (add);                                                                   \
  if ((head) == (el)) {                                                                        \
   (head) = (add);                                                                             \
  }                                                                                            \
 }                                                                                             \
} while (0)

#define CDL_REPLACE_ELEM(head, el, add)                                                        \
    CDL_REPLACE_ELEM2(head, el, add, prev, next)

#define CDL_PREPEND_ELEM2(head, el, add, prev, next)                                           \
do {                                                                                           \
  if (el) {                                                                                    \
    assert((head) != NULL);                                                                    \
    assert((add) != NULL);                                                                     \
    (add)->next = (el);                                                                        \
    (add)->prev = (el)->prev;                                                                  \
    (el)->prev = (add);                                                                        \
    (add)->prev->next = (add);                                                                 \
    if ((head) == (el)) {                                                                      \
      (head) = (add);                                                                          \
    }                                                                                          \
  } else {                                                                                     \
    CDL_APPEND2(head, add, prev, next);                                                        \
  }                                                                                            \
} while (0)

#define CDL_PREPEND_ELEM(head, el, add)                                                        \
    CDL_PREPEND_ELEM2(head, el, add, prev, next)

#define CDL_APPEND_ELEM2(head, el, add, prev, next)                                            \
do {                                                                                           \
 if (el) {                                                                                     \
  assert((head) != NULL);                                                                      \
  assert((add) != NULL);                                                                       \
  (add)->next = (el)->next;                                                                    \
  (add)->prev = (el);                                                                          \
  (el)->next = (add);                                                                          \
  (add)->next->prev = (add);                                                                   \
 } else {                                                                                      \
  CDL_PREPEND2(head, add, prev, next);                                                         \
 }                                                                                             \
} while (0)

#define CDL_APPEND_ELEM(head, el, add)                                                         \
    CDL_APPEND_ELEM2(head, el, add, prev, next)

#ifdef NO_DECLTYPE
/* Here are VS2008 / NO_DECLTYPE replacements for a few functions */

#undef CDL_INSERT_INORDER2
#define CDL_INSERT_INORDER2(head,add,cmp,prev,next)                                            \
do {                                                                                           \
  if ((head) == NULL) {                                                                        \
    (add)->prev = (add);                                                                       \
    (add)->next = (add);                                                                       \
    (head) = (add);                                                                            \
  } else if ((cmp(head, add)) >= 0) {                                                          \
    (add)->prev = (head)->prev;                                                                \
    (add)->next = (head);                                                                      \
    (add)->prev->next = (add);                                                                 \
    (head)->prev = (add);                                                                      \
    (head) = (add);                                                                            \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while ((char*)(head)->next != _tmp && (cmp((head)->next, add)) < 0) {                      \
      (head) = (head)->next;                                                                   \
    }                                                                                          \
    (add)->prev = (head);                                                                      \
    (add)->next = (head)->next;                                                                \
    (add)->next->prev = (add);                                                                 \
    (head)->next = (add);                                                                      \
    UTLIST_RS(head);                                                                           \
  }                                                                                            \
} while (0)
#endif /* NO_DECLTYPE */

#endif /* UTLIST_H */
