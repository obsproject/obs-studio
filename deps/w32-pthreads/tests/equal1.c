/*
 * Test for pthread_equal.
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
 * Depends on functions: pthread_create().
 */

#include "test.h"

void * func(void * arg)
{
  Sleep(2000);
  return 0;
}

int 
main()
{
  pthread_t t1, t2;

  assert(pthread_create(&t1, NULL, func, (void *) 1) == 0);

  assert(pthread_create(&t2, NULL, func, (void *) 2) == 0);

  assert(pthread_equal(t1, t2) == 0);

  assert(pthread_equal(t1,t1) != 0);

  /* This is a hack. We don't want to rely on pthread_join
     yet if we can help it. */
   Sleep(4000);

  /* Success. */
  return 0;
}
