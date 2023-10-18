/**
 * @file   rmap.h
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2018
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief rmap command/reply helper functions
 */

#ifndef RMAP_H
#define RMAP_H

#include <stdint.h>
#include <stdlib.h>

/**
 * valid RMAP command codes, see Table 5-1 of ECSS‐E‐ST‐50‐52C
 *
 * all valid commands are made up of the four bits below
 */


#define RMAP_CMD_BIT_WRITE			0x8
#define RMAP_CMD_BIT_VERIFY			0x4
#define RMAP_CMD_BIT_REPLY			0x2
#define RMAP_CMD_BIT_INC			0x1

#define RMAP_READ_ADDR_SINGLE			0x2
#define RMAP_READ_ADDR_INC			0x3
#define RMAP_READ_MODIFY_WRITE_ADDR_INC		0x7
#define RMAP_WRITE_ADDR_SINGLE			0x8
#define RMAP_WRITE_ADDR_INC			0x9
#define RMAP_WRITE_ADDR_SINGLE_REPLY		0xa
#define RMAP_WRITE_ADDR_INC_REPLY		0xb
#define RMAP_WRITE_ADDR_SINGLE_VERIFY		0xc
#define RMAP_WRITE_ADDR_INC_VERIFY		0xd
#define RMAP_WRITE_ADDR_SINGLE_VERIFY_REPLY	0xe
#define RMAP_WRITE_ADDR_INC_VERIFY_REPLY	0xf

/**
 * RMAP error and status codes, see Table 5-4 of ECSS‐E‐ST‐50‐52C
 */

#define RMAP_STATUS_SUCCESS			0x0
#define RMAP_STATUS_GENERAL_ERROR		0x1
#define RMAP_STATUS_UNUSED_TYPE_OR_CODE		0x2
#define RMAP_STATUS_INVALID_KEY			0x3
#define RMAP_STATUS_INVALID_DATA_CRC		0x4
#define RMAP_STATUS_EARLY_EOP			0x5
#define RMAP_STATUS_TOO_MUCH_DATA		0x6
#define RMAP_STATUS_EEP				0x7
#define RMAP_STATUS_RESERVED			0x8
#define RMAP_STATUS_VERIFY_BUFFER_OVERRRUN	0x9
#define RMAP_STATUS_CMD_NOT_IMPL_OR_AUTH	0xa
#define RMAP_STATUS_RMW_DATA_LEN_ERROR		0xb
#define RMAP_STATUS_INVALID_TARGET_LOGICAL_ADDR	0xc


/**
 * RMAP minimum header sizes, see ECSS‐E‐ST‐50‐52C
 */

#define RMAP_HDR_MIN_SIZE_WRITE_CMD		15
#define RMAP_HDR_MIN_SIZE_WRITE_REP		 7

#define RMAP_HDR_MIN_SIZE_READ_CMD		RMAP_HDR_MIN_SIZE_WRITE_CMD
#define RMAP_HDR_MIN_SIZE_READ_REP		11

#define RMAP_HDR_MIN_SIZE_RMW_CMD		RMAP_HDR_MIN_SIZE_READ_CMD
#define RMAP_HDR_MIN_SIZE_RMW_REP		RMAP_HDR_MIN_SIZE_READ_REP



/* RMAP header bytes in relative offsets following last entry in target path */
#define RMAP_DEST_ADDRESS	0x00
#define RMAP_PROTOCOL_ID	0x01
#define RMAP_INSTRUCTION	0x02
#define RMAP_CMD_DESTKEY	0x03
#define RMAP_REPLY_STATUS	RMAP_CMD_DESTKEY
#define RMAP_REPLY_ADDR_START	0x04	/* optional */

/* RMAP header bytes in relative offsets, add (reply address length * 4) */
#define RMAP_SRC_ADDR		0x04
#define RMAP_TRANS_ID_BYTE0	0x05
#define RMAP_TRANS_ID_BYTE1	0x06

/* depending on the command, this is 0 or may contain an address extension */
#define RMAP_RESERVED		0x07
#define RMAP_EXTENDED		RMAP_RESERVED

/* optional RMAP header bytes in relative offsets */
#define RMAP_ADDR_BYTE0		0x08
#define RMAP_ADDR_BYTE1		0x09
#define RMAP_ADDR_BYTE2		0x0a
#define RMAP_ADDR_BYTE3		0x0b

