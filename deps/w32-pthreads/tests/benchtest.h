/*
 *
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 * 
 *      Contact Email: rpj@callisto.canberra.edu.au
 * 
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "../config.h"

enum {
  OLD_WIN32CS,
  OLD_WIN32MUTEX
};

extern int old_mutex_use;

struct old_mutex_t_ {
  HANDLE mutex;
  CRITICAL_SECTION cs;
};

typedef struct old_mutex_t_ * old_mutex_t;

struct old_mutexattr_t_ {
  int pshared;
};

typedef struct old_mutexattr_t_ * old_mutexattr_t;

extern BOOL (WINAPI *ptw32_try_enter_critical_section)(LPCRITICAL_SECTION);
extern HINSTANCE ptw32_h_kernel32;

#define PTW32_OBJECT_AUTO_INIT ((void *) -1)

void dummy_call(int * a);
void interlocked_inc_with_conditionals(int *a);
void interlocked_dec_with_conditionals(int *a);
int old_mutex_init(old_mutex_t *mutex, const old_mutexattr_t *attr);
int old_mutex_lock(old_mutex_t *mutex);
int old_mutex_unlock(old_mutex_t *mutex);
int old_mutex_trylock(old_mutex_t *mutex);
int old_mutex_destroy(old_mutex_t *mutex);
/****************************************************************************************/
