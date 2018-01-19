/* See COPYING file for copyright and license details. */

#include "ebur128.h"

#include <float.h>
#include <limits.h>
#include <math.h> /* You may have to define _USE_MATH_DEFINES if you use MSVC */
#include <stdio.h>
#include <stdlib.h>

/* This can be replaced by any BSD-like queue implementation. */
#include <sys/queue.h>

#define CHECK_ERROR(condition, errorcode, goto_point)                          \
  if ((condition)) {                                                           \
    errcode = (errorcode);                                                     \
    goto goto_point;                                                           \
  }

STAILQ_HEAD(ebur128_double_queue, ebur128_dq_entry);
struct ebur128_dq_entry {
  double z;
  STAILQ_ENTRY(ebur128_dq_entry) entries;
};

#define ALMOST_ZERO 0.000001

typedef struct {              /* Data structure for polyphase FIR interpolator */
  unsigned int factor;        /* Interpolation factor of the interpolator */
  unsigned int taps;          /* Taps (prefer odd to increase zero coeffs) */
  unsigned int channels;      /* Number of channels */
  unsigned int delay;         /* Size of delay buffer */
  struct {
    unsigned int count;       /* Number of coefficients in this subfilter */
    unsigned int* index;      /* Delay index of corresponding filter coeff */
    double* coeff;            /* List of subfilter coefficients */
  }* filter;                  /* List of subfilters (one for each factor) */
  float** z;                  /* List of delay buffers (one for each channel) */
  unsigned int zi;            /* Current delay buffer index */
} interpolator;

struct ebur128_state_internal {
  /** Filtered audio data (used as ring buffer). */
  double* audio_data;
  /** Size of audio_data array. */
  size_t audio_data_frames;
  /** Current index for audio_data. */
  size_t audio_data_index;
  /** How many frames are needed for a gating block. Will correspond to 400ms
   *  of audio at initialization, and 100ms after the first block (75% overlap
   *  as specified in the 2011 revision of BS1770). */
  unsigned long needed_frames;
  /** The channel map. Has as many elements as there are channels. */
  int* channel_map;
  /** How many samples fit in 100ms (rounded). */
  unsigned long samples_in_100ms;
  /** BS.1770 filter coefficients (nominator). */
  double b[5];
  /** BS.1770 filter coefficients (denominator). */
  double a[5];
  /** BS.1770 filter state. */
  double v[5][5];
  /** Linked list of block energies. */
  struct ebur128_double_queue block_list;
  unsigned long block_list_max;
  unsigned long block_list_size;
  /** Linked list of 3s-block energies, used to calculate LRA. */
  struct ebur128_double_queue short_term_block_list;
  unsigned long st_block_list_max;
  unsigned long st_block_list_size;
  int use_histogram;
  unsigned long *block_energy_histogram;
  unsigned long *short_term_block_energy_histogram;
  /** Keeps track of when a new short term block is needed. */
  size_t short_term_frame_counter;
  /** Maximum sample peak, one per channel */
  double* sample_peak;
  double* prev_sample_peak;
  /** Maximum true peak, one per channel */
  double* true_peak;
  double* prev_true_peak;
  interpolator* interp;
  float* resampler_buffer_input;
  size_t resampler_buffer_input_frames;
  float* resampler_buffer_output;
  size_t resampler_buffer_output_frames;
  /** The maximum window duration in ms. */
  unsigned long window;
  unsigned long history;
};

static double relative_gate = -10.0;

/* Those will be calculated when initializing the library */
static double relative_gate_factor;
static double minus_twenty_decibels;
static double histogram_energies[1000];
static double histogram_energy_boundaries[1001];

static interpolator* interp_create(unsigned int taps, unsigned int factor, unsigned int channels) {
  interpolator* interp = calloc(1, sizeof(interpolator));
  unsigned int j = 0;

  interp->taps = taps;
  interp->factor = factor;
  interp->channels = channels;
  interp->delay = (interp->taps + interp->factor - 1) / interp->factor;

  /* Initialize the filter memory
   * One subfilter per interpolation factor. */
  interp->filter = calloc(interp->factor, sizeof(*interp->filter));
  for (j = 0; j < interp->factor; j++) {
    interp->filter[j].index = calloc(interp->delay, sizeof(unsigned int));
    interp->filter[j].coeff = calloc(interp->delay, sizeof(double));
  }
  /* One delay buffer per channel. */
  interp->z = calloc(interp->channels, sizeof(float*));
  for (j = 0; j < interp->channels; j++) {
    interp->z[j] = calloc( interp->delay, sizeof(float) );
  }

  /* Calculate the filter coefficients */
  for (j = 0; j < interp->taps; j++) {
    /* Calculate sinc */
    double m = (double)j - (double)(interp->taps - 1) / 2.0;
    double c = 1.0;
    if (fabs(m) > ALMOST_ZERO) {
      c = sin(m * M_PI / interp->factor) / (m * M_PI / interp->factor);
    }
    /* Apply Hanning window */
    c *= 0.5 * (1 - cos(2 * M_PI * j / (interp->taps - 1)));

    if (fabs(c) > ALMOST_ZERO) { /* Ignore any zero coeffs. */
      /* Put the coefficient into the correct subfilter */
      unsigned int f = j % interp->factor;
      unsigned int t = interp->filter[f].count++;
      interp->filter[f].coeff[t] = c;
      interp->filter[f].index[t] = j / interp->factor;
    }
  }
  return interp;
}

static void interp_destroy(interpolator* interp) {
  unsigned int j = 0;
  if (!interp) {
    return;
  }
  for (j = 0; j < interp->factor; j++) {
    free(interp->filter[j].index);
    free(interp->filter[j].coeff);
  }
  free(interp->filter);
  for (j = 0; j < interp->channels; j++) {
    free(interp->z[j]);
  }
  free(interp->z);
  free(interp);
}

static size_t interp_process(interpolator* interp, size_t frames, float* in, float* out) {
  size_t frame = 0;
  unsigned int chan = 0;
  unsigned int f = 0;
  unsigned int t = 0;
  unsigned int out_stride = interp->channels * interp->factor;
  float* outp = 0;
  double acc = 0;
  double c = 0;

  for (frame = 0; frame < frames; frame++) {
    for (chan = 0; chan < interp->channels; chan++) {
      /* Add sample to delay buffer */
      interp->z[chan][interp->zi] = *in++;
      /* Apply coefficients */
      outp = out + chan;
      for (f = 0; f < interp->factor; f++) {
        acc = 0.0;
        for (t = 0; t < interp->filter[f].count; t++) {
          int i = (int)interp->zi - (int)interp->filter[f].index[t];
          if (i < 0) {
            i += interp->delay;
          }
          c = interp->filter[f].coeff[t];
          acc += interp->z[chan][i] * c;
        }
        *outp = (float)acc;
        outp += interp->channels;
      }
    }
    out += out_stride;
    interp->zi++;
    if (interp->zi == interp->delay) {
      interp->zi = 0;
    }
  }

  return frames * interp->factor;
}

