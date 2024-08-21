#ifndef BPM_H
#define BPM_H
#ifdef __cplusplus
extern "C" {
#endif

/* BPM callback. Allocation of BPM metrics data happens automatically
 * with the first invokation of the callback associated with the output.
 * Deallocation must be done explicitly with bpm_destroy(), after the
 * callback is removed.
 *
 * BPM is designed to operate at the packet level. The bpm_inject()
 * callback function must be registered and unregistered with
 * obs_output_add_packet_callback() and obs_output_remove_packet_callback(),
 * respectively.
 */
void bpm_inject(obs_output_t *output, struct encoder_packet *pkt,
		struct encoder_packet_time *pkt_time, void *param);

/* BPM function to destroy all allocations for a given output. */
void bpm_destroy(obs_output_t *output);

#ifdef __cplusplus
}
#endif
#endif
