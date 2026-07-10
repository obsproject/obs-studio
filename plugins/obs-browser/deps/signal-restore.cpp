/******************************************************************************
 Copyright (C) 2022 by Kyle Manning <tt2468@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <signal.h>
#include <string.h>

#include "signal-restore.hpp"

// Method here borrowed from https://bitbucket.org/chromiumembedded/java-cef/src/master/native/signal_restore_posix.cpp

#ifndef _WIN32
template<typename T, size_t N> char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

const int signals_to_restore[] = {SIGHUP,  SIGINT,  SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV,
				  SIGALRM, SIGTERM, SIGCHLD, SIGBUS, SIGTRAP, SIGPIPE};
struct sigaction signal_handlers[arraysize(signals_to_restore)];

void BackupSignalHandlers()
{
	struct sigaction sigact;
	for (unsigned i = 0; i < arraysize(signals_to_restore); ++i) {
		memset(&sigact, 0, sizeof(sigact));
		sigaction(signals_to_restore[i], nullptr, &sigact);
		signal_handlers[i] = sigact;
	}
}

void RestoreSignalHandlers()
{
	for (unsigned i = 0; i < arraysize(signals_to_restore); ++i) {
		sigaction(signals_to_restore[i], &signal_handlers[i], nullptr);
	}
}
#endif