static void ebur128_init_filter(ebur128_state* st) {
  int i, j;

  double f0 = 1681.974450955533;
  double G  =    3.999843853973347;
  double Q  =    0.7071752369554196;

  double K  = tan(M_PI * f0 / (double) st->samplerate);
  double Vh = pow(10.0, G / 20.0);
  double Vb = pow(Vh, 0.4996667741545416);

  double pb[3] = {0.0,  0.0, 0.0};
  double pa[3] = {1.0,  0.0, 0.0};
  double rb[3] = {1.0, -2.0, 1.0};
  double ra[3] = {1.0,  0.0, 0.0};

  double a0 =      1.0 + K / Q + K * K      ;
  pb[0] =     (Vh + Vb * K / Q + K * K) / a0;
  pb[1] =           2.0 * (K * K -  Vh) / a0;
  pb[2] =     (Vh - Vb * K / Q + K * K) / a0;
  pa[1] =           2.0 * (K * K - 1.0) / a0;
  pa[2] =         (1.0 - K / Q + K * K) / a0;

  /* fprintf(stderr, "%.14f %.14f %.14f %.14f %.14f\n",
                     b1[0], b1[1], b1[2], a1[1], a1[2]); */

  f0 = 38.13547087602444;
  Q  =  0.5003270373238773;
  K  = tan(M_PI * f0 / (double) st->samplerate);

  ra[1] =   2.0 * (K * K - 1.0) / (1.0 + K / Q + K * K);
  ra[2] = (1.0 - K / Q + K * K) / (1.0 + K / Q + K * K);

  /* fprintf(stderr, "%.14f %.14f\n", a2[1], a2[2]); */

  st->d->b[0] = pb[0] * rb[0];
  st->d->b[1] = pb[0] * rb[1] + pb[1] * rb[0];
  st->d->b[2] = pb[0] * rb[2] + pb[1] * rb[1] + pb[2] * rb[0];
  st->d->b[3] = pb[1] * rb[2] + pb[2] * rb[1];
  st->d->b[4] = pb[2] * rb[2];

  st->d->a[0] = pa[0] * ra[0];
  st->d->a[1] = pa[0] * ra[1] + pa[1] * ra[0];
  st->d->a[2] = pa[0] * ra[2] + pa[1] * ra[1] + pa[2] * ra[0];
  st->d->a[3] = pa[1] * ra[2] + pa[2] * ra[1];
  st->d->a[4] = pa[2] * ra[2];

  for (i = 0; i < 5; ++i) {
    for (j = 0; j < 5; ++j) {
      st->d->v[i][j] = 0.0;
    }
  }
}

static int ebur128_init_channel_map(ebur128_state* st) {
  size_t i;
  st->d->channel_map = (int*) malloc(st->channels * sizeof(int));
  if (!st->d->channel_map) {
    return EBUR128_ERROR_NOMEM;
  }
  if (st->channels == 4) {
    st->d->channel_map[0] = EBUR128_LEFT;
    st->d->channel_map[1] = EBUR128_RIGHT;
    st->d->channel_map[2] = EBUR128_LEFT_SURROUND;
    st->d->channel_map[3] = EBUR128_RIGHT_SURROUND;
  } else if (st->channels == 5) {
    st->d->channel_map[0] = EBUR128_LEFT;
    st->d->channel_map[1] = EBUR128_RIGHT;
    st->d->channel_map[2] = EBUR128_CENTER;
    st->d->channel_map[3] = EBUR128_LEFT_SURROUND;
    st->d->channel_map[4] = EBUR128_RIGHT_SURROUND;
  } else {
    for (i = 0; i < st->channels; ++i) {
      switch (i) {
        case 0:  st->d->channel_map[i] = EBUR128_LEFT;           break;
        case 1:  st->d->channel_map[i] = EBUR128_RIGHT;          break;
        case 2:  st->d->channel_map[i] = EBUR128_CENTER;         break;
        case 3:  st->d->channel_map[i] = EBUR128_UNUSED;         break;
        case 4:  st->d->channel_map[i] = EBUR128_LEFT_SURROUND;  break;
        case 5:  st->d->channel_map[i] = EBUR128_RIGHT_SURROUND; break;
        default: st->d->channel_map[i] = EBUR128_UNUSED;         break;
      }
    }
  }
  return EBUR128_SUCCESS;
}

static int ebur128_init_resampler(ebur128_state* st) {
  int errcode = EBUR128_SUCCESS;

  if (st->samplerate < 96000) {
    st->d->interp = interp_create(49, 4, st->channels);
    CHECK_ERROR(!st->d->interp, EBUR128_ERROR_NOMEM, exit)
  } else if (st->samplerate < 192000) {
    st->d->interp = interp_create(49, 2, st->channels);
    CHECK_ERROR(!st->d->interp, EBUR128_ERROR_NOMEM, exit)
  } else {
    st->d->resampler_buffer_input = NULL;
    st->d->resampler_buffer_output = NULL;
    st->d->interp = NULL;
    goto exit;
  }

  st->d->resampler_buffer_input_frames = st->d->samples_in_100ms * 4;
  st->d->resampler_buffer_input = malloc(st->d->resampler_buffer_input_frames *
                                      st->channels *
                                      sizeof(float));
  CHECK_ERROR(!st->d->resampler_buffer_input, EBUR128_ERROR_NOMEM, free_interp)

  st->d->resampler_buffer_output_frames =
                                    st->d->resampler_buffer_input_frames *
                                    st->d->interp->factor;
  st->d->resampler_buffer_output = malloc
                                      (st->d->resampler_buffer_output_frames *
                                       st->channels *
                                       sizeof(float));
  CHECK_ERROR(!st->d->resampler_buffer_output, EBUR128_ERROR_NOMEM, free_input)

  return errcode;

free_interp:
  interp_destroy(st->d->interp);
  st->d->interp = NULL;
free_input:
  free(st->d->resampler_buffer_input);
  st->d->resampler_buffer_input = NULL;
exit:
  return errcode;
}

static void ebur128_destroy_resampler(ebur128_state* st) {
  free(st->d->resampler_buffer_input);
  st->d->resampler_buffer_input = NULL;
  free(st->d->resampler_buffer_output);
  st->d->resampler_buffer_output = NULL;
  interp_destroy(st->d->interp);
  st->d->interp = NULL;
}

