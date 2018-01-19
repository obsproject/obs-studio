/* See COPYING file for copyright and license details. */

#include <sndfile.h>
#include <string.h>
#include <stdlib.h>

#include "ebur128.h"


int main(int ac, const char* av[]) {
  SF_INFO file_info;
  SNDFILE* file;
  sf_count_t nr_frames_read;
  ebur128_state** sts = NULL;
  double* buffer;
  double loudness;
  int i;

  if (ac < 2) {
    fprintf(stderr, "usage: %s FILENAME...\n", av[0]);
    exit(1);
  }

  sts = malloc((size_t) (ac - 1) * sizeof(ebur128_state*));

  for (i = 0; i < ac - 1; ++i) {
    memset(&file_info, '\0', sizeof(file_info));
    file = sf_open(av[i + 1], SFM_READ, &file_info);

    sts[i] = ebur128_init((unsigned) file_info.channels,
                          (unsigned) file_info.samplerate,
                          EBUR128_MODE_I);

    /* example: set channel map (note: see ebur128.h for the default map) */
    if (file_info.channels == 5) {
      ebur128_set_channel(sts[i], 0, EBUR128_LEFT);
      ebur128_set_channel(sts[i], 1, EBUR128_RIGHT);
      ebur128_set_channel(sts[i], 2, EBUR128_CENTER);
      ebur128_set_channel(sts[i], 3, EBUR128_LEFT_SURROUND);
      ebur128_set_channel(sts[i], 4, EBUR128_RIGHT_SURROUND);
    }

    buffer = (double*) malloc(sts[i]->samplerate * sts[i]->channels * sizeof(double));
    while ((nr_frames_read = sf_readf_double(file, buffer,
                                             (sf_count_t) sts[i]->samplerate))) {
      ebur128_add_frames_double(sts[i], buffer, (size_t) nr_frames_read);
    }

    ebur128_loudness_global(sts[i], &loudness);
    fprintf(stderr, "%.2f LUFS, %s\n", loudness, av[i + 1]);


    free(buffer);
    buffer = NULL;

    if (sf_close(file)) {
      fprintf(stderr, "Could not close input file!\n");
    }
  }

  ebur128_loudness_global_multiple(sts, (size_t) ac - 1, &loudness);
  fprintf(stderr, "-----------\n%.2f LUFS\n", loudness);

  /* clean up */
  for (i = 0; i < ac - 1; ++i) {
    ebur128_destroy(&sts[i]);
  }
  free(sts);

  return 0;
}
