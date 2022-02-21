/*
 * ptw32_OLL_lock.c
 *
 * Description:
 * This translation unit implements extended reader/writer queue-based locks.
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
 */

/*
 * About the OLL lock (Scalable Reader-Writer Lock):
 *
 * OLL locks are queue-based locks similar to the MCS queue lock, where the queue
 * nodes are local to the thread but where reader threads can enter the critical
 * section immediately without going through a central guard lock if there are
 * already readers holding the lock.
 *
 * Covered by United States Patent Application 20100241774 (Oracle)
 */

#include "pthread.h"
#include "sched.h"
#include "implement.h"

/*
 * C-SNZI support
 */
typedef union  ptw32_oll_counter_t_		ptw32_oll_counter_t;
typedef struct ptw32_oll_snziRoot_t_		ptw32_oll_snziRoot_t;
typedef struct ptw32_oll_snziNode_t_		ptw32_oll_snziNode_t;
typedef union  ptw32_oll_snziNodeOrRoot_t_	ptw32_oll_snziNodeOrRoot_t;
typedef struct ptw32_oll_queryResult_t_		ptw32_oll_queryResult_t;
typedef struct ptw32_oll_ticket_t_		ptw32_oll_ticket_t;
typedef struct ptw32_oll_csnzi_t_		ptw32_oll_csnzi_t;

enum
{
  ptw32_archWidth	= sizeof(size_t)*8,
  ptw32_oll_countWidth	= ptw32_archWidth-2
};

#define PTW32_OLL_MAXREADERS (((size_t)2<<(ptw32_oll_countWidth-1))-1)

union ptw32_oll_counter_t_
{
  size_t	word	: ptw32_archWidth;
  struct
  {
    /*
     * This needs to be a single word
     *
     *    ------------------------------------
     *    | STATE |  ROOT  | COUNT (readers) |
     *    ------------------------------------
     *     63 / 31  62 / 30   61 / 29 .. 0
     */
    size_t	count	: ptw32_oll_countWidth;
    size_t	root	: 1;			/* ROOT or NODE */
    size_t	state	: 1;			/* OPEN or CLOSED (root only) */
  } internal;
};

struct ptw32_oll_snziRoot_t_
{
  /*
   * "counter" must be at same offset in both
   * ptw32_oll_snziNode_t and ptw32_oll_snziRoot_t
   */
  ptw32_oll_counter_t	counter;
};

enum
{
  ptw32_oll_snziRoot_open	= 0,
  ptw32_oll_snziRoot_closed	= 1
};

enum
{
  ptw32_oll_snzi_root	= 0,
  ptw32_oll_snzi_node	= 1
};

/*
 * Some common SNZI root whole-word states that can be used to set or compare
 * root words with a single operation.
 */
ptw32_oll_snziRoot_t ptw32_oll_snziRoot_openAndZero = {.counter.internal.count = 0,
                                                       .counter.internal.root = ptw32_oll_snzi_root,
                                                       .counter.internal.state = ptw32_oll_snziRoot_open};
ptw32_oll_snziRoot_t ptw32_oll_snziRoot_closedAndZero = {.counter.internal.count = 0,
                                                         .counter.internal.root = ptw32_oll_snzi_root,
                                                         .counter.internal.state = ptw32_oll_snziRoot_closed};

struct ptw32_oll_queryResult_t_
{
  BOOL	nonZero;
  BOOL	open;
};

union ptw32_oll_snziNodeOrRoot_t_
{
  ptw32_oll_snziRoot_t* rootPtr;
  ptw32_oll_snziNode_t* nodePtr;
};

struct ptw32_oll_snziNode_t_
{
  /* "counter" must be at same offset in both
   * ptw32_oll_snziNode_t and ptw32_oll_snziRoot_t
   */
  ptw32_oll_counter_t		counter;
  ptw32_oll_snziNodeOrRoot_t	parentPtr;
};

struct ptw32_oll_ticket_t_
{
  ptw32_oll_snziNodeOrRoot_t	snziNodeOrRoot;
};

ptw32_oll_ticket_t ptw32_oll_ticket_null = {NULL};

struct ptw32_oll_csnzi_t_
{
  ptw32_oll_snziRoot_t	proxyRoot;
  ptw32_oll_snziNode_t	leafs[];
};

/*
 * FOLL lock support
 */