void ebur128_get_version(int* major, int* minor, int* patch) {
  *major = EBUR128_VERSION_MAJOR;
  *minor = EBUR128_VERSION_MINOR;
  *patch = EBUR128_VERSION_PATCH;
}

ebur128_state* ebur128_init(unsigned int channels,
                            unsigned long samplerate,
                            int mode) {
  int result;
  int errcode;
  ebur128_state* st;
  unsigned int i;
  size_t j;

  if (channels == 0 || samplerate < 5) {
    return NULL;
  }

  st = (ebur128_state*) malloc(sizeof(ebur128_state));
  CHECK_ERROR(!st, 0, exit)
  st->d = (struct ebur128_state_internal*)
          malloc(sizeof(struct ebur128_state_internal));
  CHECK_ERROR(!st->d, 0, free_state)
  st->channels = channels;
  errcode = ebur128_init_channel_map(st);
  CHECK_ERROR(errcode, 0, free_internal)

  st->d->sample_peak = (double*) malloc(channels * sizeof(double));
  CHECK_ERROR(!st->d->sample_peak, 0, free_channel_map)
  st->d->prev_sample_peak = (double*) malloc(channels * sizeof(double));
  CHECK_ERROR(!st->d->prev_sample_peak, 0, free_sample_peak)
  st->d->true_peak = (double*) malloc(channels * sizeof(double));
  CHECK_ERROR(!st->d->true_peak, 0, free_prev_sample_peak)
  st->d->prev_true_peak = (double*) malloc(channels * sizeof(double));
  CHECK_ERROR(!st->d->prev_true_peak, 0, free_true_peak)
  for (i = 0; i < channels; ++i) {
    st->d->sample_peak[i] = 0.0;
    st->d->prev_sample_peak[i] = 0.0;
    st->d->true_peak[i] = 0.0;
    st->d->prev_true_peak[i] = 0.0;
  }

  st->d->use_histogram = mode & EBUR128_MODE_HISTOGRAM ? 1 : 0;
  st->d->history = ULONG_MAX;
  st->samplerate = samplerate;
  st->d->samples_in_100ms = (st->samplerate + 5) / 10;
  st->mode = mode;
  if ((mode & EBUR128_MODE_S) == EBUR128_MODE_S) {
    st->d->window = 3000;
  } else if ((mode & EBUR128_MODE_M) == EBUR128_MODE_M) {
    st->d->window = 400;
  } else {
    goto free_prev_true_peak;
  }
  st->d->audio_data_frames = st->samplerate * st->d->window / 1000;
  if (st->d->audio_data_frames % st->d->samples_in_100ms) {
    /* round up to multiple of samples_in_100ms */
    st->d->audio_data_frames = st->d->audio_data_frames
                             + st->d->samples_in_100ms
                             - (st->d->audio_data_frames % st->d->samples_in_100ms);
  }
  st->d->audio_data = (double*) malloc(st->d->audio_data_frames *
                                       st->channels *
                                       sizeof(double));
  CHECK_ERROR(!st->d->audio_data, 0, free_true_peak)
  for (j = 0; j < st->d->audio_data_frames * st->channels; ++j) {
    st->d->audio_data[j] = 0.0;
  }

  ebur128_init_filter(st);

  if (st->d->use_histogram) {
    st->d->block_energy_histogram = malloc(1000 * sizeof(unsigned long));
    CHECK_ERROR(!st->d->block_energy_histogram, 0, free_audio_data)
    for (i = 0; i < 1000; ++i) {
      st->d->block_energy_histogram[i] = 0;
    }
  } else {
    st->d->block_energy_histogram = NULL;
  }
  if (st->d->use_histogram) {
    st->d->short_term_block_energy_histogram = malloc(1000 * sizeof(unsigned long));
    CHECK_ERROR(!st->d->short_term_block_energy_histogram, 0, free_block_energy_histogram)
    for (i = 0; i < 1000; ++i) {
      st->d->short_term_block_energy_histogram[i] = 0;
    }
  } else {
    st->d->short_term_block_energy_histogram = NULL;
  }
  STAILQ_INIT(&st->d->block_list);
  st->d->block_list_size = 0;
  st->d->block_list_max = st->d->history / 100;
  STAILQ_INIT(&st->d->short_term_block_list);
  st->d->st_block_list_size = 0;
  st->d->st_block_list_max = st->d->history / 3000;
  st->d->short_term_frame_counter = 0;

  result = ebur128_init_resampler(st);
  CHECK_ERROR(result, 0, free_short_term_block_energy_histogram)

  /* the first block needs 400ms of audio data */
  st->d->needed_frames = st->d->samples_in_100ms * 4;
  /* start at the beginning of the buffer */
  st->d->audio_data_index = 0;

  /* initialize static constants */
  relative_gate_factor = pow(10.0, relative_gate / 10.0);
  minus_twenty_decibels = pow(10.0, -20.0 / 10.0);
  histogram_energy_boundaries[0] = pow(10.0, (-70.0 + 0.691) / 10.0);
  if (st->d->use_histogram) {
    for (i = 0; i < 1000; ++i) {
      histogram_energies[i] = pow(10.0, ((double) i / 10.0 - 69.95 + 0.691) / 10.0);
    }
    for (i = 1; i < 1001; ++i) {
      histogram_energy_boundaries[i] = pow(10.0, ((double) i / 10.0 - 70.0 + 0.691) / 10.0);
    }
  }

  return st;

free_short_term_block_energy_histogram:
  free(st->d->short_term_block_energy_histogram);
free_block_energy_histogram:
  free(st->d->block_energy_histogram);
free_audio_data:
  free(st->d->audio_data);
free_prev_true_peak:
  free(st->d->prev_true_peak);
free_true_peak:
  free(st->d->true_peak);
free_prev_sample_peak:
  free(st->d->prev_sample_peak);
free_sample_peak:
  free(st->d->sample_peak);
free_channel_map:
  free(st->d->channel_map);
free_internal:
  free(st->d);
free_state:
  free(st);
exit:
  return NULL;
}

void ebur128_destroy(ebur128_state** st) {
  struct ebur128_dq_entry* entry;
  free((*st)->d->block_energy_histogram);
  free((*st)->d->short_term_block_energy_histogram);
  free((*st)->d->audio_data);
  free((*st)->d->channel_map);
  free((*st)->d->sample_peak);
  free((*st)->d->prev_sample_peak);
  free((*st)->d->true_peak);
  free((*st)->d->prev_true_peak);
  while (!STAILQ_EMPTY(&(*st)->d->block_list)) {
    entry = STAILQ_FIRST(&(*st)->d->block_list);
    STAILQ_REMOVE_HEAD(&(*st)->d->block_list, entries);
    free(entry);
  }
  while (!STAILQ_EMPTY(&(*st)->d->short_term_block_list)) {
    entry = STAILQ_FIRST(&(*st)->d->short_term_block_list);
    STAILQ_REMOVE_HEAD(&(*st)->d->short_term_block_list, entries);
    free(entry);
  }
  ebur128_destroy_resampler(*st);
  free((*st)->d);
  free(*st);
  *st = NULL;
}

