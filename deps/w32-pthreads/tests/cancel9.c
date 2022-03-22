/*
 * File: cancel9.c
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
 * Test Synopsis: Test true asynchronous cancelation with Alert driver.
 *
 * Test Method (Validation or Falsification):
 * - 
 *
 * Requirements Tested:
 * - Cancel threads, including those blocked on system recources
 *   such as network I/O.
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
 * - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
 *   pthread_testcancel, pthread_cancel, pthread_join
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */

#include "test.h"
#include <windows.h>


void *
test_udp (void *arg)
{
  struct sockaddr_in serverAddress;
  struct sockaddr_in clientAddress;
  SOCKET UDPSocket;
  int addr_len;
  int nbyte, bytes;
  char buffer[4096];
  WORD wsaVersion = MAKEWORD (2, 2);
  WSADATA wsaData;

  pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  if (WSAStartup (wsaVersion, &wsaData) != 0)
    {
      return NULL;
    }

  UDPSocket = socket (AF_INET, SOCK_DGRAM, 0);
  if ((int)UDPSocket == -1)
    {
      printf ("Server: socket ERROR \n");
      exit (-1);
    }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons (9003);

  if (bind
      (UDPSocket, (struct sockaddr *) &serverAddress,
       sizeof (struct sockaddr_in)))
    {
      printf ("Server: ERROR can't bind UDPSocket");
      exit (-1);
    }

  addr_len = sizeof (struct sockaddr);

  nbyte = 512;

  bytes =
    recvfrom (UDPSocket, (char *) buffer, nbyte, 0,
	      (struct sockaddr *) &clientAddress, &addr_len);

  closesocket (UDPSocket);
  WSACleanup ();

  return NULL;
}


void *
test_sleep (void *arg)
{
  pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  Sleep (1000);
  return NULL;

}

void *
test_wait (void *arg)
{
  HANDLE hEvent;
  DWORD dwEvent;

  pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

  dwEvent = WaitForSingleObject (hEvent, 1000);	/* WAIT_IO_COMPLETION */

  return NULL;
}


int
main ()
{
  pthread_t t;
  void *result;

  if (pthread_win32_test_features_np (PTW32_ALERTABLE_ASYNC_CANCEL))
    {
      printf ("Cancel sleeping thread.\n");
      assert (pthread_create (&t, NULL, test_sleep, NULL) == 0);
      /* Sleep for a while; then cancel */
      Sleep (100);
      assert (pthread_cancel (t) == 0);
      assert (pthread_join (t, &result) == 0);
      assert (result == PTHREAD_CANCELED && "test_sleep" != NULL);

      printf ("Cancel waiting thread.\n");
      assert (pthread_create (&t, NULL, test_wait, NULL) == 0);
      /* Sleep for a while; then cancel. */
      Sleep (100);
      assert (pthread_cancel (t) == 0);
      assert (pthread_join (t, &result) == 0);
      assert (result == PTHREAD_CANCELED && "test_wait");

      printf ("Cancel blocked thread (blocked on network I/O).\n");
      assert (pthread_create (&t, NULL, test_udp, NULL) == 0);
      /* Sleep for a while; then cancel. */
      Sleep (100);
      assert (pthread_cancel (t) == 0);
      assert (pthread_join (t, &result) == 0);
      assert (result == PTHREAD_CANCELED && "test_udp" != NULL);
    }
  else
    {
      printf ("Alertable async cancel not available.\n");
    }

  /*
   * Success.
   */
  return 0;
}
