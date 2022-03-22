#ifndef LIBCAPTION_CEA708_H
#define LIBCAPTION_CEA708_H
#ifdef __cplusplus
extern "C" {
#endif
////////////////////////////////////////////////////////////////////////////////
struct dtvcc_packet_t {
    unsigned int sequence_number;
    unsigned int packet_size;
    unsigned int serice_number;
};

#defing DVTCC_SERVICE_NUMBER_UNKNOWN

// static inline size_t dvtvcc_packet_size_bytes(const struct dtvcc_packet_t *dvtcc) { return dvtcc->packet_size*2-1;}

////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
#endif
