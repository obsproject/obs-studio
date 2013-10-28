/*
 * once4.c
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
 * Create several pthread_once objects and channel several threads
 * through each. Make the init_routine cancelable and cancel them
 * waiters waiting. Vary the priorities.
 *
 * Depends on API functions:
 *	pthread_once()
 *	pthread_create()
 *      pthread_testcancel()
 *      pthread_cancel()
 *      pthread_once()
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

typedef struct {
  int threadnum;
  int oncenum;
  int myPrio;
  HANDLE w32Thread;
} bag_t;

static bag_t threadbag[NUM_THREADS][NUM_ONCE];

CRITICAL_SECTION print_lock;

void
mycleanupfunc(void * arg)
{
  bag_t * bag = (bag_t *) arg;
  EnterCriticalSection(&print_lock);
  /*      once thrd  prio error */
  printf("%4d %4d %4d %4d\n",
	 bag->oncenum,
	 bag->threadnum,
	 bag->myPrio,
	 bag->myPrio - GetThreadPriority(bag->w32Thread));
  LeaveCriticalSection(&print_lock);
}

void
myinitfunc(void)
{
  EnterCriticalSection(&numOnce.cs);
  numOnce.i++;
  LeaveCriticalSection(&numOnce.cs);
  /* Simulate slow once routine so that following threads pile up behind it */
  Sleep(10);
  /* test for cancelation late so we're sure to have waiters. */
  pthread_testcancel();
}

void *
mythread(void * arg)
{
  bag_t * bag = (bag_t *) arg;
  struct sched_param param;

  /*
   * Cancel every thread. These threads are deferred cancelable only, so
   * only the thread performing the init_routine will see it (there are
   * no other cancelation points here). The result will be that every thread
   * eventually cancels only when it becomes the new initter.
   */
  pthread_t self = pthread_self();
  bag->w32Thread = pthread_getw32threadhandle_np(self);
  /*
   * Set priority between -2 and 2 inclusive.
   */
  bag->myPrio = (bag->threadnum % 5) - 2;
  param.sched_priority = bag->myPrio;
  pthread_setschedparam(self, SCHED_OTHER, &param);

  /* Trigger a cancellation at the next cancellation point in this thread */
  pthread_cancel(self);
#if 0
  pthread_cleanup_push(mycleanupfunc, arg);
  assert(pthread_once(&once[bag->oncenum], myinitfunc) == 0);
  pthread_cleanup_pop(1);
#else
  assert(pthread_once(&once[bag->oncenum], myinitfunc) == 0);
#endif
  EnterCriticalSection(&numThreads.cs);
  numThreads.i++;
  LeaveCriticalSection(&numThreads.cs);
  return 0;
}

int
main()
{
  pthread_t t[NUM_THREADS][NUM_ONCE];
  int i, j;
  
  InitializeCriticalSection(&print_lock);
  InitializeCriticalSection(&numThreads.cs);
  InitializeCriticalSection(&numOnce.cs);

#if 0
  /*       once thrd  prio change */
  printf("once thrd  prio  error\n");
#endif

  /*
   * Set the priority class to realtime - otherwise normal
   * Windows random priority boosting will obscure any problems.
   */
  SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
  /* Set main thread to lower prio than threads */
  SetThreadPriority(GetCurrentThread(), -2);

  for (j = 0; j < NUM_ONCE; j++)
    {
      once[j] = o;

      for (i = 0; i < NUM_THREADS; i++)
        {
	  bag_t * bag = &threadbag[i][j];
	  bag->threadnum = i;
	  bag->oncenum = j;
          /* GCC build: create was failing with EAGAIN after 790 threads */
          while (0 != pthread_create(&t[i][j], NULL, mythread, (void *)bag))
            sched_yield();
        }
    }

  for (j = 0; j < NUM_ONCE; j++)
    for (i = 0; i < NUM_THREADS; i++)
      if (pthread_join(t[i][j], NULL) != 0)
        printf("Join failed for [thread,once] = [%d,%d]\n", i, j);

  /*
   * All threads will cancel, none will return normally from
   * pthread_once and so numThreads should never be incremented. However,
   * numOnce should be incremented by every thread (NUM_THREADS*NUM_ONCE).
   */
  assert(numOnce.i == NUM_ONCE * NUM_THREADS);
  assert(numThreads.i == 0);

  DeleteCriticalSection(&numOnce.cs);
  DeleteCriticalSection(&numThreads.cs);
  DeleteCriticalSection(&print_lock);

  return 0;
}
