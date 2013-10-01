/* Simple POSIX threads program.
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
 * Author: Eyal Lebedinsky eyal@eyal.emu.id.au
 * Written: Sep 1998.
 * Version Date: 12 Sep 1998
 *
 * Do we need to lock stdout or is it thread safe?
 *
 * Used:
 *	pthread_t
 *	pthread_attr_t
 *	pthread_create()
 *	pthread_join()
 *	pthread_mutex_t
 *	PTHREAD_MUTEX_INITIALIZER
 *	pthread_mutex_init() [not used now]
 *	pthread_mutex_destroy()
 *	pthread_mutex_lock()
 *	pthread_mutex_trylock()
 *	pthread_mutex_unlock()
 *
 * What this program does is establish a work queue (implemented using
 * four mutexes for each thread). It then schedules work (by storing
 * a number in 'todo') and releases the threads. When the work is done
 * the threads will block. The program then repeats the same thing once
 * more (just to test the logic) and when the work is done it destroyes
 * the threads.
 *
 * The 'work' we do is simply burning CPU cycles in a loop.
 * The 'todo' work queue is trivial - each threads pops one element
 * off it by incrementing it, the poped number is the 'work' to do.
 * When 'todo' reaches the limit (nwork) the queue is considered
 * empty.
 *
 * The number displayed at the end is the amount of work each thread
 * did, so we can see if the load was properly distributed.
 *
 * The program was written to test a threading setup (not seen here)
 * rather than to demonstrate correct usage of the pthread facilities.
 *
 * Note how each thread is given access to a thread control structure
 * (TC) which is used for communicating to/from the main program (e.g.
 * the threads knows its 'id' and also filles in the 'work' done).
*/

#include "test.h"

#include <stdlib.h>
#include <math.h>

struct thread_control {
  int		id;
  pthread_t	thread;		/* thread id */
  pthread_mutex_t	mutex_start;
  pthread_mutex_t	mutex_started;
  pthread_mutex_t	mutex_end;
  pthread_mutex_t	mutex_ended;
  long		work;		/* work done */
  int		stat;		/* pthread_init status */
};

typedef struct thread_control	TC;

static TC		*tcs = NULL;
static int		nthreads = 10;
static int		nwork = 100;
static int		quiet = 0;

static int		todo = -1;

static pthread_mutex_t	mutex_todo = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	mutex_stdout = PTHREAD_MUTEX_INITIALIZER;


static void
die (int ret)
{
  if (NULL != tcs)
    {
      free (tcs);
      tcs = NULL;
    }

  if (ret)
    exit (ret);
}


static double
waste_time (int n)
{
  int		i;
  double	f, g, h, s;

  s = 0.0;

  /*
   * Useless work.
   */
  for (i = n*100; i > 0; --i)
    {
      f = rand ();
      g = rand ();
      h = rand ();
      s += 2.0 * f * g / (h != 0.0 ? (h * h) : 1.0);
    }
  return s;
}

static int
do_work_unit (int who, int n)
{
  int		i;
  static int	nchars = 0;
  double	f = 0.0;

  if (quiet)
    i = 0;
  else {
    /*
     * get lock on stdout
     */
    assert(pthread_mutex_lock (&mutex_stdout) == 0);

    /*
     * do our job
     */
    i = printf ("%c", "0123456789abcdefghijklmnopqrstuvwxyz"[who]);

    if (!(++nchars % 50))
      printf ("\n");

    fflush (stdout);

    /*
     * release lock on stdout
     */
    assert(pthread_mutex_unlock (&mutex_stdout) == 0);
  }

  n = rand () % 10000;	/* ignore incoming 'n' */
  f = waste_time (n);

  /* This prevents the statement above from being optimised out */
  if (f > 0.0)
    return(n);

  return (n);
}