typedef struct ptw32_foll_node_t_ ptw32_foll_node_t;
typedef struct ptw32_foll_local_t_ ptw32_foll_local_t;
typedef struct ptw32_foll_rwlock_t_ ptw32_foll_rwlock_t;

enum
{
  ptw32_srwl_reader,
  ptw32_srwl_writer
};

enum
{
  ptw32_srwl_free,
  ptw32_srwl_in_use
};

struct ptw32_foll_node_t_
{
  ptw32_foll_node_t*	qNextPtr;
  ptw32_oll_csnzi_t*	csnziPtr;
  ptw32_foll_node_t*	nextPtr;
  int			kind;
  int			allocState;
  BOOL			spin;
};

struct ptw32_foll_local_t_
{
  ptw32_foll_node_t*	rNodePtr; // Default read node. Immutable
  ptw32_foll_node_t*	wNodePtr; // Write node. Immutable.
  ptw32_foll_node_t*	departFromPtr; // List node we last arrived at.
  ptw32_oll_ticket_t	ticket; // C-SNZI ticket
};

struct ptw32_foll_rwlock_t_
{
  ptw32_foll_node_t*	tailPtr;
  ptw32_foll_node_t*	rNodesPtr; // Head of reader node
};

/*
 * ShouldArriveAtTree() returns true if:
 *   the compare_exchange in Arrive() fails too often under read access; or
 *   ??
 * Note that this is measured across all access to 
 * this lock, not just this attempt, so that highly
 * read-contended locks will use C-SNZI. Lightly
 * read-contended locks can reduce memory usage and some
 * processing by using the root directly.
 */
BOOL
ptw32_oll_ShouldArriveAtTree()
{
  return PTW32_FALSE;
}

size_t
ptw32_oll_GetLeafForThread()
{
  return 0;
}

/*
 * Only readers call ptw32_oll_Arrive()
 *
 * Checks whether the C-SNZI state is OPEN, and if so,
 * increments the surplus of the C-SNZI by either directly
 * arriving at the root node, or calling TreeArrive on one
 * of the leaf nodes. Returns a ticket pointing to the node
 * that was arrived at. If the state is CLOSED, makes no
 * change and returns a ticket that contains no pointer.
 */
ptw32_oll_ticket_t
ptw32_oll_Arrive(ptw32_oll_csnzi_t* csnzi)
{
  for (;;)
  {
    ptw32_oll_ticket_t ticket;
    ptw32_oll_snziRoot_t oldProxy = csnzi->proxyRoot;
    if (oldProxy.counter.internal.state != ptw32_oll_snziRoot_open)
    {
      ticket.snziNodeOrRoot.rootPtr = (ptw32_oll_snziRoot_t*)NULL;
      return ticket;
    }
    if (!ptw32_oll_ShouldArriveAtTree())
    {
      ptw32_oll_snziRoot_t newProxy = oldProxy;
      newProxy.counter.internal.count++;
      if (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                (PTW32_INTERLOCKED_SIZEPTR)&csnzi->proxyRoot.counter,
                (PTW32_INTERLOCKED_SIZE)newProxy.counter.word,
                (PTW32_INTERLOCKED_SIZE)oldProxy.counter.word)
          == (PTW32_INTERLOCKED_SIZE)oldProxy.counter.word)
      {
        /* Exchange successful */
        ticket.snziNodeOrRoot.rootPtr = &csnzi->proxyRoot;
        return ticket;
      }
    }
    else
    {
      ptw32_oll_snziNode_t* leafPtr = &csnzi->leafs[ptw32_oll_GetLeafForThread()];
      ticket.snziNodeOrRoot.nodePtr = (ptw32_oll_TreeArrive(leafPtr) ? leafPtr : (ptw32_oll_snziNode_t*)NULL);
      return ticket;
    }
  }
}

/*
 * Decrements the C-SNZI surplus. Returns false iff the
 * resulting state is CLOSED and the surplus is zero.
 * Ticket must have been returned by an arrival. Must have
 * received this ticket from Arrive more times than Depart
 * has been called with the ticket. (Thus, the surplus
 * must be greater than zero.)
 */
BOOL
ptw32_oll_Depart(ptw32_oll_ticket_t ticket)
{
  return ptw32_oll_TreeDepart(ticket.snziNodeOrRoot);
}

