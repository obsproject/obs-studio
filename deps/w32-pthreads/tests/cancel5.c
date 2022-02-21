/*
 * File: cancel5.c
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
 * Test Synopsis: Test calling pthread_cancel from the main thread
 *                without calling pthread_self() in main.
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

#include "test.h"

/*
 * Create NUMTHREADS threads in addition to the Main thread.
 */
enum
{
  NUMTHREADS = 4
};

typedef struct bag_t_ bag_t;
struct bag_t_
{
  int threadnum;
  int started;
  /* Add more per-thread state variables here */
  int count;
};

static bag_t threadbag[NUMTHREADS + 1];

void *
mythread (void *arg)
{
  void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
  bag_t *bag = (bag_t *) arg;

  assert (bag == &threadbag[bag->threadnum]);
  assert (bag->started == 0);
  bag->started = 1;

  /* Set to known state and type */

  assert (pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL) == 0);

  assert (pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);

  /*
   * We wait up to 10 seconds, waking every 0.1 seconds,
   * for a cancelation to be applied to us.
   */
  for (bag->count = 0; bag->count < 100; bag->count++)
    Sleep (100);

  return result;
}

int
main ()
{
  int failed = 0;
  int i;
  pthread_t t[NUMTHREADS + 1];

  for (i = 1; i <= NUMTHREADS; i++)
    {
      threadbag[i].started = 0;
      threadbag[i].threadnum = i;
      assert (pthread_create (&t[i], NULL, mythread, (void *) &threadbag[i])
	      == 0);
    }

  /*
   * Code to control or munipulate child threads should probably go here.
   */
  Sleep (500);

  for (i = 1; i <= NUMTHREADS; i++)
    {
      assert (pthread_cancel (t[i]) == 0);
    }

  /*
   * Give threads time to run.
   */
  Sleep (NUMTHREADS * 100);

  /*
   * Standard check that all threads started.
   */
  for (i = 1; i <= NUMTHREADS; i++)
    {
      if (!threadbag[i].started)
	{
	  failed |= !threadbag[i].started;
	  fprintf (stderr, "Thread %d: started %d\n", i,
		   threadbag[i].started);
	}
    }

  assert (!failed);

  /*
   * Check any results here. Set "failed" and only print output on failure.
   */
  failed = 0;
  for (i = 1; i <= NUMTHREADS; i++)
    {
      int fail = 0;
      void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);

      /*
       * The thread does not contain any cancelation points, so
       * a return value of PTHREAD_CANCELED confirms that async
       * cancelation succeeded.
       */
      assert (pthread_join (t[i], &result) == 0);

      fail = (result != PTHREAD_CANCELED);

      if (fail)
	{
	  fprintf (stderr, "Thread %d: started %d: count %d\n",
		   i, threadbag[i].started, threadbag[i].count);
	}
      failed = (failed || fail);
    }

  assert (!failed);

  /*
   * Success.
   */
  return 0;
}
