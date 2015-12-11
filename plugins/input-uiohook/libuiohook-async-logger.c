/* libUIOHook: Cross-platfrom userland keyboard and mouse hooking.
 * Copyright (C) 2006-2015 Alexander Barker. 2015 Christian Frisson. All Rights Received.
 * https://github.com/ChristianFrisson/libuiohook/
 *
 * libUIOHook is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libUIOHook is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libuiohook-async-logger.h"

// Thread and mutex variables.
#ifdef _WIN32
static HANDLE hook_thread;

static HANDLE hook_running_mutex;
static HANDLE hook_control_mutex;
static HANDLE hook_control_cond;
#else
static pthread_t hook_thread;

static pthread_mutex_t hook_running_mutex;
static pthread_mutex_t hook_control_mutex;
static pthread_cond_t hook_control_cond;
#endif

FILE * event_file = 0;

bool logger_proc(unsigned int level, const char *format, ...)
{
	bool status = false;

	va_list args;
	switch (level) {
#ifdef USE_DEBUG
	case LOG_LEVEL_DEBUG:
	case LOG_LEVEL_INFO:
		va_start(args, format);
		status = vfprintf(stdout, format, args) >= 0;
		va_end(args);
		break;
#endif

	case LOG_LEVEL_WARN:
	case LOG_LEVEL_ERROR:
		va_start(args, format);
		status = vfprintf(stderr, format, args) >= 0;
		va_end(args);
		break;
	}

	return status;
}

// NOTE: The following callback executes on the same thread that hook_run() is called
// from.  This is important because hook_run() attaches to the operating systems
// event dispatcher and may delay event delivery to the target application.
// Furthermore, some operating systems may choose to disable your hook if it
// takes to long to process.  If you need to do any extended processing, please
// do so by copying the event to your own queued dispatch thread.
void dispatch_proc(uiohook_event * const event)
{
	char buffer[256] = { 0 };
	size_t length = snprintf(buffer, sizeof(buffer),
	                         "id=%i,when=%" PRIu64 ",mask=0x%X",
	                         event->type, event->time, event->mask);

	switch (event->type) {
	case EVENT_HOOK_ENABLED:
		// Lock the running mutex so we know if the hook is enabled.
#ifdef _WIN32
		WaitForSingleObject(hook_running_mutex, INFINITE);
#else
		pthread_mutex_lock(&hook_running_mutex);
#endif


#ifdef _WIN32
		// Signal the control event.
		SetEvent(hook_control_cond);
#else
		// Unlock the control mutex so hook_enable() can continue.
		pthread_cond_signal(&hook_control_cond);
		pthread_mutex_unlock(&hook_control_mutex);
#endif
		break;

	case EVENT_HOOK_DISABLED:
		// Lock the control mutex until we exit.
#ifdef _WIN32
		WaitForSingleObject(hook_control_mutex, INFINITE);
#else
		pthread_mutex_lock(&hook_control_mutex);
#endif

		// Unlock the running mutex so we know if the hook is disabled.
#ifdef _WIN32
		ReleaseMutex(hook_running_mutex);
		ResetEvent(hook_control_cond);
#else
#if defined(__APPLE__) && defined(__MACH__)
		// Stop the main runloop so that this program ends.
		CFRunLoopStop(CFRunLoopGetMain());
#endif

		pthread_mutex_unlock(&hook_running_mutex);
#endif
		break;

	case EVENT_KEY_PRESSED:
		// If the escape key is pressed, naturally terminate the program.
		if (event->data.keyboard.keycode == VC_ESCAPE) {
			int status = hook_stop();
			switch (status) {
			// System level errors.
			case UIOHOOK_ERROR_OUT_OF_MEMORY:
				logger_proc(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
				break;

			case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
				// NOTE This is the only platform specific error that occurs on hook_stop().
				logger_proc(LOG_LEVEL_ERROR, "Failed to get XRecord context. (%#X)", status);
				break;

			// Default error.
			case UIOHOOK_FAILURE:
			default:
				logger_proc(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
				break;
			}
		}
	case EVENT_KEY_RELEASED:
		snprintf(buffer + length, sizeof(buffer) - length,
		         ",keycode=%u,rawcode=0x%X",
		         event->data.keyboard.keycode, event->data.keyboard.rawcode);
		break;

	case EVENT_KEY_TYPED:
		snprintf(buffer + length, sizeof(buffer) - length,
		         ",keychar=%lc,rawcode=%u",
		         (wint_t) event->data.keyboard.keychar,
		         event->data.keyboard.rawcode);
		break;

	case EVENT_MOUSE_PRESSED:
	case EVENT_MOUSE_RELEASED:
	case EVENT_MOUSE_CLICKED:
	case EVENT_MOUSE_MOVED:
	case EVENT_MOUSE_DRAGGED:
		snprintf(buffer + length, sizeof(buffer) - length,
		         ",x=%i,y=%i,button=%i,clicks=%i",
		         event->data.mouse.x, event->data.mouse.y,
		         event->data.mouse.button, event->data.mouse.clicks);
		break;

	case EVENT_MOUSE_WHEEL:
		snprintf(buffer + length, sizeof(buffer) - length,
		         ",type=%i,amount=%i,rotation=%i",
		         event->data.wheel.type, event->data.wheel.amount,
		         event->data.wheel.rotation);
		break;

	default:
		break;
	}

	//fprintf(stdout, "%s\n",	 buffer);
	if(event_file)fprintf(event_file, "%s\n",	 buffer);
}

#ifdef _WIN32
DWORD WINAPI hook_thread_proc(LPVOID arg)
{
#else
void *hook_thread_proc(void *arg)
{
#endif
	// Set the hook status.
	int status = hook_run();
	if (status != UIOHOOK_SUCCESS) {
#ifdef _WIN32
		*(DWORD *) arg = status;
#else
		*(int *) arg = status;
#endif
	}

	// Make sure we signal that we have passed any exception throwing code for
	// the waiting hook_enable().
#ifdef _WIN32
	SetEvent(hook_control_cond);

	return status;
#else
	// Make sure we signal that we have passed any exception throwing code for
	// the waiting hook_enable().
	pthread_cond_signal(&hook_control_cond);
	pthread_mutex_unlock(&hook_control_mutex);

	return arg;
#endif
}

int hook_enable()
{
	// Lock the thread control mutex.  This will be unlocked when the
	// thread has finished starting, or when it has fully stopped.
#ifdef _WIN32
	WaitForSingleObject(hook_control_mutex, INFINITE);
#else
	pthread_mutex_lock(&hook_control_mutex);
#endif

	// Set the initial status.
	int status = UIOHOOK_FAILURE;

#ifndef _WIN32
	// Create the thread attribute.
	pthread_attr_t hook_thread_attr;
	pthread_attr_init(&hook_thread_attr);

	// Get the policy and priority for the thread attr.
	int policy;
	pthread_attr_getschedpolicy(&hook_thread_attr, &policy);
	int priority = sched_get_priority_max(policy);
#endif

#if defined(_WIN32)
	DWORD hook_thread_id;
	DWORD *hook_thread_status = malloc(sizeof(DWORD));
	hook_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) hook_thread_proc, hook_thread_status, 0, &hook_thread_id);
	if (hook_thread != INVALID_HANDLE_VALUE) {
#else
	int *hook_thread_status = malloc(sizeof(int));
	if (pthread_create(&hook_thread, &hook_thread_attr, hook_thread_proc, hook_thread_status) == 0) {
#endif
#if defined(_WIN32)
		// Attempt to set the thread priority to time critical.
		if (SetThreadPriority(hook_thread, THREAD_PRIORITY_TIME_CRITICAL) == 0) {
			logger_proc(LOG_LEVEL_WARN, "%s [%u]: Could not set thread priority %li for thread %#p! (%#lX)\n",
			            __FUNCTION__, __LINE__, (long) THREAD_PRIORITY_TIME_CRITICAL,
			            hook_thread	, (unsigned long) GetLastError());
		}
#elif (defined(__APPLE__) && defined(__MACH__)) || _POSIX_C_SOURCE >= 200112L
		// Some POSIX revisions do not support pthread_setschedprio so we will
		// use pthread_setschedparam instead.
		struct sched_param param = { .sched_priority = priority };
		if (pthread_setschedparam(hook_thread, SCHED_OTHER, &param) != 0) {
			logger_proc(LOG_LEVEL_WARN,	"%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
			            __FUNCTION__, __LINE__, priority, (unsigned long) hook_thread);
		}
#else
		// Raise the thread priority using glibc pthread_setschedprio.
		if (pthread_setschedprio(hook_thread, priority) != 0) {
			logger_proc(LOG_LEVEL_WARN,	"%s [%u]: Could not set thread priority %i for thread 0x%lX!\n",
			            __FUNCTION__, __LINE__, priority, (unsigned long) hook_thread);
		}
#endif


		// Wait for the thread to indicate that it has passed the
		// initialization portion by blocking until either a EVENT_HOOK_ENABLED
		// event is received or the thread terminates.
		// NOTE This unlocks the hook_control_mutex while we wait.
#ifdef _WIN32
		WaitForSingleObject(hook_control_cond, INFINITE);
#else
		pthread_cond_wait(&hook_control_cond, &hook_control_mutex);
#endif

#ifdef _WIN32
		if (WaitForSingleObject(hook_running_mutex, 0) != WAIT_TIMEOUT) {
#else
		if (pthread_mutex_trylock(&hook_running_mutex) == 0) {
#endif
			// Lock Successful; The hook is not running but the hook_control_cond
			// was signaled!  This indicates that there was a startup problem!

			// Get the status back from the thread.
#ifdef _WIN32
			WaitForSingleObject(hook_thread,  INFINITE);
			GetExitCodeThread(hook_thread, hook_thread_status);
#else
			pthread_join(hook_thread, (void **) &hook_thread_status);
			status = *hook_thread_status;
#endif
		} else {
			// Lock Failure; The hook is currently running and wait was signaled
			// indicating that we have passed all possible start checks.  We can
			// always assume a successful startup at this point.
			status = UIOHOOK_SUCCESS;
		}

		free(hook_thread_status);

		logger_proc(LOG_LEVEL_DEBUG,	"%s [%u]: Thread Result: (%#X).\n",
		            __FUNCTION__, __LINE__, status);
	} else {
		status = UIOHOOK_ERROR_THREAD_CREATE;
	}

	// Make sure the control mutex is unlocked.
#ifdef _WIN32
	ReleaseMutex(hook_control_mutex);
#else
	pthread_mutex_unlock(&hook_control_mutex);
#endif

	return status;
}

static int start_status = UIOHOOK_FAILURE;

int test_hooking(void)
{

	// Lock the thread control mutex.  This will be unlocked when the
	// thread has finished starting, or when it has fully stopped.
#ifdef _WIN32
	// Create event handles for the thread hook.
	hook_running_mutex = CreateMutex(NULL, FALSE, TEXT("hook_running_mutex"));
	hook_control_mutex = CreateMutex(NULL, FALSE, TEXT("hook_control_mutex"));
	hook_control_cond = CreateEvent(NULL, TRUE, FALSE, TEXT("hook_control_cond"));
#else
	pthread_mutex_init(&hook_running_mutex, NULL);
	pthread_mutex_init(&hook_control_mutex, NULL);
	pthread_cond_init(&hook_control_cond, NULL);
#endif

	// Set the logger callback for library output.
	hook_set_logger_proc(&logger_proc);

	// Set the event callback for uiohook events.
	hook_set_dispatch_proc(&dispatch_proc);

	// Start the hook and block.
	// NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
	int status = hook_enable();
	start_status = status;
	switch (status) {
	case UIOHOOK_SUCCESS: {
		// We no longer block, so we need to explicitly wait for the thread to die.
		/*#ifdef _WIN32
		WaitForSingleObject(hook_thread,  INFINITE);
		#else
		#if defined(__APPLE__) && defined(__MACH__)
		// NOTE Darwin requires that you start your own runloop from main.
		CFRunLoopRun();
		#endif

		pthread_join(hook_thread, NULL);
		#endif*/
	}
	break;

	// System level errors.
	case UIOHOOK_ERROR_OUT_OF_MEMORY:
		logger_proc(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)\n", status);
		break;


	// X11 specific errors.
	case UIOHOOK_ERROR_X_OPEN_DISPLAY:
		logger_proc(LOG_LEVEL_ERROR, "Failed to open X11 display. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
		logger_proc(LOG_LEVEL_ERROR, "Unable to locate XRecord extension. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
		logger_proc(LOG_LEVEL_ERROR, "Unable to allocate XRecord range. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
		logger_proc(LOG_LEVEL_ERROR, "Unable to allocate XRecord context. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
		logger_proc(LOG_LEVEL_ERROR, "Failed to enable XRecord context. (%#X)\n", status);
		break;


	// Windows specific errors.
	case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
		logger_proc(LOG_LEVEL_ERROR, "Failed to register low level windows hook. (%#X)\n", status);
		break;


	// Darwin specific errors.
	case UIOHOOK_ERROR_AXAPI_DISABLED:
		logger_proc(LOG_LEVEL_ERROR, "Failed to enable access for assistive devices. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_CREATE_EVENT_PORT:
		logger_proc(LOG_LEVEL_ERROR, "Failed to create apple event port. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
		logger_proc(LOG_LEVEL_ERROR, "Failed to create apple run loop source. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_GET_RUNLOOP:
		logger_proc(LOG_LEVEL_ERROR, "Failed to acquire apple run loop. (%#X)\n", status);
		break;

	case UIOHOOK_ERROR_CREATE_OBSERVER:
		logger_proc(LOG_LEVEL_ERROR, "Failed to create apple run loop observer. (%#X)\n", status);
		break;

	// Default error.
	case UIOHOOK_FAILURE:
	default:
		logger_proc(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)\n", status);
		break;
	}

	return status;
}

const char* start_logging(char* event_file_name)
{

	if (!event_file_name || (event_file_name && !event_file_name[0])) {
		return "event file name empty";
	}

	event_file = fopen (event_file_name, "w+");
	setbuf(event_file, NULL);

	// Lock the thread control mutex.  This will be unlocked when the
	// thread has finished starting, or when it has fully stopped.
#ifdef _WIN32
	// Create event handles for the thread hook.
	hook_running_mutex = CreateMutex(NULL, FALSE, TEXT("hook_running_mutex"));
	hook_control_mutex = CreateMutex(NULL, FALSE, TEXT("hook_control_mutex"));
	hook_control_cond = CreateEvent(NULL, TRUE, FALSE, TEXT("hook_control_cond"));
#else
	pthread_mutex_init(&hook_running_mutex, NULL);
	pthread_mutex_init(&hook_control_mutex, NULL);
	pthread_cond_init(&hook_control_cond, NULL);
#endif

	// Set the logger callback for library output.
	hook_set_logger_proc(&logger_proc);

	// Set the event callback for uiohook events.
	hook_set_dispatch_proc(&dispatch_proc);

	// Start the hook and block.
	// NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
	int status = hook_enable();
	start_status = status;
	switch (status) {
	case UIOHOOK_SUCCESS: {
		// We no longer block, so we need to explicitly wait for the thread to die.
		/*#ifdef _WIN32
		WaitForSingleObject(hook_thread,  INFINITE);
		#else
		#if defined(__APPLE__) && defined(__MACH__)
		// NOTE Darwin requires that you start your own runloop from main.
		CFRunLoopRun();
		#endif

		pthread_join(hook_thread, NULL);
		#endif*/
	}
	break;

	// System level errors.
	case UIOHOOK_ERROR_OUT_OF_MEMORY:
		return "Failed to allocate memory";
		break;


	// X11 specific errors.
	case UIOHOOK_ERROR_X_OPEN_DISPLAY:
		return "Failed to open X11 display.";
		break;

	case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
		return "Unable to locate XRecord extension.";
		break;

	case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
		return "Unable to allocate XRecord range.";
		break;

	case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
		return "Unable to allocate XRecord context.";
		break;

	case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
		return "Failed to enable XRecord context.";
		break;


	// Windows specific errors.
	case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
		return "Failed to register low level windows hook.";
		break;

	// Darwin specific errors.
	case UIOHOOK_ERROR_AXAPI_DISABLED:
		return "Please enable access for assistive devices to OBS in the System Preferences.";
		break;

	case UIOHOOK_ERROR_CREATE_EVENT_PORT:
		return "Failed to create apple event port.";
		break;

	case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
		return "Failed to create apple run loop source.";
		break;

	case UIOHOOK_ERROR_GET_RUNLOOP:
		return "Failed to acquire apple run loop.";
		break;

	case UIOHOOK_ERROR_CREATE_OBSERVER:
		return "Failed to create apple run loop observer.";
		break;

	// Default error.
	case UIOHOOK_FAILURE:
	default:
		return "An unknown hook error occurred.";
		break;
	}

	return "";
}

int stop_logging(void)
{

	int status = true;
	if(start_status == UIOHOOK_SUCCESS) {
		status = hook_stop();
	}

#ifdef _WIN32
	// Create event handles for the thread hook.
	CloseHandle(hook_thread);
	CloseHandle(hook_running_mutex);
	CloseHandle(hook_control_mutex);
	CloseHandle(hook_control_cond);
#else
	pthread_mutex_destroy(&hook_running_mutex);
	pthread_mutex_destroy(&hook_control_mutex);
	pthread_cond_destroy(&hook_control_cond);
#endif

	// Close the file
	//fflush(event_file);
	fclose(event_file);

	return status;
}