static void ebur128_check_true_peak(ebur128_state* st, size_t frames) {
  size_t c, i, frames_out;

  frames_out = interp_process(st->d->interp, frames,
                 st->d->resampler_buffer_input,
                 st->d->resampler_buffer_output);

  for (i = 0; i < frames_out; ++i) {
    for (c = 0; c < st->channels; ++c) {
      float val = st->d->resampler_buffer_output[i * st->channels + c];

      if (val > st->d->prev_true_peak[c]) {
        st->d->prev_true_peak[c] = val;
      } else if (-val > st->d->prev_true_peak[c]) {
        st->d->prev_true_peak[c] = -val;
      }
    }
  }
}

#ifdef __SSE2_MATH__
#include <xmmintrin.h>
#define TURN_ON_FTZ \
        unsigned int mxcsr = _mm_getcsr(); \
        _mm_setcsr(mxcsr | _MM_FLUSH_ZERO_ON);
#define TURN_OFF_FTZ _mm_setcsr(mxcsr);
#define FLUSH_MANUALLY
#else
#warning "manual FTZ is being used, please enable SSE2 (-msse2 -mfpmath=sse)"
#define TURN_ON_FTZ
#define TURN_OFF_FTZ
#define FLUSH_MANUALLY \
    st->d->v[ci][4] = fabs(st->d->v[ci][4]) < DBL_MIN ? 0.0 : st->d->v[ci][4]; \
    st->d->v[ci][3] = fabs(st->d->v[ci][3]) < DBL_MIN ? 0.0 : st->d->v[ci][3]; \
    st->d->v[ci][2] = fabs(st->d->v[ci][2]) < DBL_MIN ? 0.0 : st->d->v[ci][2]; \
    st->d->v[ci][1] = fabs(st->d->v[ci][1]) < DBL_MIN ? 0.0 : st->d->v[ci][1];
#endif

#define EBUR128_FILTER(type, min_scale, max_scale)                             \
static void ebur128_filter_##type(ebur128_state* st, const type* src,          \
                                  size_t frames) {                             \
  static double scaling_factor =                                               \
                 -((double) (min_scale)) > (double) (max_scale) ?              \
                 -((double) (min_scale)) : (double) (max_scale);               \
  double* audio_data = st->d->audio_data + st->d->audio_data_index;            \
  size_t i, c;                                                                 \
                                                                               \
  TURN_ON_FTZ                                                                  \
                                                                               \
  if ((st->mode & EBUR128_MODE_SAMPLE_PEAK) == EBUR128_MODE_SAMPLE_PEAK) {     \
    for (c = 0; c < st->channels; ++c) {                                       \
      double max = 0.0;                                                        \
      for (i = 0; i < frames; ++i) {                                           \
        if (src[i * st->channels + c] > max) {                                 \
          max =        src[i * st->channels + c];                              \
        } else if (-src[i * st->channels + c] > max) {                         \
          max = -1.0 * src[i * st->channels + c];                              \
        }                                                                      \
      }                                                                        \
      max /= scaling_factor;                                                   \
      if (max > st->d->prev_sample_peak[c]) st->d->prev_sample_peak[c] = max;  \
    }                                                                          \
  }                                                                            \
  if ((st->mode & EBUR128_MODE_TRUE_PEAK) == EBUR128_MODE_TRUE_PEAK &&         \
      st->d->interp) {                                                         \
    for (c = 0; c < st->channels; ++c) {                                       \
      for (i = 0; i < frames; ++i) {                                           \
        st->d->resampler_buffer_input[i * st->channels + c] =                  \
                      (float) (src[i * st->channels + c] / scaling_factor);    \
      }                                                                        \
    }                                                                          \
    ebur128_check_true_peak(st, frames);                                       \
  }                                                                            \
  for (c = 0; c < st->channels; ++c) {                                         \
    int ci = st->d->channel_map[c] - 1;                                        \
    if (ci < 0) continue;                                                      \
    else if (ci == EBUR128_DUAL_MONO - 1) ci = 0; /*dual mono */               \
    for (i = 0; i < frames; ++i) {                                             \
      st->d->v[ci][0] = (double) (src[i * st->channels + c] / scaling_factor)  \
                   - st->d->a[1] * st->d->v[ci][1]                             \
                   - st->d->a[2] * st->d->v[ci][2]                             \
                   - st->d->a[3] * st->d->v[ci][3]                             \
                   - st->d->a[4] * st->d->v[ci][4];                            \
      audio_data[i * st->channels + c] =                                       \
                     st->d->b[0] * st->d->v[ci][0]                             \
                   + st->d->b[1] * st->d->v[ci][1]                             \
                   + st->d->b[2] * st->d->v[ci][2]                             \
                   + st->d->b[3] * st->d->v[ci][3]                             \
                   + st->d->b[4] * st->d->v[ci][4];                            \
      st->d->v[ci][4] = st->d->v[ci][3];                                       \
      st->d->v[ci][3] = st->d->v[ci][2];                                       \
      st->d->v[ci][2] = st->d->v[ci][1];                                       \
      st->d->v[ci][1] = st->d->v[ci][0];                                       \
    }                                                                          \
    FLUSH_MANUALLY                                                             \
  }                                                                            \
  TURN_OFF_FTZ                                                                 \
}
EBUR128_FILTER(short, SHRT_MIN, SHRT_MAX)
EBUR128_FILTER(int, INT_MIN, INT_MAX)
EBUR128_FILTER(float, -1.0f, 1.0f)
EBUR128_FILTER(double, -1.0, 1.0)

static double ebur128_energy_to_loudness(double energy) {
  return 10 * (log(energy) / log(10.0)) - 0.691;
}

static size_t find_histogram_index(double energy) {
  size_t index_min = 0;
  size_t index_max = 1000;
  size_t index_mid;

  do {
    index_mid = (index_min + index_max) / 2;
    if (energy >= histogram_energy_boundaries[index_mid]) {
      index_min = index_mid;
    } else {
      index_max = index_mid;
    }
  } while (index_max - index_min != 1);

  return index_min;
}

