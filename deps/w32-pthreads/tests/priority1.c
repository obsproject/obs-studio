/*
 * File: priority1.c
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
 * Test Synopsis:
 * - Test thread priority explicit setting using thread attribute.
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
 * -
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

#include "test.h"

enum {
  PTW32TEST_THREAD_INIT_PRIO = 0,
  PTW32TEST_MAXPRIORITIES = 512
};

int minPrio;
int maxPrio;
int validPriorities[PTW32TEST_MAXPRIORITIES];

void *
func(void * arg)
{
  int policy;
  struct sched_param param;
  pthread_t threadID = pthread_self();

  assert(pthread_getschedparam(threadID, &policy, &param) == 0);
  assert(policy == SCHED_OTHER);
  return (void *) (size_t)(param.sched_priority);
}

void *
getValidPriorities(void * arg)
{
  int prioSet;
  pthread_t threadID = pthread_self();
  HANDLE threadH = pthread_getw32threadhandle_np(threadID);

  printf("Using GetThreadPriority\n");
  printf("%10s %10s\n", "Set value", "Get value");

  for (prioSet = minPrio;
       prioSet <= maxPrio;
       prioSet++)
    {
	/*
       * If prioSet is invalid then the threads priority is unchanged
       * from the previous value. Make the previous value a known
       * one so that we can check later.
       */
        if (prioSet < 0)
	  SetThreadPriority(threadH, THREAD_PRIORITY_LOWEST);
        else
	  SetThreadPriority(threadH, THREAD_PRIORITY_HIGHEST);
	SetThreadPriority(threadH, prioSet);
	validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
	printf("%10d %10d\n", prioSet, validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)]);
    }

  return (void *) 0;
}


int
main()
{
  pthread_t t;
  pthread_attr_t attr;
  void * result = NULL;
  struct sched_param param;

  assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
  assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);

  assert(pthread_create(&t, NULL, getValidPriorities, NULL) == 0);
  assert(pthread_join(t, &result) == 0);

  assert(pthread_attr_init(&attr) == 0);
  assert(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);

  /* Set the thread's priority to a known initial value. */
  SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()),
                    PTW32TEST_THREAD_INIT_PRIO);

  printf("Using pthread_getschedparam\n");
  printf("%10s %10s %10s\n", "Set value", "Get value", "Win priority");

  for (param.sched_priority = minPrio;
       param.sched_priority <= maxPrio;
       param.sched_priority++)
    {
      int prio;

      assert(pthread_attr_setschedparam(&attr, &param) == 0);
      assert(pthread_create(&t, &attr, func, (void *) &attr) == 0);

      assert((prio = GetThreadPriority(pthread_getw32threadhandle_np(t)))
             == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);

      assert(pthread_join(t, &result) == 0);

      assert(param.sched_priority == (int)(size_t) result);
      printf("%10d %10d %10d\n", param.sched_priority, (int)(size_t) result, prio);
    }

  return 0;
}
