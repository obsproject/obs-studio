/* 
 * robust3.c
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
 * --------------------------------------------------------------------------
 *
 * For all robust mutex types.
 * Thread A locks mutex
 * Thread B blocks on mutex
 * Thread A terminates with threads waiting on robust mutex
 * Thread B awakes and inherits mutex, sets consistent and unlocks
 * Main acquires mutex with recovered state.
 *
 * Depends on API functions: 
 *      pthread_create()
 *      pthread_join()
 *	pthread_mutex_init()
 *	pthread_mutex_lock()
 *	pthread_mutex_unlock()
 *	pthread_mutex_consistent()
 *	pthread_mutex_destroy()
 *	pthread_mutexattr_init()
 *	pthread_mutexattr_setrobust()
 *	pthread_mutexattr_settype()
 *	pthread_mutexattr_destroy()
 */

#include "test.h"

static int lockCount;

static pthread_mutex_t mutex;

void * owner(void * arg)
{
  assert(pthread_mutex_lock(&mutex) == 0);
  lockCount++;
  Sleep(200);

  return 0;
}
 
void * inheritor(void * arg)
{
  assert(pthread_mutex_lock(&mutex) == EOWNERDEAD);
  lockCount++;
  assert(pthread_mutex_consistent(&mutex) == 0);
  assert(pthread_mutex_unlock(&mutex) == 0);

  return 0;
}
 
int
main()
{
  pthread_t to, ti;
  pthread_mutexattr_t ma;

  assert(pthread_mutexattr_init(&ma) == 0);
  assert(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);

  /* Default (NORMAL) type */ 
  lockCount = 0;
  assert(pthread_mutex_init(&mutex, &ma) == 0);
  assert(pthread_create(&to, NULL, owner, NULL) == 0);
  Sleep(100);
  assert(pthread_create(&ti, NULL, inheritor, NULL) == 0);
  assert(pthread_join(to, NULL) == 0);
  assert(pthread_join(ti, NULL) == 0);
  assert(lockCount == 2);
  assert(pthread_mutex_lock(&mutex) == 0);
  assert(pthread_mutex_unlock(&mutex) == 0);
  assert(pthread_mutex_destroy(&mutex) == 0);

  /* NORMAL type */ 
  lockCount = 0;
  assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
  assert(pthread_mutex_init(&mutex, &ma) == 0);
  assert(pthread_create(&to, NULL, owner, NULL) == 0);
  Sleep(100);
  assert(pthread_create(&ti, NULL, inheritor, NULL) == 0);
  assert(pthread_join(to, NULL) == 0);
  assert(pthread_join(ti, NULL) == 0);
  assert(lockCount == 2);
  assert(pthread_mutex_lock(&mutex) == 0);
  assert(pthread_mutex_unlock(&mutex) == 0);
  assert(pthread_mutex_destroy(&mutex) == 0);

  /* ERRORCHECK type */ 
  lockCount = 0;
  assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
  assert(pthread_mutex_init(&mutex, &ma) == 0);
  assert(pthread_create(&to, NULL, owner, NULL) == 0);
  Sleep(100);
  assert(pthread_create(&ti, NULL, inheritor, NULL) == 0);
  assert(pthread_join(to, NULL) == 0);
  assert(pthread_join(ti, NULL) == 0);
  assert(lockCount == 2);
  assert(pthread_mutex_lock(&mutex) == 0);
  assert(pthread_mutex_unlock(&mutex) == 0);
  assert(pthread_mutex_destroy(&mutex) == 0);

  /* RECURSIVE type */ 
  lockCount = 0;
  assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
  assert(pthread_mutex_init(&mutex, &ma) == 0);
  assert(pthread_create(&to, NULL, owner, NULL) == 0);
  Sleep(100);
  assert(pthread_create(&ti, NULL, inheritor, NULL) == 0);
  assert(pthread_join(to, NULL) == 0);
  assert(pthread_join(ti, NULL) == 0);
  assert(lockCount == 2);
  assert(pthread_mutex_lock(&mutex) == 0);
  assert(pthread_mutex_unlock(&mutex) == 0);
  assert(pthread_mutex_destroy(&mutex) == 0);

  assert(pthread_mutexattr_destroy(&ma) == 0);

  return 0;
}