static int ebur128_calc_gating_block(ebur128_state* st, size_t frames_per_block,
                                     double* optional_output) {
  size_t i, c;
  double sum = 0.0;
  double channel_sum;
  for (c = 0; c < st->channels; ++c) {
    if (st->d->channel_map[c] == EBUR128_UNUSED) {
      continue;
    }
    channel_sum = 0.0;
    if (st->d->audio_data_index < frames_per_block * st->channels) {
      for (i = 0; i < st->d->audio_data_index / st->channels; ++i) {
        channel_sum += st->d->audio_data[i * st->channels + c] *
                       st->d->audio_data[i * st->channels + c];
      }
      for (i = st->d->audio_data_frames -
              (frames_per_block -
               st->d->audio_data_index / st->channels);
           i < st->d->audio_data_frames; ++i) {
        channel_sum += st->d->audio_data[i * st->channels + c] *
                       st->d->audio_data[i * st->channels + c];
      }
    } else {
      for (i = st->d->audio_data_index / st->channels - frames_per_block;
           i < st->d->audio_data_index / st->channels;
           ++i) {
        channel_sum += st->d->audio_data[i * st->channels + c] *
                       st->d->audio_data[i * st->channels + c];
      }
    }
    if (st->d->channel_map[c] == EBUR128_Mp110 ||
        st->d->channel_map[c] == EBUR128_Mm110 ||
        st->d->channel_map[c] == EBUR128_Mp060 ||
        st->d->channel_map[c] == EBUR128_Mm060 ||
        st->d->channel_map[c] == EBUR128_Mp090 ||
        st->d->channel_map[c] == EBUR128_Mm090) {
      channel_sum *= 1.41;
    } else if (st->d->channel_map[c] == EBUR128_DUAL_MONO) {
      channel_sum *= 2.0;
    }
    sum += channel_sum;
  }
  sum /= (double) frames_per_block;
  if (optional_output) {
    *optional_output = sum;
    return EBUR128_SUCCESS;
  } else if (sum >= histogram_energy_boundaries[0]) {
    if (st->d->use_histogram) {
      ++st->d->block_energy_histogram[find_histogram_index(sum)];
    } else {
      struct ebur128_dq_entry* block;
      if (st->d->block_list_size == st->d->block_list_max) {
        block = STAILQ_FIRST(&st->d->block_list);
        STAILQ_REMOVE_HEAD(&st->d->block_list, entries);
      } else {
        block = (struct ebur128_dq_entry*) malloc(sizeof(struct ebur128_dq_entry));
        if (!block) {
          return EBUR128_ERROR_NOMEM;
        }
        st->d->block_list_size++;
      }
      block->z = sum;
      STAILQ_INSERT_TAIL(&st->d->block_list, block, entries);
    }
    return EBUR128_SUCCESS;
  } else {
    return EBUR128_SUCCESS;
  }
}

int ebur128_set_channel(ebur128_state* st,
                        unsigned int channel_number,
                        int value) {
  if (channel_number >= st->channels) {
    return 1;
  }
  if (value == EBUR128_DUAL_MONO &&
      (st->channels != 1 || channel_number != 0)) {
    fprintf(stderr, "EBUR128_DUAL_MONO only works with mono files!\n");
    return 1;
  }
  st->d->channel_map[channel_number] = value;
  return 0;
}

int ebur128_change_parameters(ebur128_state* st,
                              unsigned int channels,
                              unsigned long samplerate) {
  int errcode = EBUR128_SUCCESS;
  size_t j;

  if (channels == 0 || samplerate < 5) {
    return EBUR128_ERROR_NOMEM;
  }

  if (channels == st->channels &&
      samplerate == st->samplerate) {
    return EBUR128_ERROR_NO_CHANGE;
  }

  free(st->d->audio_data);
  st->d->audio_data = NULL;

  if (channels != st->channels) {
    unsigned int i;

    free(st->d->channel_map); st->d->channel_map = NULL;
    free(st->d->sample_peak); st->d->sample_peak = NULL;
    free(st->d->prev_sample_peak); st->d->prev_sample_peak = NULL;
    free(st->d->true_peak);   st->d->true_peak = NULL;
    free(st->d->prev_true_peak); st->d->prev_true_peak = NULL;
    st->channels = channels;

    errcode = ebur128_init_channel_map(st);
    CHECK_ERROR(errcode, EBUR128_ERROR_NOMEM, exit)

    st->d->sample_peak = (double*) malloc(channels * sizeof(double));
    CHECK_ERROR(!st->d->sample_peak, EBUR128_ERROR_NOMEM, exit)
    st->d->prev_sample_peak = (double*) malloc(channels * sizeof(double));
    CHECK_ERROR(!st->d->prev_sample_peak, EBUR128_ERROR_NOMEM, exit)
    st->d->true_peak = (double*) malloc(channels * sizeof(double));
    CHECK_ERROR(!st->d->true_peak, EBUR128_ERROR_NOMEM, exit)
    st->d->prev_true_peak = (double*) malloc(channels * sizeof(double));
    CHECK_ERROR(!st->d->prev_true_peak, EBUR128_ERROR_NOMEM, exit)
    for (i = 0; i < channels; ++i) {
      st->d->sample_peak[i] = 0.0;
      st->d->prev_sample_peak[i] = 0.0;
      st->d->true_peak[i] = 0.0;
      st->d->prev_true_peak[i] = 0.0;
    }
  }
  if (samplerate != st->samplerate) {
    st->samplerate = samplerate;
    st->d->samples_in_100ms = (st->samplerate + 5) / 10;
    ebur128_init_filter(st);
  }
  st->d->audio_data_frames = st->samplerate * st->d->window / 1000;
  if (st->d->audio_data_frames % st->d->samples_in_100ms) {
    /* round up to multiple of samples_in_100ms */
    st->d->audio_data_frames = st->d->audio_data_frames
                             + st->d->samples_in_100ms
                             - (st->d->audio_data_frames % st->d->samples_in_100ms);
  }
  st->d->audio_data = (double*) malloc(st->d->audio_data_frames *
                                       st->channels *
                                       sizeof(double));
  CHECK_ERROR(!st->d->audio_data, EBUR128_ERROR_NOMEM, exit)
  for (j = 0; j < st->d->audio_data_frames * st->channels; ++j) {
    st->d->audio_data[j] = 0.0;
  }

  ebur128_destroy_resampler(st);
  errcode = ebur128_init_resampler(st);
  CHECK_ERROR(errcode, EBUR128_ERROR_NOMEM, exit)

  /* the first block needs 400ms of audio data */
  st->d->needed_frames = st->d->samples_in_100ms * 4;
  /* start at the beginning of the buffer */
  st->d->audio_data_index = 0;
  /* reset short term frame counter */
  st->d->short_term_frame_counter = 0;

exit:
  return errcode;
}

