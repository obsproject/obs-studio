File Watcher
============

A basic wrapper that notifies when a file change is detected.

Runs file checks on a single, global shared thread. This avoids issues related to system IO delays,
which can block important threads in the process.

Checks must be triggered via :c:func:`file_watcher_check()` to asynchronously queue a check. Common implementations
use existing interval-based callbacks available within OBS to pump this call, like :c:member:`obs_source_info.video_tick`.

The helper is also architected to prevent a large backlog of checks. At most, there may only be one check running, and one check
pending, for any individual watcher. This ensures that a heavily-loaded check thread is naturally rate-limited, rather than falling
further and further behind.

The callback will be executed upon file create, file remove, size change, and modified-time change. Every check compares the current
file state with the state observed during the previous check. If multiple changes occur between checks, the callback will only be
executed once.

.. code:: cpp

	#include <util/file-watcher.h>

.. type:: file_watcher_t

	Opaque file watcher object

File Watcher Functions
----------------------


.. function:: file_watcher_t *file_watcher_create(const char *path, file_watcher_cb callback, void *data)

	Creates a file watcher, initializing the global file watcher thread
	if this is the first watcher.

	:param path:     File path to monitor
	:param callback: Function to call when a change is detected
	:param data:     Private data passed to the callback
	:return:         The file watcher, or *NULL* if failure

	.. versionadded:: 33.2

	Relevant data types used with this function:

.. code:: cpp

   typedef bool (*file_watcher_cb)(void *data, char *path);

----------------------

.. function:: void file_watcher_destroy(file_watcher_t *fw)

	Destroys a file watcher. If it's the last watcher, the global watcher
	thread will be destroyed, too.

	New checks should cease before calling this function. Additionally,
	no pending check callbacks will be executed after this function
	returns. It is therefore safe to free the associated data immediately.

	This function may not be called by the file watcher callback.

	.. versionadded:: 33.2

----------------------

.. function:: void file_watcher_check(file_watcher_t *fw)

	Schedules a check of the file for new changes on the global watcher
	thread.

	This will not block on slow file systems, and does nothing if there
	is already a check scheduled.

	This function is thread-safe, but may not be called concurrently
	with :c:func:`file_watcher_destroy()`.

	.. versionadded:: 32.2
