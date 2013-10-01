/*
 * Test for pthread_detach().
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
 * Depends on API functions: pthread_create(), pthread_detach(), pthread_exit().
 */

#include "test.h"


enum {
  NUMTHREADS = 100
};

void *
func(void * arg)
{
    int i = (int)(size_t)arg;

    Sleep(i * 10);

    pthread_exit(arg);

    /* Never reached. */
    exit(1);
}

int
main(int argc, char * argv[])
{
	pthread_t id[NUMTHREADS];
	int i;

	/* Create a few threads and then exit. */
	for (i = 0; i < NUMTHREADS; i++)
	  {
	    assert(pthread_create(&id[i], NULL, func, (void *)(size_t)i) == 0);
	  }

	/* Some threads will finish before they are detached, some after. */
	Sleep(NUMTHREADS/2 * 10 + 50);

	for (i = 0; i < NUMTHREADS; i++)
	  {
	    assert(pthread_detach(id[i]) == 0);
	  }

	Sleep(NUMTHREADS * 10 + 100);

	/*
	 * Check that all threads are now invalid.
	 * This relies on unique thread IDs - e.g. works with
	 * pthreads-w32 or Solaris, but may not work for Linux, BSD etc.
	 */
	for (i = 0; i < NUMTHREADS; i++)
	  {
	    assert(pthread_kill(id[i], 0) == ESRCH);
	  }

	/* Success. */
	return 0;
}