int ebur128_set_max_window(ebur128_state* st, unsigned long window)
{
  int errcode = EBUR128_SUCCESS;
  size_t j;

  if ((st->mode & EBUR128_MODE_S) == EBUR128_MODE_S && window < 3000) {
    window = 3000;
  } else if ((st->mode & EBUR128_MODE_M) == EBUR128_MODE_M && window < 400) {
    window = 400;
  }
  if (window == st->d->window) {
    return EBUR128_ERROR_NO_CHANGE;
  }

  st->d->window = window;
  free(st->d->audio_data);
  st->d->audio_data = NULL;
  st->d->audio_data_frames = st->samplerate * st->d->window / 1000;
  if (st->d->audio_data_frames % st->d->samples_in_100ms) {
    /* round up to multiple of samples_in_100ms */
    st->d->audio_data_frames = st->d->audio_data_frames
                             + st->d->samples_in_100ms
                             - (st->d->audio_data_frames % st->d->samples_in_100ms);
  }
  st->d->audio_data = (double*) malloc(st->d->audio_data_frames *
                                       st->channels *
                                       sizeof(double));
  CHECK_ERROR(!st->d->audio_data, EBUR128_ERROR_NOMEM, exit)
  for (j = 0; j < st->d->audio_data_frames * st->channels; ++j) {
    st->d->audio_data[j] = 0.0;
  }

  /* the first block needs 400ms of audio data */
  st->d->needed_frames = st->d->samples_in_100ms * 4;
  /* start at the beginning of the buffer */
  st->d->audio_data_index = 0;
  /* reset short term frame counter */
  st->d->short_term_frame_counter = 0;

exit:
  return errcode;
}

int ebur128_set_max_history(ebur128_state* st, unsigned long history)
{
  if ((st->mode & EBUR128_MODE_LRA) == EBUR128_MODE_LRA && history < 3000) {
    history = 3000;
  } else if ((st->mode & EBUR128_MODE_M) == EBUR128_MODE_M && history < 400) {
    history = 400;
  }
  if (history == st->d->history) {
    return EBUR128_ERROR_NO_CHANGE;
  }
  st->d->history = history;
  st->d->block_list_max = st->d->history / 100;
  st->d->st_block_list_max = st->d->history / 3000;
  while (st->d->block_list_size > st->d->block_list_max) {
    struct ebur128_dq_entry* block = STAILQ_FIRST(&st->d->block_list);
    STAILQ_REMOVE_HEAD(&st->d->block_list, entries);
    free(block);
    st->d->block_list_size--;
  }
  while (st->d->st_block_list_size > st->d->st_block_list_max) {
    struct ebur128_dq_entry* block = STAILQ_FIRST(&st->d->short_term_block_list);
    STAILQ_REMOVE_HEAD(&st->d->short_term_block_list, entries);
    free(block);
    st->d->st_block_list_size--;
  }
  return EBUR128_SUCCESS;
}

static int ebur128_energy_shortterm(ebur128_state* st, double* out);
#define EBUR128_ADD_FRAMES(type)                                               \
int ebur128_add_frames_##type(ebur128_state* st,                               \
                              const type* src, size_t frames) {                \
  size_t src_index = 0;                                                        \
  unsigned int c = 0;                                                          \
  for (c = 0; c < st->channels; c++) {                                         \
    st->d->prev_sample_peak[c] = 0.0;                                          \
    st->d->prev_true_peak[c] = 0.0;                                            \
  }                                                                            \
  while (frames > 0) {                                                         \
    if (frames >= st->d->needed_frames) {                                      \
      ebur128_filter_##type(st, src + src_index, st->d->needed_frames);        \
      src_index += st->d->needed_frames * st->channels;                        \
      frames -= st->d->needed_frames;                                          \
      st->d->audio_data_index += st->d->needed_frames * st->channels;          \
      /* calculate the new gating block */                                     \
      if ((st->mode & EBUR128_MODE_I) == EBUR128_MODE_I) {                     \
        if (ebur128_calc_gating_block(st, st->d->samples_in_100ms * 4, NULL)) {\
          return EBUR128_ERROR_NOMEM;                                          \
        }                                                                      \
      }                                                                        \
      if ((st->mode & EBUR128_MODE_LRA) == EBUR128_MODE_LRA) {                 \
        st->d->short_term_frame_counter += st->d->needed_frames;               \
        if (st->d->short_term_frame_counter == st->d->samples_in_100ms * 30) { \
          struct ebur128_dq_entry* block;                                      \
          double st_energy;                                                    \
          if (ebur128_energy_shortterm(st, &st_energy) == EBUR128_SUCCESS &&   \
                  st_energy >= histogram_energy_boundaries[0]) {               \
            if (st->d->use_histogram) {                                        \
              ++st->d->short_term_block_energy_histogram[                      \
                                              find_histogram_index(st_energy)];\
            } else {                                                           \
              if (st->d->st_block_list_size == st->d->st_block_list_max) {     \
                block = STAILQ_FIRST(&st->d->short_term_block_list);           \
                STAILQ_REMOVE_HEAD(&st->d->short_term_block_list, entries);    \
              } else {                                                         \
                block = (struct ebur128_dq_entry*)                             \
                        malloc(sizeof(struct ebur128_dq_entry));               \
                if (!block) return EBUR128_ERROR_NOMEM;                        \
                st->d->st_block_list_size++;                                   \
              }                                                                \
              block->z = st_energy;                                            \
              STAILQ_INSERT_TAIL(&st->d->short_term_block_list,                \
                                 block, entries);                              \
            }                                                                  \
          }                                                                    \
          st->d->short_term_frame_counter = st->d->samples_in_100ms * 20;      \
        }                                                                      \
      }                                                                        \
      /* 100ms are needed for all blocks besides the first one */              \
      st->d->needed_frames = st->d->samples_in_100ms;                          \
      /* reset audio_data_index when buffer full */                            \
      if (st->d->audio_data_index == st->d->audio_data_frames * st->channels) {\
        st->d->audio_data_index = 0;                                           \
      }                                                                        \
    } else {                                                                   \
      ebur128_filter_##type(st, src + src_index, frames);                      \
      st->d->audio_data_index += frames * st->channels;                        \
      if ((st->mode & EBUR128_MODE_LRA) == EBUR128_MODE_LRA) {                 \
        st->d->short_term_frame_counter += frames;                             \
      }                                                                        \
      st->d->needed_frames -= frames;                                          \
      frames = 0;                                                              \
    }                                                                          \
  }                                                                            \
  for (c = 0; c < st->channels; c++) {                                         \
    if (st->d->prev_sample_peak[c] > st->d->sample_peak[c]) {                  \
      st->d->sample_peak[c] = st->d->prev_sample_peak[c];                      \
    }                                                                          \
    if (st->d->prev_true_peak[c] > st->d->true_peak[c]) {                      \
      st->d->true_peak[c] = st->d->prev_true_peak[c];                          \
    }                                                                          \
  }                                                                            \
  return EBUR128_SUCCESS;                                                      \
}
EBUR128_ADD_FRAMES(short)
EBUR128_ADD_FRAMES(int)
EBUR128_ADD_FRAMES(float)
EBUR128_ADD_FRAMES(double)

