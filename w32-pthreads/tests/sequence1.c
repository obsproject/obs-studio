/*
 * File: sequence1.c
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
 * - that unique thread sequence numbers are generated.
 * - Analyse thread struct reuse.
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
 * - This test is implementation specific
 * because it uses knowledge of internals that should be
 * opaque to an application.
 *
 * Input:
 * - None.
 *
 * Output:
 * - File name, Line number, and failed expression on failure.
 * - analysis output on success.
 *
 * Assumptions:
 * -
 *
 * Pass Criteria:
 * - unique sequence numbers are generated for every new thread.
 *
 * Fail Criteria:
 * - 
 */

#include "test.h"

/*
 */

enum {
	NUMTHREADS = 10000
};


static long done = 0;
/*
 * seqmap should have 1 in every element except [0]
 * Thread sequence numbers start at 1 and we will also
 * include this main thread so we need NUMTHREADS+2
 * elements. 
 */
static UINT64 seqmap[NUMTHREADS+2];

void * func(void * arg)
{
  sched_yield();
  seqmap[(int)pthread_getunique_np(pthread_self())] = 1;
  InterlockedIncrement(&done);

  return (void *) 0; 
}
 
int
main()
{
  pthread_t t[NUMTHREADS];
  pthread_attr_t attr;
  int i;

  assert(pthread_attr_init(&attr) == 0);
  assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);

  for (i = 0; i < NUMTHREADS+2; i++)
    {
      seqmap[i] = 0;
    }

  for (i = 0; i < NUMTHREADS; i++)
    {
      if (NUMTHREADS/2 == i)
        {
          /* Include this main thread, which will be an implicit pthread_t */
          seqmap[(int)pthread_getunique_np(pthread_self())] = 1;
        }
      assert(pthread_create(&t[i], &attr, func, NULL) == 0);
    }

  while (NUMTHREADS > InterlockedExchangeAdd((LPLONG)&done, 0L))
    Sleep(100);

  Sleep(100);

  assert(seqmap[0] == 0);
  for (i = 1; i < NUMTHREADS+2; i++)
    {
      assert(seqmap[i] == 1);
    }

  return 0;
}
