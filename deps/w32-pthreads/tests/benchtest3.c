/*
 * benchtest3.c
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
 * Measure time taken to complete an elementary operation.
 *
 * - Mutex
 *   Single thread iteration over a trylock on a locked mutex for each mutex type.
 */

#include "test.h"
#include <sys/timeb.h>

#ifdef __GNUC__
#include <stdlib.h>
#endif

#include "benchtest.h"

#define PTW32_MUTEX_TYPES
#define ITERATIONS      10000000L

pthread_mutex_t mx;
old_mutex_t ox;
pthread_mutexattr_t ma;
PTW32_STRUCT_TIMEB currSysTimeStart;
PTW32_STRUCT_TIMEB currSysTimeStop;
long durationMilliSecs;
long overHeadMilliSecs = 0;

#define GetDurationMilliSecs(_TStart, _TStop) ((long)((_TStop.time*1000+_TStop.millitm) \
                                               - (_TStart.time*1000+_TStart.millitm)))

/*
 * Dummy use of j, otherwise the loop may be removed by the optimiser
 * when doing the overhead timing with an empty loop.
 */
#define TESTSTART \
  { int i, j = 0, k = 0; PTW32_FTIME(&currSysTimeStart); for (i = 0; i < ITERATIONS; i++) { j++;

#define TESTSTOP \
  }; PTW32_FTIME(&currSysTimeStop); if (j + k == i) j++; }


void *
trylockThread (void * arg)
{
  TESTSTART
  (void) pthread_mutex_trylock(&mx);
  TESTSTOP

  return NULL;
}


void *
oldTrylockThread (void * arg)
{
  TESTSTART
  (void) old_mutex_trylock(&ox);
  TESTSTOP

  return NULL;
}


void
runTest (char * testNameString, int mType)
{
  pthread_t t;

#ifdef PTW32_MUTEX_TYPES
  (void) pthread_mutexattr_settype(&ma, mType);
#endif
  assert(pthread_mutex_init(&mx, &ma) == 0);
  assert(pthread_mutex_lock(&mx) == 0);
  assert(pthread_create(&t, NULL, trylockThread, 0) == 0);
  assert(pthread_join(t, NULL) == 0);
  assert(pthread_mutex_unlock(&mx) == 0);
  assert(pthread_mutex_destroy(&mx) == 0);

  durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;

  printf( "%-45s %15ld %15.3f\n",
	    testNameString,
          durationMilliSecs,
          (float) durationMilliSecs * 1E3 / ITERATIONS);
}


int
main (int argc, char *argv[])
{
  pthread_t t;

  assert(pthread_mutexattr_init(&ma) == 0);

  printf( "=============================================================================\n");
  printf( "\nTrylock on a locked mutex.\n");
  printf( "%ld iterations.\n\n", ITERATIONS);
  printf( "%-45s %15s %15s\n",
	    "Test",
	    "Total(msec)",
	    "average(usec)");
  printf( "-----------------------------------------------------------------------------\n");

  /*
   * Time the loop overhead so we can subtract it from the actual test times.
   */

  TESTSTART
  TESTSTOP

  durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
  overHeadMilliSecs = durationMilliSecs;


  old_mutex_use = OLD_WIN32CS;
  assert(old_mutex_init(&ox, NULL) == 0);
  assert(old_mutex_lock(&ox) == 0);
  assert(pthread_create(&t, NULL, oldTrylockThread, 0) == 0);
  assert(pthread_join(t, NULL) == 0);
  assert(old_mutex_unlock(&ox) == 0);
  assert(old_mutex_destroy(&ox) == 0);
  durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
  printf( "%-45s %15ld %15.3f\n",
	    "Old PT Mutex using a Critical Section (WNT)",
          durationMilliSecs,
          (float) durationMilliSecs * 1E3 / ITERATIONS);

  old_mutex_use = OLD_WIN32MUTEX;
  assert(old_mutex_init(&ox, NULL) == 0);
  assert(old_mutex_lock(&ox) == 0);
  assert(pthread_create(&t, NULL, oldTrylockThread, 0) == 0);
  assert(pthread_join(t, NULL) == 0);
  assert(old_mutex_unlock(&ox) == 0);
  assert(old_mutex_destroy(&ox) == 0);
  durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
  printf( "%-45s %15ld %15.3f\n",
	    "Old PT Mutex using a Win32 Mutex (W9x)",
          durationMilliSecs,
          (float) durationMilliSecs * 1E3 / ITERATIONS);

  printf( ".............................................................................\n");

  /*
   * Now we can start the actual tests
   */
#ifdef PTW32_MUTEX_TYPES
  runTest("PTHREAD_MUTEX_DEFAULT", PTHREAD_MUTEX_DEFAULT);

  runTest("PTHREAD_MUTEX_NORMAL", PTHREAD_MUTEX_NORMAL);

  runTest("PTHREAD_MUTEX_ERRORCHECK", PTHREAD_MUTEX_ERRORCHECK);

  runTest("PTHREAD_MUTEX_RECURSIVE", PTHREAD_MUTEX_RECURSIVE);
#else
  runTest("Non-blocking lock", 0);
#endif

  printf( ".............................................................................\n");

  pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);

#ifdef PTW32_MUTEX_TYPES
  runTest("PTHREAD_MUTEX_DEFAULT (Robust)", PTHREAD_MUTEX_DEFAULT);

  runTest("PTHREAD_MUTEX_NORMAL (Robust)", PTHREAD_MUTEX_NORMAL);

  runTest("PTHREAD_MUTEX_ERRORCHECK (Robust)", PTHREAD_MUTEX_ERRORCHECK);

  runTest("PTHREAD_MUTEX_RECURSIVE (Robust)", PTHREAD_MUTEX_RECURSIVE);
#else
  runTest("Non-blocking lock", 0);
#endif

  printf( "=============================================================================\n");

  /*
   * End of tests.
   */

  pthread_mutexattr_destroy(&ma);

  return 0;
}
