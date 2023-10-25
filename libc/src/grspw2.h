#ifndef GRSPW2_H
#define GRSPW2_H

#include <stdint.h>

#define GRSPW2_DEFAULT_MTU	0x0000404

#define GRSPW2_OP_ADD_PKT		1
#define GRSPW2_OP_ADD_RMAP		2
#define GRSPW2_OP_GET_NUM_PKT_AVAIL	3
#define GRSPW2_OP_GET_NEXT_PKT_SIZE	4
#define GRSPW2_OP_DROP_PKT		5
#define GRSPW2_OP_GET_PKT		6
#define GRSPW2_OP_GET_NEXT_PKT_EEP	7


/* XXX to transfer */
struct grspw2_data{
	uint8_t op;

	uint8_t link;
	void *hdr;
	uint32_t hdr_size;
	uint8_t non_crc_bytes;
	void *data;
	uint32_t data_size;
	uint8_t *pkt;
};


uint32_t grspw2_get_num_pkts_avail(uint8_t link);
uint32_t grspw2_get_next_pkt_size(uint8_t link);
uint32_t grspw2_drop_pkt(uint8_t link);

int32_t grspw2_add_pkt(uint8_t link, void *hdr,  uint32_t hdr_size,
			void *data, uint32_t data_size);
int32_t grspw2_add_rmap(uint8_t link, void *hdr,  uint32_t hdr_size,
			uint8_t non_crc_bytes,
			void *data, uint32_t data_size);

uint32_t grspw2_get_pkt(uint8_t link, uint8_t *pkt);

int grspw2_get_next_pkt_eep(uint8_t link);



#endif /* GRSPW2_H */
