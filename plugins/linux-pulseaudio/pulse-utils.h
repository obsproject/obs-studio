#include <inttypes.h>
#include <pulse/stream.h>

/**
 * Get obs audo format from pulse audio format
 * @param format pulseaudio format
 *
 * @return obs audio format
 */
enum audio_format pulse_to_obs_audio_format(pa_sample_format_t format);

/**
 * Get obs speaker layout from number of channels
 *
 * @param channels number of channels reported by pulseaudio
 *
 * @return obs speaker_layout id
 *
 * @note This *might* not work for some rather unusual setups, but should work
 *       fine for the majority of cases.
 */
enum speaker_layout pulse_channels_to_obs_speakers(uint_fast32_t channels);

/**
 * Get a pulseaudio channel map for an obs speaker layout
 * 
 * @param layout obs speaker layout
 * 
 * @return pulseaudio channel map
 */
pa_channel_map pulse_channel_map(enum speaker_layout layout);
