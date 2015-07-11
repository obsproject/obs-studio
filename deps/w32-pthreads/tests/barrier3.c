/*
 * barrier3.c
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
 * Declare a single barrier object with barrier attribute, wait on it, 
 * and then destroy it.
 *
 */

#include "test.h"
 
pthread_barrier_t barrier = NULL;
static void* result = (void*)1;

void * func(void * arg)
{
  return (void *) (size_t)pthread_barrier_wait(&barrier);
}
 
int
main()
{
  pthread_t t;
  pthread_barrierattr_t ba;

  assert(pthread_barrierattr_init(&ba) == 0);
  assert(pthread_barrierattr_setpshared(&ba, PTHREAD_PROCESS_PRIVATE) == 0);
  assert(pthread_barrier_init(&barrier, &ba, 1) == 0);

  assert(pthread_create(&t, NULL, func, NULL) == 0);

  assert(pthread_join(t, &result) == 0);

  assert((int)(size_t)result == PTHREAD_BARRIER_SERIAL_THREAD);

  assert(pthread_barrier_destroy(&barrier) == 0);
  assert(pthread_barrierattr_destroy(&ba) == 0);

  return 0;
}
