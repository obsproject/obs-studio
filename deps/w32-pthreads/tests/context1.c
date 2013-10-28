/*
 * File: context1.c
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
 * Test Synopsis: Test context switching method.
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
 * - pthread_create
 *   pthread_exit
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

#define _WIN32_WINNT 0x400

#include "test.h"
#include "../implement.h"
#include "../context.h"

static int washere = 0;

static void * func(void * arg)
{
  washere = 1;

  Sleep(1000);

  return 0; 
}

static void
anotherEnding ()
{
  /*
   * Switched context
   */
  washere++;

  pthread_exit(0);
}

int
main()
{
  pthread_t t;
  HANDLE hThread;

  assert(pthread_create(&t, NULL, func, NULL) == 0);

  hThread = ((ptw32_thread_t *)t.p)->threadH;

  Sleep(500);

  SuspendThread(hThread);

  if (WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT) 
    {
      /*
       * Ok, thread did not exit before we got to it.
       */
      CONTEXT context;

      context.ContextFlags = CONTEXT_CONTROL;

      GetThreadContext(hThread, &context);
      PTW32_PROGCTR (context) = (DWORD_PTR) anotherEnding;
      SetThreadContext(hThread, &context);
      ResumeThread(hThread);
    }
  else
    {
      printf("Exited early\n");
      fflush(stdout);
    }

  Sleep(1000);

  assert(washere == 2);

  return 0;
}

