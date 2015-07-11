/* 
 * mutex1r.c
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
 * As for mutex1.c but with type set to PTHREAD_MUTEX_RECURSIVE.
 *
 * Create a simple mutex object, lock it, unlock it, then destroy it.
 * This is the simplest test of the pthread mutex family that we can do.
 *
 * Depends on API functions:
 *	pthread_mutexattr_settype()
 * 	pthread_mutex_init()
 *	pthread_mutex_destroy()
 */

#include "test.h"

pthread_mutex_t mutex = NULL;
pthread_mutexattr_t mxAttr;

int
main()
{
  assert(pthread_mutexattr_init(&mxAttr) == 0);

  BEGIN_MUTEX_STALLED_ROBUST(mxAttr)

  assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_RECURSIVE) == 0);

  assert(mutex == NULL);

  assert(pthread_mutex_init(&mutex, &mxAttr) == 0);

  assert(mutex != NULL);

  assert(pthread_mutex_lock(&mutex) == 0);

  assert(pthread_mutex_unlock(&mutex) == 0);

  assert(pthread_mutex_destroy(&mutex) == 0);

  assert(mutex == NULL);

  END_MUTEX_STALLED_ROBUST(mxAttr)

  return 0;
}
