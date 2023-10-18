#include <syscalls.h>
#include <grspw2.h>


int32_t grspw2_add_pkt(uint8_t link,
		       void *hdr,  uint32_t hdr_size,
		       void *data, uint32_t data_size)
{
	struct grspw2_data spw;


	spw.op		  = GRSPW2_OP_ADD_PKT;
	spw.link	  = link;
	spw.hdr           = hdr;
	spw.hdr_size      = hdr_size;
	spw.data          = data;
	spw.data_size     = data_size;

	return (int32_t) sys_grspw2((void *) &spw);
}

int32_t grspw2_add_rmap(uint8_t link,
			void *hdr,  uint32_t hdr_size,
			uint8_t non_crc_bytes,
			void *data, uint32_t data_size)
{
	struct grspw2_data spw;


	spw.op		  = GRSPW2_OP_ADD_RMAP;
	spw.link	  = link;
	spw.hdr           = hdr;
	spw.hdr_size      = hdr_size;
	spw.non_crc_bytes = non_crc_bytes;
	spw.data          = data;
	spw.data_size     = data_size;

	return (int32_t) sys_grspw2((void *) &spw);
}


uint32_t grspw2_get_num_pkts_avail(uint8_t link)
{
	struct grspw2_data spw;


	spw.op   = GRSPW2_OP_GET_NUM_PKT_AVAIL;
	spw.link = link;

	return (uint32_t) sys_grspw2((void *) &spw);
}

uint32_t grspw2_get_next_pkt_size(uint8_t link)
{
	struct grspw2_data spw;


	spw.op   = GRSPW2_OP_GET_NEXT_PKT_SIZE;
	spw.link = link;

	return (uint32_t) sys_grspw2((void *) &spw);
}

uint32_t grspw2_drop_pkt(uint8_t link)
{
	struct grspw2_data spw;


	spw.op   = GRSPW2_OP_DROP_PKT;
	spw.link = link;

	return (uint32_t) sys_grspw2((void *) &spw);
}


uint32_t grspw2_get_pkt(uint8_t link, uint8_t *pkt)
{
	struct grspw2_data spw;


	spw.op   = GRSPW2_OP_GET_PKT;
	spw.link = link;
	spw.pkt  = pkt;

	return (uint32_t) sys_grspw2((void *) &spw);
}

