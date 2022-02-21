/* 
 * spin3.c
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
 * Thread A locks spin - thread B tries to unlock.
 * This should succeed, but it's undefined behaviour.
 *
 */

#include "test.h"

static int wasHere = 0;

static pthread_spinlock_t spin;
 
void * unlocker(void * arg)
{
  int expectedResult = (int)(size_t)arg;

  wasHere++;
  assert(pthread_spin_unlock(&spin) == expectedResult);
  wasHere++;
  return NULL;
}
 
int
main()
{
  pthread_t t;

  wasHere = 0;
  assert(pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE) == 0);
  assert(pthread_spin_lock(&spin) == 0);
  assert(pthread_create(&t, NULL, unlocker, (void*)0) == 0);
  assert(pthread_join(t, NULL) == 0);
  /*
   * Our spinlocks don't record the owner thread so any thread can unlock the spinlock,
   * but nor is it an error for any thread to unlock a spinlock that is not locked.
   */
  assert(pthread_spin_unlock(&spin) == 0);
  assert(pthread_spin_destroy(&spin) == 0);
  assert(wasHere == 2);

  return 0;
}
