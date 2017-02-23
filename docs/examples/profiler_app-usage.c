#include "libobs/util/profiler.h"

static const char* PROFILER_PROGRAM "Program";
static const char* PROFILER_THREADSTART "ThreadStart";
static const char* PROFILER_THREADSTOP "ThreadStop";

static profiler_name_store_t *profiler_namestore;

struct thread_data {
	int id;
	pthread_t* thread;
	bool quit;
}

static void* thread_main(void* data) {
	thread_data* tdata = (thread_data*)data;
	
	// Create a new formatted name
	const char* name = profile_store_name(profiler_namestore, "Thread %ull", tdata->id);
	// Register the thread root
	profile_register_root(name, 0);
	// Begin profiling the thread root
	profile_start(name);
	
	while (!tdata->quit) {
		// Update the thread-local Profiler flag
		profile_reenable_thread();
		
		const char* work_part_name = "Work";		
		profile_start(work_part_name);
		// Do things
		profile_end(work_part_name);
	}
	// End profiling the thread root
	profile_end(name);
}

int main(int argc, const char* argv[]) {
	const unsigned int threadCount = 4;
	thread_data* threads[threadCount];
	
	// Enable the Profiler
	profiler_start();
	// Create a global name store (you should normally hide this behind an API)
	profiler_namestore = profiler_name_store_create();
	
	// Register the main program root
	profile_register_root(PROFILER_PROGRAM, 0);
	// Begin profiling the main program
	profile_start(PROFILER_PROGRAM);
	
	// Begin profiling thread startup routine
	profile_start(PROFILER_THREADSTART);
	for (unsigned int i = 0; i < threadCount; i++) {
		threads[i] = malloc(sizeof *threads[i]);
		pthread_create(threads[i]->thread, NULL, thread_main, threads[i]);
	}
	// End profiling thread startup routine
	profile_end(PROFILER_THREADSTART);
	
	// Do actual work in the main thread (signal handling, console input, etc)
	
	// Begin profiling thread shutdown routine
	profile_start(PROFILER_THREADSTOP);
	for (unsigned int i = 0; i < threadCount; i++) {
		threads[i]->quit = true;
		pthread_detach(threads[i]->thread);
		sleep(1); // Wait 1 second
		pthread_cancel(threads[i]->thread);
		free(threads[i]);
	}
	// End profiling thread shutdown routine
	profile_end(PROFILER_THREADSTOP);
	
	// End profiling the main program
	profile_end(PROFILER_PROGRAM);
	// Free the global name store
	profiler_name_store_free(profiler_namestore);
	// Disable the Profiler
	profiler_stop();
	// Disable & free the Profiler Memory
	profiler_free();
}
