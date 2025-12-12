extern "C" {
#include "media-playback.h"
#include "media.h"
#include <util/threading.h>
}
#include <iostream>

int main()
{
	std::cout << "[TestMedia] Initializing..." << std::endl;

	// Basic test of the media structure init/free
	// We don't actually load a file because that requires valid ffmpeg context and mocks
	// But we can verify symbols link and struct allocates.

	mp_media_t *media = (mp_media_t *)calloc(1, sizeof(mp_media_t));
	if (!media) {
		std::cerr << "Failed to allocate media" << std::endl;
		return 1;
	}

	// We can't easily call mp_media_init without a valid internal obs context or a lot more mocks
	// But linking this test proves the library built.

	// Testing our Semaphore implementation
	os_sem_t *sem = nullptr;
	if (os_sem_init(&sem, 0) != 0) {
		std::cerr << "Failed to init semaphore" << std::endl;
		return 1;
	}

	// Test post/wait
	os_sem_post(sem);
	os_sem_wait(sem);

	os_sem_destroy(sem);
	std::cout << "[TestMedia] Semaphore test passed." << std::endl;

	free(media);
	std::cout << "[TestMedia] Done." << std::endl;
	return 0;
}