/*
 * Increments the C-SNZI surplus and returns true if the
 * C-SNZI is open or has a surplus. Calls TreeArrive
 * recursively on the node’s parent if needed.
 * Otherwise, returns false without making any changes.
 */
BOOL
ptw32_oll_TreeArrive(ptw32_oll_snziNodeOrRoot_t snziNodeOrRoot)
{
  if (snziNodeOrRoot.nodePtr->counter.internal.root != ptw32_oll_snzi_root)
  {
    /* Non-root node */
    ptw32_oll_counter_t newCounter, oldCounter;
    BOOL arrivedAtParent = PTW32_FALSE;
    do
    {
      oldCounter = snziNodeOrRoot.nodePtr->counter;
      if (0 == oldCounter.internal.count && !arrivedAtParent)
      {
        if (ptw32_oll_TreeArrive(snziNodeOrRoot.nodePtr->parentPtr))
          arrivedAtParent = PTW32_TRUE;
        else
          return PTW32_FALSE;
      }
      newCounter = oldCounter;
      newCounter.internal.count++;
    } while (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                  (PTW32_INTERLOCKED_SIZEPTR)&snziNodeOrRoot.nodePtr->counter,
                  (PTW32_INTERLOCKED_SIZE)newCounter.word,
                  (PTW32_INTERLOCKED_SIZE)oldCounter.word)
             != (PTW32_INTERLOCKED_SIZE)oldCounter.word);
    if (newCounter.internal.count != 0 && arrivedAtParent)
      ptw32_oll_TreeDepart(snziNodeOrRoot.nodePtr->parentPtr);
    return PTW32_TRUE;
  }
  else
  {
    /* Root node */
    ptw32_oll_snziRoot_t newRoot, oldRoot;
    do
    {
      oldRoot = *(ptw32_oll_snziRoot_t*)snziNodeOrRoot.rootPtr;
      if (oldRoot.counter.word == ptw32_oll_snziRoot_closedAndZero.counter.word)
        return PTW32_FALSE;
      newRoot = oldRoot;
      newRoot.counter.internal.count++;
    } while (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                   (PTW32_INTERLOCKED_SIZEPTR)&snziNodeOrRoot.rootPtr->counter,
                   (PTW32_INTERLOCKED_SIZE)newRoot.counter.word,
                   (PTW32_INTERLOCKED_SIZE)oldRoot.counter.word)
             != (PTW32_INTERLOCKED_SIZE)oldRoot.counter.word);
    return PTW32_TRUE;
  }
}

/*
 * Decrements the C-SNZI surplus, calling TreeDepart
 * recursively on the node’s parent if needed. Returns
 * false iff the resulting state of the C-SNZI is CLOSED
 * and the surplus is zero. Otherwise, returns true.
 */
BOOL
ptw32_oll_TreeDepart(ptw32_oll_snziNodeOrRoot_t snziNodeOrRoot)
{
  if (snziNodeOrRoot.nodePtr->counter.internal.root != ptw32_oll_snzi_root)
  {
    /* Non-root node */
    ptw32_oll_counter_t newCounter, oldCounter;
    do
    {
      newCounter = oldCounter = snziNodeOrRoot.nodePtr->counter;
      newCounter.internal.count--;
    } while (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                   (PTW32_INTERLOCKED_SIZEPTR)&snziNodeOrRoot.nodePtr->counter,
                   (PTW32_INTERLOCKED_SIZE)newCounter.word,
                   (PTW32_INTERLOCKED_SIZE)oldCounter.word)
             != (PTW32_INTERLOCKED_SIZE)oldCounter.word);
    return (0 == newCounter.internal.count)
             ? ptw32_oll_TreeDepart(snziNodeOrRoot.nodePtr->parentPtr)
             : PTW32_TRUE;
  }
  else
  {
    /* Root node */
    ptw32_oll_snziRoot_t newRoot, oldRoot;
    do
    {
      newRoot = oldRoot = *(ptw32_oll_snziRoot_t*)snziNodeOrRoot.rootPtr;
      newRoot.counter.internal.count--;
    } while (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                   (PTW32_INTERLOCKED_SIZEPTR)&snziNodeOrRoot.rootPtr->counter,
                   (PTW32_INTERLOCKED_SIZE)newRoot.counter.word,
                   (PTW32_INTERLOCKED_SIZE)oldRoot.counter.word)
             != (PTW32_INTERLOCKED_SIZE)oldRoot.counter.word);
    return (newRoot.counter.word != ptw32_oll_snziRoot_closedAndZero.counter.word);
  }
}