static int
print_server (void *ptr)
{
  int		mywork;
  int		n;
  TC		*tc = (TC *)ptr;

  assert(pthread_mutex_lock (&tc->mutex_started) == 0);

  for (;;)
    {
      assert(pthread_mutex_lock (&tc->mutex_start) == 0);
      assert(pthread_mutex_unlock (&tc->mutex_start) == 0);
      assert(pthread_mutex_lock (&tc->mutex_ended) == 0);
      assert(pthread_mutex_unlock (&tc->mutex_started) == 0);

      for (;;)
	{

	  /*
	   * get lock on todo list
	   */
	  assert(pthread_mutex_lock (&mutex_todo) == 0);

	  mywork = todo;
	  if (todo >= 0)
	    {
	      ++todo;
	      if (todo >= nwork)
		todo = -1;
	    }
	  assert(pthread_mutex_unlock (&mutex_todo) == 0);

	  if (mywork < 0)
	    break;

	  assert((n = do_work_unit (tc->id, mywork)) >= 0);
	  tc->work += n;
	}

      assert(pthread_mutex_lock (&tc->mutex_end) == 0);
      assert(pthread_mutex_unlock (&tc->mutex_end) == 0);
      assert(pthread_mutex_lock (&tc->mutex_started) == 0);
      assert(pthread_mutex_unlock (&tc->mutex_ended) == 0);

      if (-2 == mywork)
	break;
    }

  assert(pthread_mutex_unlock (&tc->mutex_started) == 0);

  return (0);
}

static void
dosync (void)
{
  int		i;

  for (i = 0; i < nthreads; ++i)
    {
      assert(pthread_mutex_lock (&tcs[i].mutex_end) == 0);
      assert(pthread_mutex_unlock (&tcs[i].mutex_start) == 0);
      assert(pthread_mutex_lock (&tcs[i].mutex_started) == 0);
      assert(pthread_mutex_unlock (&tcs[i].mutex_started) == 0);
    }

  /*
   * Now threads do their work
   */
  for (i = 0; i < nthreads; ++i)
    {
      assert(pthread_mutex_lock (&tcs[i].mutex_start) == 0);
      assert(pthread_mutex_unlock (&tcs[i].mutex_end) == 0);
      assert(pthread_mutex_lock (&tcs[i].mutex_ended) == 0);
      assert(pthread_mutex_unlock (&tcs[i].mutex_ended) == 0);
    }
}

static void
dowork (void)
{
  todo = 0;
  dosync();

  todo = 0;
  dosync();
}

int
main (int argc, char *argv[])
{
  int		i;

  assert(NULL != (tcs = (TC *) calloc (nthreads, sizeof (*tcs))));

  /* 
   * Launch threads
   */
  for (i = 0; i < nthreads; ++i)
    {
      tcs[i].id = i;

      assert(pthread_mutex_init (&tcs[i].mutex_start, NULL) == 0);
      assert(pthread_mutex_init (&tcs[i].mutex_started, NULL) == 0);
      assert(pthread_mutex_init (&tcs[i].mutex_end, NULL) == 0);
      assert(pthread_mutex_init (&tcs[i].mutex_ended, NULL) == 0);

      tcs[i].work = 0;  

      assert(pthread_mutex_lock (&tcs[i].mutex_start) == 0);
      assert((tcs[i].stat = 
	      pthread_create (&tcs[i].thread,
			      NULL,
                  (void *(*)(void *))print_server,
                (void *) &tcs[i])
	      ) == 0);

      /* 
       * Wait for thread initialisation
       */
      {
	int trylock = 0;

	while (trylock == 0)
	  {
	    trylock = pthread_mutex_trylock(&tcs[i].mutex_started);
	    assert(trylock == 0 || trylock == EBUSY);

	    if (trylock == 0)
	      {
		assert(pthread_mutex_unlock (&tcs[i].mutex_started) == 0);
	      }
	  }
      }
    }

  dowork ();

  /*
   * Terminate threads
   */
  todo = -2;	/* please terminate */
  dosync();

  for (i = 0; i < nthreads; ++i)
    {
      if (0 == tcs[i].stat)
	assert(pthread_join (tcs[i].thread, NULL) == 0);
    }

  /* 
   * destroy locks
   */
  assert(pthread_mutex_destroy (&mutex_stdout) == 0);
  assert(pthread_mutex_destroy (&mutex_todo) == 0);

  /*
   * Cleanup
   */
  printf ("\n");

  /*
   * Show results
   */
  for (i = 0; i < nthreads; ++i)
    {
      printf ("%2d ", i);
      if (0 == tcs[i].stat)
	printf ("%10ld\n", tcs[i].work);
      else
	printf ("failed %d\n", tcs[i].stat);

      assert(pthread_mutex_unlock(&tcs[i].mutex_start) == 0);

      assert(pthread_mutex_destroy (&tcs[i].mutex_start) == 0);
      assert(pthread_mutex_destroy (&tcs[i].mutex_started) == 0);
      assert(pthread_mutex_destroy (&tcs[i].mutex_end) == 0);
      assert(pthread_mutex_destroy (&tcs[i].mutex_ended) == 0);
    }

  die (0);

  return (0);
}
