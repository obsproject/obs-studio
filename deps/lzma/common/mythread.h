///////////////////////////////////////////////////////////////////////////////
//
/// \file       mythread.h
/// \brief      Wrappers for threads
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"


#ifdef HAVE_PTHREAD
#	include <pthread.h>

#	define mythread_once(func) \
	do { \
		static pthread_once_t once_ = PTHREAD_ONCE_INIT; \
		pthread_once(&once_, &func); \
	} while (0)

#	define mythread_sigmask(how, set, oset) \
		pthread_sigmask(how, set, oset)

#else

#	define mythread_once(func) \
	do { \
		static bool once_ = false; \
		if (!once_) { \
			func(); \
			once_ = true; \
		} \
	} while (0)

#	define mythread_sigmask(how, set, oset) \
		sigprocmask(how, set, oset)

#endif
