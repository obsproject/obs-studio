/*
 * self1.c
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
 * Test for pthread_self().
 *
 * Depends on API functions:
 *	pthread_self()
 *
 * Implicitly depends on:
 *	pthread_getspecific()
 *	pthread_setspecific()
 */

#include "test.h"

int
main(int argc, char * argv[])
{
	/*
	 * This should always succeed unless the system has no
	 * resources (memory) left.
	 */
	pthread_t self;

#if defined(PTW32_STATIC_LIB) && !(defined(_MSC_VER) || defined(__MINGW32__))
	pthread_win32_process_attach_np();
#endif

	self = pthread_self();

	assert(self.p != NULL);

#if defined(PTW32_STATIC_LIB) && !(defined(_MSC_VER) || defined(__MINGW32__))
	pthread_win32_process_detach_np();
#endif
	return 0;
}
