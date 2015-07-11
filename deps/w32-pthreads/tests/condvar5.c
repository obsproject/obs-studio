/*
 * File: condvar5.c
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
 * - Test pthread_cond_broadcast.
 *
 * Test Method (Validation or Falsification):
 * - Validation
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
 * - Test broadcast with one waiting CV.
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
 * - pthread_cond_timedwait returns 0.
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - pthread_cond_timedwait returns ETIMEDOUT.
 * - Process returns non-zero exit status.
 */

#include "test.h"
#include <sys/timeb.h>

typedef struct cvthing_t_ cvthing_t;

struct cvthing_t_ {
  pthread_cond_t notbusy;
  pthread_mutex_t lock;
  int shared;
};

static cvthing_t cvthing = {
  PTHREAD_COND_INITIALIZER,
  PTHREAD_MUTEX_INITIALIZER,
  0
};

enum {
  NUMTHREADS = 2
};

void *
mythread(void * arg)
{
  assert(pthread_mutex_lock(&cvthing.lock) == 0);
  cvthing.shared++;
  assert(pthread_mutex_unlock(&cvthing.lock) == 0);

  assert(pthread_cond_broadcast(&cvthing.notbusy) == 0);

  return (void *) 0;
}

int
main()
{
  pthread_t t[NUMTHREADS];
  struct timespec abstime = { 0, 0 };
  PTW32_STRUCT_TIMEB currSysTime;
  const DWORD NANOSEC_PER_MILLISEC = 1000000;

  cvthing.shared = 0;

  assert((t[0] = pthread_self()).p != NULL);

  assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);

  assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);

  assert(pthread_mutex_lock(&cvthing.lock) == 0);

  assert(cvthing.lock != PTHREAD_MUTEX_INITIALIZER);

  /* get current system time */
  PTW32_FTIME(&currSysTime);

  abstime.tv_sec = (long)currSysTime.time;
  abstime.tv_nsec = NANOSEC_PER_MILLISEC * currSysTime.millitm;

  abstime.tv_sec += 5;

  assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == ETIMEDOUT);
  
  assert(cvthing.notbusy != PTHREAD_COND_INITIALIZER);

  assert(pthread_create(&t[1], NULL, mythread, (void *) 1) == 0);

  PTW32_FTIME(&currSysTime);

  abstime.tv_sec = (long)currSysTime.time;
  abstime.tv_nsec = NANOSEC_PER_MILLISEC * currSysTime.millitm;

  abstime.tv_sec += 5;

  while (! (cvthing.shared > 0))
    assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);

  assert(cvthing.shared > 0);

  assert(pthread_mutex_unlock(&cvthing.lock) == 0);

  assert(pthread_join(t[1], NULL) == 0);

  assert(pthread_mutex_destroy(&cvthing.lock) == 0);

  assert(cvthing.lock == NULL);

  assert(pthread_cond_destroy(&cvthing.notbusy) == 0);

  assert(cvthing.notbusy == NULL);

  return 0;
}
