/* 
 * mutex7e.c
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
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * --------------------------------------------------------------------------
 *
 * Tests PTHREAD_MUTEX_ERRORCHECK mutex type.
 * Thread locks and then trylocks mutex (attempted recursive lock).
 * Trylock should fail with an EBUSY error.
 * The second unlock attempt should fail with an EPERM error.
 *
 * Depends on API functions: 
 *      pthread_create()
 *      pthread_join()
 *      pthread_mutexattr_init()
 *      pthread_mutexattr_destroy()
 *      pthread_mutexattr_settype()
 *      pthread_mutexattr_gettype()
 *      pthread_mutex_init()
 *      pthread_mutex_destroy()
 *	pthread_mutex_lock()
 *	pthread_mutex_unlock()
 */

#include "test.h"

static int lockCount;

static pthread_mutex_t mutex;
static pthread_mutexattr_t mxAttr;

void * locker(void * arg)
{
  assert(pthread_mutex_lock(&mutex) == 0);
  lockCount++;
  assert(pthread_mutex_trylock(&mutex) == EBUSY);
  lockCount++;
  assert(pthread_mutex_unlock(&mutex) == 0);

  return (void *) 555;
}
 
int
main()
{
  pthread_t t;
  void* result = (void*)0;
  int mxType = -1;

  assert(pthread_mutexattr_init(&mxAttr) == 0);

  BEGIN_MUTEX_STALLED_ROBUST(mxAttr)

  lockCount = 0;
  assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
  assert(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
  assert(mxType == PTHREAD_MUTEX_ERRORCHECK);

  assert(pthread_mutex_init(&mutex, &mxAttr) == 0);

  assert(pthread_create(&t, NULL, locker, NULL) == 0);

  assert(pthread_join(t, &result) == 0);
  assert((int)(size_t)result == 555);

  assert(lockCount == 2);

  assert(pthread_mutex_destroy(&mutex) == 0);

  END_MUTEX_STALLED_ROBUST(mxAttr)

  assert(pthread_mutexattr_destroy(&mxAttr) == 0);

  exit(0);

  /* Never reached */
  return 0;
}

