/*
 * once1.c
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
 * Create a static pthread_once and test that it calls myfunc once.
 *
 * Depends on API functions:
 *	pthread_once()
 *	pthread_create()
 */

#include "test.h"

pthread_once_t once = PTHREAD_ONCE_INIT;

static int washere = 0;

void
myfunc(void)
{
  washere++;
}

void *
mythread(void * arg)
{
   assert(pthread_once(&once, myfunc) == 0);

   return 0;
}

int
main()
{
  pthread_t t1, t2;
  
  assert(pthread_create(&t1, NULL, mythread, NULL) == 0);

  assert(pthread_create(&t2, NULL, mythread, NULL) == 0);

  Sleep(2000);

  assert(washere == 1);

  return 0;
}
