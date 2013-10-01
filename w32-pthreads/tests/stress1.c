/*
 * stress1.c
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
 * - Stress test condition variables, mutexes, semaphores.
 *
 * Test Method (Validation or Falsification):
 * - Validation
 *
 * Requirements Tested:
 * - Correct accounting of semaphore and condition variable waiters.
 *
 * Features Tested:
 * - 
 *
 * Cases Tested:
 * - 
 *
 * Description:
 * Attempting to expose race conditions in cond vars, semaphores etc.
 * - Master attempts to signal slave close to when timeout is due.
 * - Master and slave do battle continuously until main tells them to stop.
 * - Afterwards, the CV must be successfully destroyed (will return an
 * error if there are waiters (including any internal semaphore waiters,
 * which, if there are, cannot be real waiters).
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
 * - CV is successfully destroyed.
 *
 * Fail Criteria:
 * - CV destroy fails.
 */

#include "test.h"
#include <string.h>
#include <sys/timeb.h>


const unsigned int ITERATIONS = 1000;

static pthread_t master, slave;
typedef struct {
  int value;
  pthread_cond_t cv;
  pthread_mutex_t mx;
} mysig_t;

static int allExit;
static mysig_t control = {0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
static pthread_barrier_t startBarrier, readyBarrier, holdBarrier;
static int timeoutCount = 0;
static int signalsTakenCount = 0;
static int signalsSent = 0;
static int bias = 0;
static int timeout = 10; // Must be > 0

enum {
  CTL_STOP     = -1
};

/*
 * Returns abstime 'milliseconds' from 'now'.
 *
 * Works for: -INT_MAX <= millisecs <= INT_MAX
 */
struct timespec *
millisecondsFromNow (struct timespec * time, int millisecs)
{
  PTW32_STRUCT_TIMEB currSysTime;
  int64_t nanosecs, secs;
  const int64_t NANOSEC_PER_MILLISEC = 1000000;
  const int64_t NANOSEC_PER_SEC = 1000000000;

  /* get current system time and add millisecs */
  PTW32_FTIME(&currSysTime);

  secs = (int64_t)(currSysTime.time) + (millisecs / 1000);
  nanosecs = ((int64_t) (millisecs%1000 + currSysTime.millitm)) * NANOSEC_PER_MILLISEC;
  if (nanosecs >= NANOSEC_PER_SEC)
    {
      secs++;
      nanosecs -= NANOSEC_PER_SEC;
    }
  else if (nanosecs < 0)
    {
      secs--;
      nanosecs += NANOSEC_PER_SEC;
    }

  time->tv_nsec = (long)nanosecs;
  time->tv_sec = (long)secs;

  return time;
}

void *
masterThread (void * arg)
{
  int dither = (int)(size_t)arg;

  timeout = (int)(size_t)arg;

  pthread_barrier_wait(&startBarrier);

  do
    {
      int sleepTime;

      assert(pthread_mutex_lock(&control.mx) == 0);
      control.value = timeout;
      assert(pthread_mutex_unlock(&control.mx) == 0);

      /*
       * We are attempting to send the signal close to when the slave
       * is due to timeout. We feel around by adding some [non-random] dither.
       *
       * dither is in the range 2*timeout peak-to-peak
       * sleep time is the average of timeout plus dither.
       * e.g.
       * if timeout = 10 then dither = 20 and
       * sleep millisecs is: 5 <= ms <= 15
       *
       * The bias value attempts to apply some negative feedback to keep
       * the ratio of timeouts to signals taken close to 1:1.
       * bias changes more slowly than dither so as to average more.
       *
       * Finally, if abs(bias) exceeds timeout then timeout is incremented.
       */
      if (signalsSent % timeout == 0)
	{
          if (timeoutCount > signalsTakenCount)
	    {
	      bias++;
	    }
          else if (timeoutCount < signalsTakenCount)
	    {
	      bias--;
	    }
	  if (bias < -timeout || bias > timeout)
	    {
	      timeout++;
	    }
	}
      dither = (dither + 1 ) % (timeout * 2);
      sleepTime = (timeout - bias + dither) / 2;
      Sleep(sleepTime);
      assert(pthread_cond_signal(&control.cv) == 0);
      signalsSent++;

      pthread_barrier_wait(&holdBarrier);
      pthread_barrier_wait(&readyBarrier);
    }
  while (!allExit);

  return NULL;
}

void *
slaveThread (void * arg)
{
  struct timespec time;

  pthread_barrier_wait(&startBarrier);

  do
    {
      assert(pthread_mutex_lock(&control.mx) == 0);
      if (pthread_cond_timedwait(&control.cv,
				 &control.mx,
				 millisecondsFromNow(&time, control.value)) == ETIMEDOUT)
	{
	  timeoutCount++;
	}
      else
	{
	  signalsTakenCount++;
	}
      assert(pthread_mutex_unlock(&control.mx) == 0);

      pthread_barrier_wait(&holdBarrier);
      pthread_barrier_wait(&readyBarrier);
    }
  while (!allExit);

  return NULL;
}

int
main ()
{
  unsigned int i;

  assert(pthread_barrier_init(&startBarrier, NULL, 3) == 0);
  assert(pthread_barrier_init(&readyBarrier, NULL, 3) == 0);
  assert(pthread_barrier_init(&holdBarrier, NULL, 3) == 0);

  assert(pthread_create(&master, NULL, masterThread, (void *)(size_t)timeout) == 0);
  assert(pthread_create(&slave, NULL, slaveThread, NULL) == 0);

  allExit = FALSE;

  pthread_barrier_wait(&startBarrier);

  for (i = 1; !allExit; i++)
    {
      pthread_barrier_wait(&holdBarrier);
      if (i >= ITERATIONS)
	{
	  allExit = TRUE;
	}
      pthread_barrier_wait(&readyBarrier);
    }

  assert(pthread_join(slave, NULL) == 0);
  assert(pthread_join(master, NULL) == 0);

  printf("Signals sent = %d\nWait timeouts = %d\nSignals taken = %d\nBias = %d\nTimeout = %d\n",
	 signalsSent,
	 timeoutCount,
	 signalsTakenCount,
	 (int) bias,
	 timeout);

  /* Cleanup */
  assert(pthread_barrier_destroy(&holdBarrier) == 0);
  assert(pthread_barrier_destroy(&readyBarrier) == 0);
  assert(pthread_barrier_destroy(&startBarrier) == 0);
  assert(pthread_cond_destroy(&control.cv) == 0);
  assert(pthread_mutex_destroy(&control.mx) == 0);

  /* Success. */
  return 0;
}
