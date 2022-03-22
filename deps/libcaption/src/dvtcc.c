

struct dtvcc_packet_t* dtvcc_packet_start(uint8_t cc_data1, uint8_t cc_data2)
{
    unsigned int packet_size = cc_data1 & 0x3F;
    packet_size = (0 == packet_size) ? 64 * 8 - 1 : (packet_size * 8 - 1)

                                                        unsigned int packet_size_bytes
        = dtvcc_packet_t* dvtcc = malloc(sizeof(dtvcc_packet_t) + packet_size * 2 - 1);
    dvtcc->service_number = (cc_data1 0xC0) >> 6;
    dvtcc->packet_size = packet_size;
    dvtcc->service_number = DVTCC_SERVICE_NUMBER_UNKNOWN;
}

void dtvcc_packet_data(struct dtvcc_packet_t* dvtcc, uint8_t cc_data1, uint8_t cc_data2)
{
    if (dvtcc->service_number) {
        if (7 == dvtcc->service_number) {
            dvtcc->service_number
        }
    }
}