static int ebur128_calc_relative_threshold(ebur128_state* st,
                                           size_t* above_thresh_counter,
                                           double* relative_threshold) {
  struct ebur128_dq_entry* it;
  size_t i;
  *relative_threshold = 0.0;
  *above_thresh_counter = 0;

  if (st->d->use_histogram) {
    for (i = 0; i < 1000; ++i) {
      *relative_threshold += st->d->block_energy_histogram[i] *
                            histogram_energies[i];
      *above_thresh_counter += st->d->block_energy_histogram[i];
    }
  } else {
    STAILQ_FOREACH(it, &st->d->block_list, entries) {
      ++*above_thresh_counter;
      *relative_threshold += it->z;
    }
  }

  if (*above_thresh_counter != 0) {
    *relative_threshold /= (double) *above_thresh_counter;
    *relative_threshold *= relative_gate_factor;
  }

  return EBUR128_SUCCESS;
}

static int ebur128_gated_loudness(ebur128_state** sts, size_t size,
                                  double* out) {
  struct ebur128_dq_entry* it;
  double gated_loudness = 0.0;
  double relative_threshold = 0.0;
  size_t above_thresh_counter = 0;
  size_t i, j, start_index;

  for (i = 0; i < size; i++) {
    if (sts[i] && (sts[i]->mode & EBUR128_MODE_I) != EBUR128_MODE_I) {
      return EBUR128_ERROR_INVALID_MODE;
    }
  }

  for (i = 0; i < size; i++) {
    if (!sts[i]) {
      continue;
    }
    ebur128_calc_relative_threshold(sts[i], &above_thresh_counter, &relative_threshold);
  }
  if (!above_thresh_counter) {
    *out = -HUGE_VAL;
    return EBUR128_SUCCESS;
  }

  above_thresh_counter = 0;
  if (relative_threshold < histogram_energy_boundaries[0]) {
    start_index = 0;
  } else {
    start_index = find_histogram_index(relative_threshold);
    if (relative_threshold > histogram_energies[start_index]) {
      ++start_index;
    }
  }
  for (i = 0; i < size; i++) {
    if (!sts[i]) {
      continue;
    }
    if (sts[i]->d->use_histogram) {
      for (j = start_index; j < 1000; ++j) {
        gated_loudness += sts[i]->d->block_energy_histogram[j] *
                          histogram_energies[j];
        above_thresh_counter += sts[i]->d->block_energy_histogram[j];
      }
    } else {
      STAILQ_FOREACH(it, &sts[i]->d->block_list, entries) {
        if (it->z >= relative_threshold) {
          ++above_thresh_counter;
          gated_loudness += it->z;
        }
      }
    }
  }
  if (!above_thresh_counter) {
    *out = -HUGE_VAL;
    return EBUR128_SUCCESS;
  }
  gated_loudness /= (double) above_thresh_counter;
  *out = ebur128_energy_to_loudness(gated_loudness);
  return EBUR128_SUCCESS;
}

int ebur128_relative_threshold(ebur128_state* st, double* out) {
  double relative_threshold;
  size_t above_thresh_counter;

  if ((st->mode & EBUR128_MODE_I) != EBUR128_MODE_I) {
    return EBUR128_ERROR_INVALID_MODE;
  }

  ebur128_calc_relative_threshold(st, &above_thresh_counter, &relative_threshold);

  if (!above_thresh_counter) {
      *out = -70.0;
      return EBUR128_SUCCESS;
  }

  *out = ebur128_energy_to_loudness(relative_threshold);
  return EBUR128_SUCCESS;
}

int ebur128_loudness_global(ebur128_state* st, double* out) {
  return ebur128_gated_loudness(&st, 1, out);
}

int ebur128_loudness_global_multiple(ebur128_state** sts, size_t size,
                                     double* out) {
  return ebur128_gated_loudness(sts, size, out);
}

static int ebur128_energy_in_interval(ebur128_state* st,
                                      size_t interval_frames,
                                      double* out) {
  if (interval_frames > st->d->audio_data_frames) {
    return EBUR128_ERROR_INVALID_MODE;
  }
  ebur128_calc_gating_block(st, interval_frames, out);
  return EBUR128_SUCCESS;
}

static int ebur128_energy_shortterm(ebur128_state* st, double* out) {
  return ebur128_energy_in_interval(st, st->d->samples_in_100ms * 30, out);
}

int ebur128_loudness_momentary(ebur128_state* st, double* out) {
  double energy;
  int error = ebur128_energy_in_interval(st, st->d->samples_in_100ms * 4,
                                         &energy);
  if (error) {
    return error;
  } else if (energy <= 0.0) {
    *out = -HUGE_VAL;
    return EBUR128_SUCCESS;
  }
  *out = ebur128_energy_to_loudness(energy);
  return EBUR128_SUCCESS;
}

int ebur128_loudness_shortterm(ebur128_state* st, double* out) {
  double energy;
  int error = ebur128_energy_shortterm(st, &energy);
  if (error) {
    return error;
  } else if (energy <= 0.0) {
    *out = -HUGE_VAL;
    return EBUR128_SUCCESS;
  }
  *out = ebur128_energy_to_loudness(energy);
  return EBUR128_SUCCESS;
}

int ebur128_loudness_window(ebur128_state* st,
                            unsigned long window,
                            double* out) {
  double energy;
  size_t interval_frames = st->samplerate * window / 1000;
  int error = ebur128_energy_in_interval(st, interval_frames, &energy);
  if (error) {
    return error;
  } else if (energy <= 0.0) {
    *out = -HUGE_VAL;
    return EBUR128_SUCCESS;
  }
  *out = ebur128_energy_to_loudness(energy);
  return EBUR128_SUCCESS;
}

static int ebur128_double_cmp(const void *p1, const void *p2) {
  const double* d1 = (const double*) p1;
  const double* d2 = (const double*) p2;
  return (*d1 > *d2) - (*d1 < *d2);
}

