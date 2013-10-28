/*
 * File: semaphore2.c
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
 * Test Synopsis: Verify sem_getvalue returns the correct value.
 * - 
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

#define MAX_COUNT 100

int
main()
{
  sem_t s;
	int value = 0;
	int i;

  assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, MAX_COUNT) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	assert(value == MAX_COUNT);
//	  printf("Value = %ld\n", value);

	for (i = MAX_COUNT - 1; i >= 0; i--)
		{
			assert(sem_wait(&s) == 0);
			assert(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
			assert(value == i);
		}

	for (i = 1; i <= MAX_COUNT; i++)
		{
			assert(sem_post(&s) == 0);
			assert(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
			assert(value == i);
		}

  return 0;
}