/*
 * Opens a C-SNZI object. Requires C-SNZI state to be
 * CLOSED and the surplus to be zero.
 */
void
ptw32_oll_Open(ptw32_oll_csnzi_t* csnziPtr)
{
  csnziPtr->proxyRoot = ptw32_oll_snziRoot_openAndZero;
}

/*
 * Opens a C-SNZI object while atomically performing count
 * arrivals. Requires C-SNZI state to be CLOSED and
 * the surplus to be zero.
 */
void
ptw32_oll_OpenWithArrivals(ptw32_oll_csnzi_t* csnziPtr, size_t count, BOOL close)
{
  csnziPtr->proxyRoot.counter.internal.count = count;
  csnziPtr->proxyRoot.counter.internal.state = (close ? ptw32_oll_snziRoot_closed : ptw32_oll_snziRoot_open);
}

/*
 * Closes a C-SNZI object. Returns true iff the C-SNZI
 * state changed from OPEN to CLOSED and the surplus is
 * zero.
 */
BOOL
ptw32_oll_Close(ptw32_oll_csnzi_t* csnziPtr)
{
  ptw32_oll_snziRoot_t newProxy, oldProxy;
  do
  {
    oldProxy = csnziPtr->proxyRoot;
    if (oldProxy.counter.internal.state != ptw32_oll_snziRoot_open)
    {
      return PTW32_FALSE;
    }
    newProxy = oldProxy;
    newProxy.counter.internal.state = ptw32_oll_snziRoot_closed;
  } while (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                 (PTW32_INTERLOCKED_SIZEPTR)&csnziPtr->proxyRoot.counter,
                 (PTW32_INTERLOCKED_SIZE)newProxy.counter.word,
                 (PTW32_INTERLOCKED_SIZE)oldProxy.counter.word)
           != (PTW32_INTERLOCKED_SIZE)oldProxy.counter.word);
  return (newProxy.counter.word == ptw32_oll_snziRoot_closedAndZero.counter.word);
}

/*
 * Closes a C-SNZI if its surplus is zero. Otherwise, does
 * nothing. Returns true iff C-SNZI state changed from
 * OPEN to CLOSED.
 */
BOOL
ptw32_oll_CloseIfEmpty(ptw32_oll_csnzi_t* csnziPtr)
{
  ptw32_oll_snziRoot_t newProxy, oldProxy;
  do
  {
    oldProxy = csnziPtr->proxyRoot;
    if (oldProxy.counter.word != ptw32_oll_snziRoot_openAndZero.counter.word)
    {
      return PTW32_FALSE;
    }
    newProxy = ptw32_oll_snziRoot_closedAndZero;
  } while (PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
                 (PTW32_INTERLOCKED_SIZEPTR)&csnziPtr->proxyRoot.counter,
                 (PTW32_INTERLOCKED_SIZE)newProxy.counter.word,
                 (PTW32_INTERLOCKED_SIZE)oldProxy.counter.word)
           != (PTW32_INTERLOCKED_SIZE)oldProxy.counter.word);
  return PTW32_TRUE;
}

/*
 * Returns whether the C-SNZI has a nonzero surplus and
 * whether the C-SNZI is open.
 * "nonZero" doesn't appear to be used anywhere in the algorithms.
 */
ptw32_oll_queryResult_t
ptw32_oll_Query(ptw32_oll_csnzi_t* csnziPtr)
{
  ptw32_oll_queryResult_t query;
  ptw32_oll_snziRoot_t proxy = csnziPtr->proxyRoot;

  query.nonZero = (proxy.counter.internal.count > 0);
  query.open = (proxy.counter.internal.state == ptw32_oll_snziRoot_open);
  return query;
}

/*
 * Returns whether the Arrive operation that returned
 * the ticket succeeded.
 */
BOOL
ptw32_oll_Arrived(ptw32_oll_ticket_t t)
{
  return (t.snziNodeOrRoot.nodePtr != NULL);
}

/*
 * Constructs and returns a ticket that can be used to
 * depart from the root node.
 */
ptw32_oll_ticket_t
ptw32_oll_DirectTicket(ptw32_oll_csnzi_t* csnziPtr)
{
  ptw32_oll_ticket_t ticket;
  ticket.snziNodeOrRoot.rootPtr = &csnziPtr->proxyRoot;
  return ticket;
}

