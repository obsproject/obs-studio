/*
 * File: inherit1.c
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
 * Test Synopsis:
 * - Test thread priority inheritance.
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


void * func(void * arg)
{
  int policy;
  struct sched_param param;

  assert(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
  return (void *) (size_t)param.sched_priority;
}


void *
getValidPriorities(void * arg)
{
  int prioSet;
  pthread_t thread = pthread_self();
  HANDLE threadH = pthread_getw32threadhandle_np(thread);
  struct sched_param param;

  for (prioSet = minPrio;
       prioSet <= maxPrio;
       prioSet++)
    {
        /*
       * If prioSet is invalid then the threads priority is unchanged
       * from the previous value. Make the previous value a known
       * one so that we can check later.
       */
        param.sched_priority = prioSet;
        assert(pthread_setschedparam(thread, SCHED_OTHER, &param) == 0);
        validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
    }

  return (void *) 0;
}


int
main()
{
  pthread_t t;
  pthread_t mainThread = pthread_self();
  pthread_attr_t attr;
  void * result = NULL;
  struct sched_param param;
  struct sched_param mainParam;
  int prio;
  int policy;
  int inheritsched = -1;
  pthread_t threadID = pthread_self();
  HANDLE threadH = pthread_getw32threadhandle_np(threadID);

  assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
  assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);

  assert(pthread_create(&t, NULL, getValidPriorities, NULL) == 0);
  assert(pthread_join(t, &result) == 0);

  assert(pthread_attr_init(&attr) == 0);
  assert(pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED) == 0);
  assert(pthread_attr_getinheritsched(&attr, &inheritsched) == 0);
  assert(inheritsched == PTHREAD_INHERIT_SCHED);

  for (prio = minPrio; prio <= maxPrio; prio++)
    {
      mainParam.sched_priority = prio;

      /* Set the thread's priority to a known initial value. */
      SetThreadPriority(threadH, PTW32TEST_THREAD_INIT_PRIO);

      /* Change the main thread priority */
      assert(pthread_setschedparam(mainThread, SCHED_OTHER, &mainParam) == 0);
      assert(pthread_getschedparam(mainThread, &policy, &mainParam) == 0);
      assert(policy == SCHED_OTHER);
      /* Priority returned below should be the level set by pthread_setschedparam(). */
      assert(mainParam.sched_priority == prio);
      assert(GetThreadPriority(threadH) ==
               validPriorities[prio+(PTW32TEST_MAXPRIORITIES/2)]);

      for (param.sched_priority = prio;
           param.sched_priority <= maxPrio;
           param.sched_priority++)
        {
          /* The new thread create should ignore this new priority */
          assert(pthread_attr_setschedparam(&attr, &param) == 0);
          assert(pthread_create(&t, &attr, func, NULL) == 0);
          pthread_join(t, &result);
          assert((int)(size_t) result == mainParam.sched_priority);
        }
    }

  return 0;
}
