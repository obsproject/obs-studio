/*
 * File: cancel2.c
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
 * Test Synopsis: Test SEH or C++ cancel exception handling within
 * application exception blocks.
 *
 * Test Method (Validation or Falsification):
 * - 
 *
 * Requirements Tested:
 * -
 *
 * Features Tested:
 * - 
 *
 * Cases Tested:
 * - 
 *
 * Description:
 * - 
 *
 * Environment:
 * - 
 *
 * Input:
 * - None.
 *
 * Output:
 * - File name, Line number, and failed expression on failure.
 * - No output on success.
 *
 * Assumptions:
 * - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
 *   pthread_testcancel, pthread_cancel, pthread_join
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

/*
 * Don't know how to identify if we are using SEH so it's only C++ for now
 */
#if defined(__cplusplus)

#include "test.h"

/*
 * Create NUMTHREADS threads in addition to the Main thread.
 */
enum {
  NUMTHREADS = 4
};

typedef struct bag_t_ bag_t;
struct bag_t_ {
  int threadnum;
  int started;
  /* Add more per-thread state variables here */
};

static bag_t threadbag[NUMTHREADS + 1];

static pthread_mutex_t waitLock = PTHREAD_MUTEX_INITIALIZER;

void *
mythread(void * arg)
{
  int result = 0;
  bag_t * bag = (bag_t *) arg;

  assert(bag == &threadbag[bag->threadnum]);
  assert(bag->started == 0);
  bag->started = 1;

  /* Set to known state and type */

  assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);

  switch (bag->threadnum % 2)
    {
    case 0:
      assert(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
      result = 0;
      break;
    case 1:
      assert(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
      result = 1;
      break;
    }

#if !defined(__cplusplus)
  __try
#else
  try
#endif
    {
      /* Wait for go from main */
      assert(pthread_mutex_lock(&waitLock) == 0);
      assert(pthread_mutex_unlock(&waitLock) == 0);
      sched_yield();

      for (;;)
	{
	  pthread_testcancel();
	}
    }
#if !defined(__cplusplus)
  __except(EXCEPTION_EXECUTE_HANDLER)
#else
#if defined(PtW32CatchAll)
  PtW32CatchAll
#else
  catch(...)
#endif
#endif
    {
      /*
       * Should not get into here.
       */
      result += 100;
    }

  /* 
   * Should not get to here either.
   */
  result += 1000;

  return (void *) (size_t)result;
}

int
main()
{
  int failed = 0;
  int i;
  pthread_t t[NUMTHREADS + 1];

  assert((t[0] = pthread_self()).p != NULL);
  assert(pthread_mutex_lock(&waitLock) == 0);

  for (i = 1; i <= NUMTHREADS; i++)
    {
      threadbag[i].started = 0;
      threadbag[i].threadnum = i;
      assert(pthread_create(&t[i], NULL, mythread, (void *) &threadbag[i]) == 0);
    }

  /*
   * Code to control or munipulate child threads should probably go here.
   */
  Sleep(500);

  assert(pthread_mutex_unlock(&waitLock) == 0);

  Sleep(500);

  for (i = 1; i <= NUMTHREADS; i++)
    {
      assert(pthread_cancel(t[i]) == 0);
    }

  /*
   * Give threads time to run.
   */
  Sleep(NUMTHREADS * 100);

  /*
   * Standard check that all threads started.
   */
  for (i = 1; i <= NUMTHREADS; i++)
    { 
      if (!threadbag[i].started)
	{
	  failed |= !threadbag[i].started;
	  fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
	}
    }

  assert(!failed);

  /*
   * Check any results here. Set "failed" and only print output on failure.
   */
  failed = 0;
  for (i = 1; i <= NUMTHREADS; i++)
    {
      int fail = 0;
      void* result = (void*)0;

      assert(pthread_join(t[i], &result) == 0);
      fail = ((int)(size_t)result != (int) PTHREAD_CANCELED);
      if (fail)
	{
	  fprintf(stderr, "Thread %d: started %d: location %d: cancel type %s\n",
		  i,
		  threadbag[i].started,
		  (int)(size_t)result,
		  (((int)(size_t)result % 2) == 0) ? "ASYNCHRONOUS" : "DEFERRED");
	}
      failed |= fail;
    }

  assert(!failed);

  /*
   * Success.
   */
  return 0;
}

#else /* defined(__cplusplus) */

#include <stdio.h>

int
main()
{
  fprintf(stderr, "Test N/A for this compiler environment.\n");
  return 0;
}

#endif /* defined(__cplusplus) */