/* Scalable RW Locks */

typedef struct ptw32_srwl_rwlock_t_ ptw32_srwl_rwlock_t;
typedef struct ptw32_srwl_node_t_ ptw32_srwl_node_t;
typedef struct ptw32_srwl_local_t_ ptw32_srwl_local_t;

enum
{
  ptw32_srwl_reader	= 0,
  ptw32_srwl_writer	= 1
};

enum
{
  ptw32_srwl_free	= 0,
  ptw32_srwl_in_use	= 1
};

struct ptw32_srwl_rwlock_t_
{
  ptw32_srwl_node_t* tailPtr;
  ptw32_srwl_node_t* readerNodePtr;
};

struct ptw32_srwl_node_t_
{
  ptw32_srwl_node_t*	qNextPtr;
  ptw32_oll_csnzi_t*	csnziPtr;
  ptw32_srwl_node_t*	nextReaderPtr;
  int			kind;		/* ptw32_srwl_reader, ptw32_srwl_writer */
  int			allocState;	/* ptw32_srwl_free, ptw32_srwl_in_use */
  BOOL			spin;
};

/*
 * When a ptw32_srwl_local_t is instantiated the "kind" of each of
 * rNode and wNode must be set as appropriate. This is the only
 * time "kind" is set.
 */
struct ptw32_srwl_local_t_
{
  ptw32_srwl_node_t*	rNodePtr;
  ptw32_srwl_node_t*	wNodePtr;
  ptw32_srwl_node_t*	departFromPtr;
  ptw32_oll_ticket_t	ticket;
};

/* Allocates a new reader node. */
ptw32_srwl_node_t*
ptw32_srwl_AllocReaderNode(ptw32_srwl_local_t* local)
{
  ptw32_srwl_node_t* currNodePtr = local->rNodePtr;
  for (;;)
  {
    if (currNodePtr->allocState == ptw32_srwl_free)
    {
      if (PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG(
            (PTW32_INTERLOCKED_LONGPTR)&currNodePtr->allocState,
            (PTW32_INTERLOCKED_LONG)ptw32_srwl_in_use,
            (PTW32_INTERLOCKED_LONG)ptw32_srwl_free)
          == (PTW32_INTERLOCKED_LONG)ptw32_srwl_in_use)
      {
        return currNodePtr;
      }
    }
    currNodePtr = currNodePtr->next;
  }
}

/*
 * Frees a reader node. Requires that its allocState
 * is ptw32_srwl_in_use.
 */
void
ptw32_srwl_FreeReaderNode(ptw32_srwl_node_t* nodePtr)
{
  nodePtr->allocState := ptw32_srwl_free;
}

void
ptw32_srwl_WriterLock(ptw32_srwl_rwlock_t* lockPtr, ptw32_srwl_local_t* localPtr)
{
  oldTailPtr = (ptw32_srwl_rwlock_t*)PTW32_INTERLOCKED_EXCHANGE_PTR(
                                       (PTW32_INTERLOCKED_PVOID_PTR)&lockPtr->tailPtr,
                                       (PTW32_INTERLOCKED_PVOID)localPtr->wNodePtr);
  if (oldTailPtr != NULL)
  {
    localPtr->wNodePtr->spin := PTW32_TRUE;
    oldTailPtr->qNextPtr = localPtr->wNodePtr;
    if (oldTailPtr->kind == ptw32_srwl_writer)
    {
      while (localPtr->wNodePtr->spin);
    }
    else
    {
      /* Wait until node is properly recycled */
      while (ptw32_oll_Query(oldTailPtr->csnzi).open);
      /*
       * Close C-SNZI of previous reader node.
       * If there are no readers to signal us, spin on
       * previous node and free it before entering
       * critical section.
       */
      if (ptw32_oll_Close(oldTailPtr->csnzi))
      {
        while (oldTailPtr->spin);
        ptw32_srwl_FreeReaderNode(oldTailPtr);
      }
      else
      {
        while (localPtr->wNodePtr->spin);
      }
    }
  }
}