/* EBU - TECH 3342 */
int ebur128_loudness_range_multiple(ebur128_state** sts, size_t size,
                                    double* out) {
  size_t i, j;
  struct ebur128_dq_entry* it;
  double* stl_vector;
  size_t stl_size;
  double* stl_relgated;
  size_t stl_relgated_size;
  double stl_power, stl_integrated;
  /* High and low percentile energy */
  double h_en, l_en;
  int use_histogram = 0;

  for (i = 0; i < size; ++i) {
    if (sts[i]) {
      if ((sts[i]->mode & EBUR128_MODE_LRA) != EBUR128_MODE_LRA) {
        return EBUR128_ERROR_INVALID_MODE;
      }
      if (i == 0 && sts[i]->mode & EBUR128_MODE_HISTOGRAM) {
        use_histogram = 1;
      } else if (use_histogram != !!(sts[i]->mode & EBUR128_MODE_HISTOGRAM)) {
        return EBUR128_ERROR_INVALID_MODE;
      }
    }
  }

  if (use_histogram) {
    unsigned long hist[1000] = { 0 };
    size_t percentile_low, percentile_high;
    size_t index;

    stl_size = 0;
    stl_power = 0.0;
    for (i = 0; i < size; ++i) {
      if (!sts[i]) {
        continue;
      }
      for (j = 0; j < 1000; ++j) {
        hist[j]   += sts[i]->d->short_term_block_energy_histogram[j];
        stl_size  += sts[i]->d->short_term_block_energy_histogram[j];
        stl_power += sts[i]->d->short_term_block_energy_histogram[j]
                     * histogram_energies[j];
      }
    }
    if (!stl_size) {
      *out = 0.0;
      return EBUR128_SUCCESS;
    }

    stl_power /= stl_size;
    stl_integrated = minus_twenty_decibels * stl_power;

    if (stl_integrated < histogram_energy_boundaries[0]) {
      index = 0;
    } else {
      index = find_histogram_index(stl_integrated);
      if (stl_integrated > histogram_energies[index]) {
        ++index;
      }
    }
    stl_size = 0;
    for (j = index; j < 1000; ++j) {
      stl_size += hist[j];
    }
    if (!stl_size) {
      *out = 0.0;
      return EBUR128_SUCCESS;
    }

    percentile_low  = (size_t) ((stl_size - 1) * 0.1 + 0.5);
    percentile_high = (size_t) ((stl_size - 1) * 0.95 + 0.5);

    stl_size = 0;
    j = index;
    while (stl_size <= percentile_low) {
      stl_size += hist[j++];
    }
    l_en = histogram_energies[j - 1];
    while (stl_size <= percentile_high) {
      stl_size += hist[j++];
    }
    h_en = histogram_energies[j - 1];
    *out = ebur128_energy_to_loudness(h_en) - ebur128_energy_to_loudness(l_en);
    return EBUR128_SUCCESS;

  } else {
    stl_size = 0;
    for (i = 0; i < size; ++i) {
      if (!sts[i]) {
        continue;
      }
      STAILQ_FOREACH(it, &sts[i]->d->short_term_block_list, entries) {
        ++stl_size;
      }
    }
    if (!stl_size) {
      *out = 0.0;
      return EBUR128_SUCCESS;
    }
    stl_vector = (double*) malloc(stl_size * sizeof(double));
    if (!stl_vector) {
      return EBUR128_ERROR_NOMEM;
    }

    j = 0;
    for (i = 0; i < size; ++i) {
      if (!sts[i]) {
        continue;
      }
      STAILQ_FOREACH(it, &sts[i]->d->short_term_block_list, entries) {
        stl_vector[j] = it->z;
        ++j;
      }
    }
    qsort(stl_vector, stl_size, sizeof(double), ebur128_double_cmp);
    stl_power = 0.0;
    for (i = 0; i < stl_size; ++i) {
      stl_power += stl_vector[i];
    }
    stl_power /= (double) stl_size;
    stl_integrated = minus_twenty_decibels * stl_power;

    stl_relgated = stl_vector;
    stl_relgated_size = stl_size;
    while (stl_relgated_size > 0 && *stl_relgated < stl_integrated) {
      ++stl_relgated;
      --stl_relgated_size;
    }

    if (stl_relgated_size) {
      h_en = stl_relgated[(size_t) ((stl_relgated_size - 1) * 0.95 + 0.5)];
      l_en = stl_relgated[(size_t) ((stl_relgated_size - 1) * 0.1 + 0.5)];
      free(stl_vector);
      *out = ebur128_energy_to_loudness(h_en) - ebur128_energy_to_loudness(l_en);
      return EBUR128_SUCCESS;
    } else {
      free(stl_vector);
      *out = 0.0;
      return EBUR128_SUCCESS;
    }
  }
}

int ebur128_loudness_range(ebur128_state* st, double* out) {
  return ebur128_loudness_range_multiple(&st, 1, out);
}

int ebur128_sample_peak(ebur128_state* st,
                        unsigned int channel_number,
                        double* out) {
  if ((st->mode & EBUR128_MODE_SAMPLE_PEAK) != EBUR128_MODE_SAMPLE_PEAK) {
    return EBUR128_ERROR_INVALID_MODE;
  } else if (channel_number >= st->channels) {
    return EBUR128_ERROR_INVALID_CHANNEL_INDEX;
  }
  *out = st->d->sample_peak[channel_number];
  return EBUR128_SUCCESS;
}

int ebur128_prev_sample_peak(ebur128_state* st,
                             unsigned int channel_number,
                             double* out) {
  if ((st->mode & EBUR128_MODE_SAMPLE_PEAK) != EBUR128_MODE_SAMPLE_PEAK) {
    return EBUR128_ERROR_INVALID_MODE;
  } else if (channel_number >= st->channels) {
    return EBUR128_ERROR_INVALID_CHANNEL_INDEX;
  }
  *out = st->d->prev_sample_peak[channel_number];
  return EBUR128_SUCCESS;
}

int ebur128_true_peak(ebur128_state* st,
                      unsigned int channel_number,
                      double* out) {
  if ((st->mode & EBUR128_MODE_TRUE_PEAK) != EBUR128_MODE_TRUE_PEAK) {
    return EBUR128_ERROR_INVALID_MODE;
  } else if (channel_number >= st->channels) {
    return EBUR128_ERROR_INVALID_CHANNEL_INDEX;
  }
  *out = st->d->true_peak[channel_number] > st->d->sample_peak[channel_number]
       ? st->d->true_peak[channel_number]
       : st->d->sample_peak[channel_number];
  return EBUR128_SUCCESS;
}

int ebur128_prev_true_peak(ebur128_state* st,
                      unsigned int channel_number,
                      double* out) {
  if ((st->mode & EBUR128_MODE_TRUE_PEAK) != EBUR128_MODE_TRUE_PEAK) {
    return EBUR128_ERROR_INVALID_MODE;
  } else if (channel_number >= st->channels) {
    return EBUR128_ERROR_INVALID_CHANNEL_INDEX;
  }
  *out = st->d->prev_true_peak[channel_number]
                              > st->d->prev_sample_peak[channel_number]
       ? st->d->prev_true_peak[channel_number]
       : st->d->prev_sample_peak[channel_number];
  return EBUR128_SUCCESS;
}
