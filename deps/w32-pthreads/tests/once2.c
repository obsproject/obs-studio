/*
 * once2.c
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
 * Create several static pthread_once objects and channel several threads
 * through each.
 *
 * Depends on API functions:
 *	pthread_once()
 *	pthread_create()
 */

#include "test.h"

#define NUM_THREADS 100 /* Targeting each once control */
#define NUM_ONCE    10

pthread_once_t o = PTHREAD_ONCE_INIT;
pthread_once_t once[NUM_ONCE];

typedef struct {
  int i;
  CRITICAL_SECTION cs;
} sharedInt_t;

static sharedInt_t numOnce = {0, {0}};
static sharedInt_t numThreads = {0, {0}};

void
myfunc(void)
{
  EnterCriticalSection(&numOnce.cs);
  numOnce.i++;
  LeaveCriticalSection(&numOnce.cs);
  /* Simulate slow once routine so that following threads pile up behind it */
  Sleep(100);
}

void *
mythread(void * arg)
{
   assert(pthread_once(&once[(int)(size_t)arg], myfunc) == 0);
   EnterCriticalSection(&numThreads.cs);
   numThreads.i++;   
   LeaveCriticalSection(&numThreads.cs);
   return (void*)(size_t)0;
}

int
main()
{
  pthread_t t[NUM_THREADS][NUM_ONCE];
  int i, j;
  
  InitializeCriticalSection(&numThreads.cs);
  InitializeCriticalSection(&numOnce.cs);

  for (j = 0; j < NUM_ONCE; j++)
    {
      once[j] = o;

      for (i = 0; i < NUM_THREADS; i++)
        {
	  /* GCC build: create was failing with EAGAIN after 790 threads */
          while (0 != pthread_create(&t[i][j], NULL, mythread, (void *)(size_t)j))
	    sched_yield();
        }
    }

  for (j = 0; j < NUM_ONCE; j++)
    for (i = 0; i < NUM_THREADS; i++)
      if (pthread_join(t[i][j], NULL) != 0)
        printf("Join failed for [thread,once] = [%d,%d]\n", i, j);

  assert(numOnce.i == NUM_ONCE);
  assert(numThreads.i == NUM_THREADS * NUM_ONCE);

  DeleteCriticalSection(&numOnce.cs);
  DeleteCriticalSection(&numThreads.cs);

  return 0;
}
