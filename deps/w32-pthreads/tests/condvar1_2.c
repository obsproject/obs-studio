/*
 * File: condvar1_2.c
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
 * - Test CV linked list management and serialisation.
 *
 * Test Method (Validation or Falsification):
 * - Validation:
 *   Initiate and destroy several CVs in random order.
 *   Asynchronously traverse the CV list and broadcast.
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
 * - Creates and then imediately destroys a CV. Does not
 *   test the CV.
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
 * - All initialised CVs destroyed without segfault.
 * - Successfully broadcasts all remaining CVs after
 *   each CV is removed.
 *
 * Fail Criteria:
 */

#include <stdlib.h>
#include "test.h"

enum {
  NUM_CV = 5,
  NUM_LOOPS = 5
};

static pthread_cond_t cv[NUM_CV];

int
main()
{
  int i, j, k;
  void* result = (void*)-1;
  pthread_t t;

  for (k = 0; k < NUM_LOOPS; k++)
    {
      for (i = 0; i < NUM_CV; i++)
        {
          assert(pthread_cond_init(&cv[i], NULL) == 0);
        }

      j = NUM_CV;
      (void) srand((unsigned)time(NULL));

      /* Traverse the list asynchronously. */
      assert(pthread_create(&t, NULL, pthread_timechange_handler_np, NULL) == 0);

      do
        {
          i = (NUM_CV - 1) * rand() / RAND_MAX;
          if (cv[i] != NULL)
            {
              j--;
              assert(pthread_cond_destroy(&cv[i]) == 0);
            }
        }
      while (j > 0);

      assert(pthread_join(t, &result) == 0);
      assert ((int)(size_t)result == 0);
    }

  return 0;
}