/* RMAP header bytes in relative offsets (add extra 4 if address present)  */
#define RMAP_DATALEN_BYTE0	0x08
#define RMAP_DATALEN_BYTE1	0x09
#define RMAP_DATALEN_BYTE2	0x0a
#define RMAP_HEADER_CRC		0x0b
#define RMAP_DATA_START		0x0c

/**
 * While the size of a SpW packet is in principl not limited, the size of the
 * header cannot be more than 255 bytes given the 8-bit width of the transfer
 * descriptor's HEADERLEN field in the GRSPW2 core, so we'll use that as our
 * limit.
 *
 * The reply address path can be at most 12 bytes, as the reply address length
 * field in the RMAP instruction field is only 2 bits wide and counts the
 * number of 32 bit words needed to hold the return path.
 *
 * The maximum data length is 2^24-1 bits
 *
 * All other fields in the header (not counting the header CRC) amount to
 * 27 bytes.
 *
 * @see GR712RC-UM v2.7 p112 and ECSS‐E‐ST‐50‐52C e.g. 5.3.1.1
 */

#define RMAP_MAX_PATH_LEN		 15
#define RMAP_MAX_REPLY_ADDR_LEN		  3
#define RMAP_MAX_REPLY_PATH_LEN		 12
#define RMAP_MAX_DATA_LEN	   0xFFFFFFUL



__extension__
struct rmap_instruction {
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	uint8_t	reserved:1;
	uint8_t	cmd_resp:1;
	uint8_t cmd:4;
	uint8_t	reply_addr_len:2;
#elif (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	uint8_t	reply_addr_len:2;
	uint8_t cmd:4;
	uint8_t	cmd_resp:1;
	uint8_t	reserved:1;
#else
#error "Unknown byte order"
#endif
}__attribute__((packed));
#if 0
compile_time_assert((sizeof(struct rmap_instruction) == sizeof(uint8_t),
		    RMAP_INSTRUCTION_STRUCT_WRONG_SIZE));
#endif


/**
 * This structure holds the relevant contents of an RMAP packet.
 *
 * @note this is NOT an actual RMAP packet!
 */

__extension__
struct rmap_pkt {
	uint8_t		*path;		/* path to SpW target */
	uint8_t		path_len;	/* entries in the path */
	uint8_t		dst;		/* target logical address */
	uint8_t		proto_id;	/* protoco id (0x1 = RMAP */
	union {
		struct rmap_instruction ri;
		uint8_t	instruction;
	};
	union {
		uint8_t	key;		/* command authorisation key */
		uint8_t	status;		/* reply error/status codes */
	};
	uint8_t		src;		/* initiator logical address */
	uint8_t		*rpath;		/* reply path */
	uint8_t		rpath_len;	/* entries in the reply path */
	uint16_t	tr_id;		/* transaction identifier */
	uint32_t	addr;		/* (first) data address */
	uint8_t		*data;
	uint32_t	data_len;	/* lenght of data in bytes */
};



uint8_t rmap_crc8(const uint8_t *buf, const size_t len);

struct rmap_pkt *rmap_create_packet(void);
struct rmap_pkt *rmap_pkt_from_buffer(uint8_t *buf);
int rmap_build_hdr(struct rmap_pkt *pkt, uint8_t *hdr);
int rmap_set_data_len(struct rmap_pkt *pkt, uint32_t len);
void rmap_set_data_addr(struct rmap_pkt *pkt, uint32_t addr);
int rmap_set_cmd(struct rmap_pkt *pkt, uint8_t cmd);


void rmap_set_dst(struct rmap_pkt *pkt, uint8_t addr);
void rmap_set_src(struct rmap_pkt *pkt, uint8_t addr);
void rmap_set_key(struct rmap_pkt *pkt, uint8_t key);
void rmap_set_tr_id(struct rmap_pkt *pkt, uint16_t id);

int rmap_set_reply_path(struct rmap_pkt *pkt, const uint8_t *rpath, uint8_t len);
int rmap_set_dest_path(struct rmap_pkt *pkt, const uint8_t *path, uint8_t len);


void rmap_erase_packet(struct rmap_pkt *pkt);


void rmap_parse_pkt(uint8_t *pkt);


#endif /* RMAP_H */