void
ptw32_srwl_WriterUnlock(ptw32_srwl_rwlock_t* lockPtr, ptw32_srwl_local_t* localPtr)
{
  if (localPtr->wNodePtr->qNextPtr == NULL)
  {
    if (PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR(
              (PTW32_INTERLOCKED_PVOIDPTR)&lockPtr->tailPtr,
              (PTW32_INTERLOCKED_PVOID)NULL,
              (PTW32_INTERLOCKED_PVOID)localPtr->wNodePtr)
        == (PTW32_INTERLOCKED_PVOID)NULL)
    {
      return;
    }
    else
    {
      while (localPtr->wNodePtr->qNextPtr == NULL);
    }
  }
  /* Clean up */
  localPtr->wNodePtr->qNextPtr->spin = PTW32_FALSE;
  localPtr->wNodePtr->qNextPtr = NULL;
}

void
ptw32_srwl_ReaderLock(ptw32_srwl_rwlock_t* lockPtr, ptw32_srwl_local_t* localPtr)
{
  ptw32_srwl_node_t* rNodePtr = NULL;
  for (;;)
  {
    ptw32_srwl_node_t* tailPtr = lockPtr->tailPtr;
    /* If no nodes are in the queue */
    if (tailPtr == NULL)
    {
      if (rNodePtr == NULL)
      {
        rNodePtr = ptw32_srwl_AllocReaderNode(localPtr);
      }
      rNodePtr->spin = PTW32_FALSE;
      if (PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR(
                (PTW32_INTERLOCKED_PVOIDPTR)&lockPtr->tailPtr,
                (PTW32_INTERLOCKED_PVOID)rNodePtr,
                (PTW32_INTERLOCKED_PVOID)NULL)
          == (PTW32_INTERLOCKED_PVOID)rNodePtr)
      {
        ptw32_oll_Open(rNodePtr->csnzi);
        localPtr->ticket = ptw32_oll_Arrive(rNodePtr->csnzi);
        if (ptw32_oll_Arrived(localPtr->ticket))
        {
          localPtr->departFromPtr = rNodePtr;
          return;
        }
        /* Avoid reusing inserted node */
        rNodePtr = NULL;
      }
    }
    /* Otherwise, there is a node in the queue */
    else
    {
      /* Is last node a writer node? */
      if (tailPtr->kind == ptw32_srwl_writer)
      {
        if (rNodePtr == NULL)
        {
          rNodePtr = ptw32_srwl_AllocReaderNode(localPtr);
        }
        rNodePtr->spin = PTW32_TRUE;
        if (PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR(
                  (PTW32_INTERLOCKED_PVOIDPTR)&lockPtr->tailPtr,
                  (PTW32_INTERLOCKED_PVOID)rNodePtr,
                  (PTW32_INTERLOCKED_PVOID)tailPtr)
            == (PTW32_INTERLOCKED_PVOID)rNodePtr)
        {
          tailPtr->qNextPtr = rNodePtr;
          localPtr->ticket = ptw32_oll_Arrive(rNodePtr->csnzi);
          if (ptw32_oll_Arrived(localPtr->ticket))
          {
            localPtr->departFromPtr = rNodePtr;
            while (rNodePtr->spin);
            return;
          }
          /* Avoid reusing inserted node */
          rNodePtr = NULL;
        }
      }
      /*
       * Otherwise, last node is a reader node.
       * (tailPtr->kind == ptw32_srwl_reader)
       */
      else
      {
        localPtr->ticket = ptw32_oll_Arrive(tailPtr->csnzi);
        if (ptw32_oll_Arrived(localPtr->ticket))
        {
          if (rNodePtr != NULL)
          {
            ptw32_srwl_FreeReaderNode(rNodePtr);
          }
          localPtr->departFromPtr = tailPtr;
          while (tailPtr->spin);
          return;
        }
      }
    }
  }
}

void
ptw32_srwl_ReaderUnlock(ptw32_srwl_rwlock_t* lockPtr, ptw32_srwl_local_t* localPtr)
{
  if (ptw32_oll_Depart(localPtr->departFromPtr->csnzi, localPtr->ticket))
  {
    return;
  }
  /* Clean up */
  localPtr->departFromPtr->qNextPtr->spin = PTW32_FALSE;
  localPtr->departFromPtr->qNextPtr = NULL;
  ptw32_srwl_FreeReaderNode(localPtr->departFromPtr);
}


#include <stdio.h>

int main()
{
  printf("%lx\n", PTW32_OLL_MAXREADERS);
  return 0;
}

