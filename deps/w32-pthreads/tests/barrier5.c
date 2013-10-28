/*
 * barrier5.c
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
 * Set up a series of barriers at different heights and test various numbers
 * of threads accessing, especially cases where there are more threads than the
 * barrier height (count), i.e. test contention when the barrier is released.
 */

#include "test.h"

enum {
  NUMTHREADS = 15,
  HEIGHT = 10,
  BARRIERMULTIPLE = 1000
};
 
pthread_barrier_t barrier = NULL;
pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
LONG totalThreadCrossings;

void *
func(void * crossings)
{
  int result;
  int serialThreads = 0;

  while ((LONG)(size_t)crossings >= (LONG)InterlockedIncrement((LPLONG)&totalThreadCrossings))
    {
      result = pthread_barrier_wait(&barrier);

      if (result == PTHREAD_BARRIER_SERIAL_THREAD)
        {
          serialThreads++;
        }
      else if (result != 0)
        {
          printf("Barrier failed: result = %s\n", error_string[result]);
          fflush(stdout);
          return NULL;
        }
    }

  return (void*)(size_t)serialThreads;
}

int
main()
{
  int i, j;
  void* result;
  int serialThreadsTotal;
  LONG Crossings;
  pthread_t t[NUMTHREADS + 1];

  for (j = 1; j <= NUMTHREADS; j++)
    {
      int height = j<HEIGHT?j:HEIGHT;

      totalThreadCrossings = 0;
      Crossings = height * BARRIERMULTIPLE;

      printf("Threads=%d, Barrier height=%d\n", j, height);

      assert(pthread_barrier_init(&barrier, NULL, height) == 0);

      for (i = 1; i <= j; i++)
        {
          assert(pthread_create(&t[i], NULL, func, (void *)(size_t)Crossings) == 0);
        }

      serialThreadsTotal = 0;
      for (i = 1; i <= j; i++)
        {
          assert(pthread_join(t[i], &result) == 0);
          serialThreadsTotal += (int)(size_t)result;
        }

      assert(serialThreadsTotal == BARRIERMULTIPLE);

      assert(pthread_barrier_destroy(&barrier) == 0);
    }

  assert(pthread_mutex_destroy(&mx) == 0);

  return 0;
}
