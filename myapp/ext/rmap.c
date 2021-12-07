/**
 * @file   rmap.c
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
 *
 * @note the extended address byte is always set to 0x0
 */



#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <rmap.h>





/**
 * @brief valiidates a command code
 *
 * @param cmd the command code
 *
 * @returns 0 on success, error otherwise
 */

static int rmap_validate_cmd_code(uint8_t cmd)
{
	switch (cmd) {
	case RMAP_READ_ADDR_SINGLE:
	case RMAP_READ_ADDR_INC:
	case RMAP_READ_MODIFY_WRITE_ADDR_INC:
	case RMAP_WRITE_ADDR_SINGLE:
	case RMAP_WRITE_ADDR_INC:
	case RMAP_WRITE_ADDR_SINGLE_REPLY:
	case RMAP_WRITE_ADDR_INC_REPLY:
	case RMAP_WRITE_ADDR_SINGLE_VERIFY:
	case RMAP_WRITE_ADDR_INC_VERIFY:
	case RMAP_WRITE_ADDR_SINGLE_VERIFY_REPLY:
	case RMAP_WRITE_ADDR_INC_VERIFY_REPLY:
		return 0;
	default:
		return -1;
	}
}


/**
 * @brief get the minimum header size given the RMAP instruction
 *
 * @param pkt a struct rmap_pkt
 *
 * @returns header size or -1 on error
 */

static int rmap_get_min_hdr_size(struct rmap_pkt *pkt)
{


	switch (pkt->ri.cmd) {
	case RMAP_READ_ADDR_SINGLE:
	case RMAP_READ_ADDR_INC:
	case RMAP_READ_MODIFY_WRITE_ADDR_INC:

		if (pkt->ri.cmd_resp)
			return RMAP_HDR_MIN_SIZE_READ_CMD;

		return RMAP_HDR_MIN_SIZE_READ_REP;

	case RMAP_WRITE_ADDR_SINGLE:
	case RMAP_WRITE_ADDR_INC:
	case RMAP_WRITE_ADDR_SINGLE_REPLY:
	case RMAP_WRITE_ADDR_INC_REPLY:
	case RMAP_WRITE_ADDR_SINGLE_VERIFY:
	case RMAP_WRITE_ADDR_INC_VERIFY:
	case RMAP_WRITE_ADDR_SINGLE_VERIFY_REPLY:
	case RMAP_WRITE_ADDR_INC_VERIFY_REPLY:

		if (pkt->ri.cmd_resp)
			return RMAP_HDR_MIN_SIZE_WRITE_CMD;

		return RMAP_HDR_MIN_SIZE_WRITE_REP;

	default:
		return -1;
	}
}

/**
 * @brief calculate the CRC8 of a given buffer
 *
 * @param buf the buffer containing the data
 * @param len the length of the buffer
 *
 * @returns the CRC8
 */

uint8_t rmap_crc8(const uint8_t *buf, const size_t len)
{
	size_t i;

	uint8_t crc8 = 0;

	/* crc8 lookup table from ECSS‐E‐ST‐50‐52C A.3 */
	const uint8_t crc8_lt[256] = {
		0x00, 0x91, 0xe3, 0x72, 0x07, 0x96, 0xe4, 0x75,
		0x0e, 0x9f, 0xed, 0x7c, 0x09, 0x98, 0xea, 0x7b,
		0x1c, 0x8d, 0xff, 0x6e, 0x1b, 0x8a, 0xf8, 0x69,
		0x12, 0x83, 0xf1, 0x60, 0x15, 0x84, 0xf6, 0x67,
		0x38, 0xa9, 0xdb, 0x4a, 0x3f, 0xae, 0xdc, 0x4d,
		0x36, 0xa7, 0xd5, 0x44, 0x31, 0xa0, 0xd2, 0x43,
		0x24, 0xb5, 0xc7, 0x56, 0x23, 0xb2, 0xc0, 0x51,
		0x2a, 0xbb, 0xc9, 0x58, 0x2d, 0xbc, 0xce, 0x5f,
		0x70, 0xe1, 0x93, 0x02, 0x77, 0xe6, 0x94, 0x05,
		0x7e, 0xef, 0x9d, 0x0c, 0x79, 0xe8, 0x9a, 0x0b,
		0x6c, 0xfd, 0x8f, 0x1e, 0x6b, 0xfa, 0x88, 0x19,
		0x62, 0xf3, 0x81, 0x10, 0x65, 0xf4, 0x86, 0x17,
		0x48, 0xd9, 0xab, 0x3a, 0x4f, 0xde, 0xac, 0x3d,
		0x46, 0xd7, 0xa5, 0x34, 0x41, 0xd0, 0xa2, 0x33,
		0x54, 0xc5, 0xb7, 0x26, 0x53, 0xc2, 0xb0, 0x21,
		0x5a, 0xcb, 0xb9, 0x28, 0x5d, 0xcc, 0xbe, 0x2f,
		0xe0, 0x71, 0x03, 0x92, 0xe7, 0x76, 0x04, 0x95,
		0xee, 0x7f, 0x0d, 0x9c, 0xe9, 0x78, 0x0a, 0x9b,
		0xfc, 0x6d, 0x1f, 0x8e, 0xfb, 0x6a, 0x18, 0x89,
		0xf2, 0x63, 0x11, 0x80, 0xf5, 0x64, 0x16, 0x87,
		0xd8, 0x49, 0x3b, 0xaa, 0xdf, 0x4e, 0x3c, 0xad,
		0xd6, 0x47, 0x35, 0xa4, 0xd1, 0x40, 0x32, 0xa3,
		0xc4, 0x55, 0x27, 0xb6, 0xc3, 0x52, 0x20, 0xb1,
		0xca, 0x5b, 0x29, 0xb8, 0xcd, 0x5c, 0x2e, 0xbf,
		0x90, 0x01, 0x73, 0xe2, 0x97, 0x06, 0x74, 0xe5,
		0x9e, 0x0f, 0x7d, 0xec, 0x99, 0x08, 0x7a, 0xeb,
		0x8c, 0x1d, 0x6f, 0xfe, 0x8b, 0x1a, 0x68, 0xf9,
		0x82, 0x13, 0x61, 0xf0, 0x85, 0x14, 0x66, 0xf7,
		0xa8, 0x39, 0x4b, 0xda, 0xaf, 0x3e, 0x4c, 0xdd,
		0xa6, 0x37, 0x45, 0xd4, 0xa1, 0x30, 0x42, 0xd3,
		0xb4, 0x25, 0x57, 0xc6, 0xb3, 0x22, 0x50, 0xc1,
		0xba, 0x2b, 0x59, 0xc8, 0xbd, 0x2c, 0x5e, 0xcf,
	};



	if (!buf)
		return 0;


	for (i = 0; i < len; i++)
		crc8 = crc8_lt[crc8 ^ buf[i]];

	return crc8;
}


/**
 * @brief create an RMAP packet and set defaults
 *
 *
 * @note initialises protocol id to 1 and all others to 0
 *
 * @returns a struct rmap_pkt or NULL on error
 */

struct rmap_pkt *rmap_create_packet(void)
{
	struct rmap_pkt *pkt;


	pkt = (struct rmap_pkt *) calloc(sizeof(struct rmap_pkt), 1);
	if (pkt)
		pkt->proto_id = RMAP_PROTOCOL_ID;

	return pkt;
}


/**
 * @brief destroys an RMAP packet
 *
 * @param pkt a struct rmap_pkt
 *
 * @note this will NOT deallocate and pointer references assigned by the user
 */

void rmap_destroy_packet(struct rmap_pkt *pkt)
{
	free(pkt);
}


/**
 * @brief completely destroys an RMAP packet
 *
 * @param pkt a struct rmap_pkt
 *
 * @note this will attempt to deallocate any pointer references assigned by the
 * 	 user
 * @warning use with care
 */

void rmap_erase_packet(struct rmap_pkt *pkt)
{
	free(pkt->path);
	free(pkt->rpath);
	free(pkt->data);
	free(pkt);
}

/**
 * @brief set the destination (target) logical address
 *
 * @param pkt	a struct rmap_pkt
 * @param addr	the destination logical address
 */

void rmap_set_dst(struct rmap_pkt *pkt, uint8_t addr)
{
	if (pkt)
		pkt->dst = addr;
}


/**
 * @brief set the source (initiator) logical address
 *
 * @param pkt	a struct rmap_pkt
 * @param addr	the source logical address
 */

void rmap_set_src(struct rmap_pkt *pkt, uint8_t addr)
{
	if (pkt)
		pkt->src = addr;
}


/**
 * @brief set the command authorisation key
 *
 * @param pkt	a struct rmap_pkt
 * @param key	the authorisation key
 */

void rmap_set_key(struct rmap_pkt *pkt, uint8_t key)
{
	if (pkt)
		pkt->key = key;
}


/**
 * @brief set the reply address path
 *
 * @param pkt	a struct rmap_pkt
 * @param rpath the reply path
 * @param len   the number of elements in the reply path (multiple of 4)
 *
 * @note see ECSS‐E‐ST‐50‐52C 5.1.6 for return path rules
 *
 * @returns 0 on success, -1 on error
 */

int rmap_set_reply_path(struct rmap_pkt *pkt, const uint8_t *rpath, uint8_t len)
{
	if (!pkt)
		return -1;

	if (!rpath && len)
		return -1;

	if (len > RMAP_MAX_REPLY_PATH_LEN)
		return -1;

	if (len & 0x3)
		return -1;

	pkt->rpath_len = len;

	pkt->rpath = (uint8_t *) malloc(pkt->rpath_len);
	if (!pkt->rpath)
		return -1;

	memcpy(pkt->rpath, rpath, pkt->rpath_len);

	/* number of 32 bit words needed to contain the path */
	pkt->ri.reply_addr_len = len >> 2;

	return 0;
}


/**
 * @brief set the destination address path
 *
 * @param pkt	a struct rmap_pkt
 * @param path the destination path
 * @param len   the number of elements in the destination path
 *
 * @note see ECSS‐E‐ST‐50‐52C 5.1.6 for return path rules
 *
 * @returns 0 on success, -1 on error
 */

int rmap_set_dest_path(struct rmap_pkt *pkt, const uint8_t *path, uint8_t len)
{
	if (!pkt)
		return -1;

	if (!path && len)
		return -1;

	if (len > RMAP_MAX_PATH_LEN)
		return -1;

	pkt->path_len = len;

	pkt->path = (uint8_t *) malloc(pkt->path_len);
	if (!pkt->path)
		return -1;

	memcpy(pkt->path, path, pkt->path_len);

	return 0;
}


/**
 * @brief set an RMAP command
 *
 * @param pkt	a struct rmap_pkt
 * @param cmd	the selected command
 *
 * @param returns -1 on error
 */

int rmap_set_cmd(struct rmap_pkt *pkt, uint8_t cmd)
{
	if (!pkt)
		return -1;

	if (rmap_validate_cmd_code(cmd))
		return -1;


	pkt->ri.cmd      = cmd & 0xF;
	pkt->ri.cmd_resp = 1;

	return 0;
}


/**
 * @brief set an RMAP transaction identifier
 *
 * @param pkt	a struct rmap_pkt
 * @param id	the transaction identifier
 */

void rmap_set_tr_id(struct rmap_pkt *pkt, uint16_t id)
{
	if (!pkt)
		return;

	pkt->tr_id = id;
}


/**
 * @brief set a data address
 *
 * @param pkt	a struct rmap_pkt
 * @param addr	the address
 */

void rmap_set_data_addr(struct rmap_pkt *pkt, uint32_t addr)
{
	if (!pkt)
		return;

	pkt->addr = addr;
}

/**
 * @brief set an RMAP command
 *
 * @param pkt	a struct rmap_pkt
 * @param len	the data length (in bytes)
 *
 * @param returns -1 on error
 *
 * @note the length is at most 2^24-1 bytes
 * @note if the RMAP command is of 'SINGLE' type, only multiples of 4
 *	 will result in successfull execution of the command (at least
 *	 with the GRSPW2 core)
 */

int rmap_set_data_len(struct rmap_pkt *pkt, uint32_t len)
{
	if (!pkt)
		return -1;

	if (len > RMAP_MAX_DATA_LEN)
		return -1;

	pkt->data_len = len;

	return 0;
}


/**
 * @brief build an rmap header
 *
 * @param pkt	a struct rmap_pkt
 * @param hdr	the header buffer; if NULL, the function returns the needed size
 *
 * @returns -1 on error, size of header otherwise
 */

int rmap_build_hdr(struct rmap_pkt *pkt, uint8_t *hdr)
{
	int i;
	int n = 0;


	if (!pkt)
		return -1;

	if (!hdr) {
		n = rmap_get_min_hdr_size(pkt);
		n += pkt->path_len;
		n += pkt->rpath_len;
		return n;
	}


	for (i = 0; i < pkt->path_len; i++)
		hdr[n++] = pkt->path[i];	/* routing path to target */

	hdr[n++] = pkt->dst;			/* target logical address */
	hdr[n++] = pkt->proto_id;		/* protocol id */
	hdr[n++] = pkt->instruction;		/* instruction */
	hdr[n++] = pkt->key;			/* key/status */

	for (i = 0; i < pkt->rpath_len; i++)
		hdr[n++] = pkt->rpath[i];	/* return path to source */

	hdr[n++] = pkt->src;			/* source logical address */
	hdr[n++] = (uint8_t) (pkt->tr_id >> 8);	/* MSB of transaction id */
	hdr[n++] = (uint8_t)  pkt->tr_id;	/* LSB of transaction id */


	/* commands have a data address */
	if (pkt->ri.cmd_resp) {
		hdr[n++] = 0x0;	/* extended address field (unused) */
		hdr[n++] = (uint8_t) (pkt->addr >> 24); /* data addr MSB */
		hdr[n++] = (uint8_t) (pkt->addr >> 16);
		hdr[n++] = (uint8_t) (pkt->addr >>  8);
		hdr[n++] = (uint8_t)  pkt->addr;	/* data addr LSB */
	} else if (!pkt->ri.cmd_resp && pkt->ri.cmd & RMAP_CMD_BIT_WRITE) {
		/* all headers have data length unless they are a write reply */
		return n;
	} else {
		hdr[n++] = 0x0;	/* on other replies, this is a reserved field */
	}

	hdr[n++] = (uint8_t) (pkt->data_len >> 16); /* data len MSB */
	hdr[n++] = (uint8_t) (pkt->data_len >>  8);
	hdr[n++] = (uint8_t)  pkt->data_len;	    /* data len LSB */

	return n;
}


/**
 * @brief create an rmap packet from a buffer
 *
 * @param buf the buffer, with the target path stripped away, i.e.
 *	  starting with <logical address>, <protocol id>, ...
 *
 * @note there is no size checking, be careful
 *
 * @returns an rmap packet, containing the decoded buffer including any data,
 *	    NULL on error
 */

struct rmap_pkt *rmap_pkt_from_buffer(uint8_t *buf)
{
	size_t n = 0;
	size_t i;

	struct rmap_pkt *pkt = NULL;


	if (!buf)
		goto error;

	if (buf[RMAP_PROTOCOL_ID] != RMAP_PROTOCOL_ID) {
		printf("Not an RMAP packet, got %x but expected %x\n",
		       buf[RMAP_PROTOCOL_ID], RMAP_PROTOCOL_ID);
		goto error;
	}

	pkt = rmap_create_packet();
	if (!pkt) {
		printf("Error creating packet\n");
		goto error;
	}

	pkt->dst         = buf[RMAP_DEST_ADDRESS];
	pkt->proto_id    = buf[RMAP_PROTOCOL_ID];
	pkt->instruction = buf[RMAP_INSTRUCTION];
	pkt->key         = buf[RMAP_CMD_DESTKEY];


	if (pkt->ri.cmd_resp) {
		pkt->rpath_len = pkt->ri.reply_addr_len << 2;

		pkt->rpath = (uint8_t *) malloc(pkt->rpath_len);
		if (!pkt->rpath)
			goto error;

		for (i = 0; i < pkt->rpath_len; i++)
			pkt->rpath[i] = buf[RMAP_REPLY_ADDR_START + i];


		n = pkt->rpath_len; /* rpath skip */
	}

	pkt->src   = buf[RMAP_SRC_ADDR + n];
	pkt->tr_id = ((uint16_t) buf[RMAP_TRANS_ID_BYTE0 + n] << 8) |
	              (uint16_t) buf[RMAP_TRANS_ID_BYTE1 + n];

	/* commands have a data address */
	if (pkt->ri.cmd_resp) {
		pkt->addr = ((uint32_t) buf[RMAP_ADDR_BYTE0 + n] << 24) |
			    ((uint32_t) buf[RMAP_ADDR_BYTE1 + n] << 16) |
			    ((uint32_t) buf[RMAP_ADDR_BYTE2 + n] <<  8) |
			     (uint32_t) buf[RMAP_ADDR_BYTE3 + n];
		n += 4; /* addr skip, extended byte is incorporated in define */
	}

	/* all headers have data length unless they are a write reply */
	if (!(!pkt->ri.cmd_resp && (pkt->ri.cmd & (RMAP_CMD_BIT_WRITE)))) {

		pkt->data_len = ((uint32_t) buf[RMAP_DATALEN_BYTE0 + n] << 16) |
				((uint32_t) buf[RMAP_DATALEN_BYTE1 + n] <<  8) |
			         (uint32_t) buf[RMAP_DATALEN_BYTE2 + n];
	}

	if (pkt->data_len) {
		pkt->data = (uint8_t *) malloc(pkt->data_len);
		if (!pkt->data)
			goto error;

		for (i = 0; i < pkt->data_len; i++)
			pkt->data[i] = buf[RMAP_DATA_START + n + i];
	}


	return pkt;

error:
	if (pkt) {
		free(pkt->data);
		free(pkt->rpath);
		free(pkt);
	}

	return NULL;
}



/**** UNFINISHED INFO STUFF BELOW ******/

__extension__
static int rmap_check_status(uint8_t status)
{


	printf("\tStatus: ");

	switch (status) {
	case RMAP_STATUS_SUCCESS:
		printf("Command executed successfully");
		break;
	case RMAP_STATUS_GENERAL_ERROR:
		printf("General error code");
		break;
	case RMAP_STATUS_UNUSED_TYPE_OR_CODE:
		printf("Unused RMAP Packet Type or Command Code");
		break;
	case RMAP_STATUS_INVALID_KEY:
		printf("Invalid key");
		break;
	case RMAP_STATUS_INVALID_DATA_CRC:
		printf("Invalid Data CRC");
		break;
	case RMAP_STATUS_EARLY_EOP:
		printf("Early EOP");
		break;
	case RMAP_STATUS_TOO_MUCH_DATA:
		printf("Too much data");
		break;
	case RMAP_STATUS_EEP:
		printf("EEP");
		break;
	case RMAP_STATUS_RESERVED:
		printf("Reserved");
		break;
	case RMAP_STATUS_VERIFY_BUFFER_OVERRRUN:
		printf("Verify buffer overrrun");
		break;
	case RMAP_STATUS_CMD_NOT_IMPL_OR_AUTH:
		printf("RMAP Command not implemented or not authorised");
		break;
	case RMAP_STATUS_RMW_DATA_LEN_ERROR:
		printf("RMW Data Length error");
		break;
	case RMAP_STATUS_INVALID_TARGET_LOGICAL_ADDR:
		printf("Invalid Target Logical Address");
		break;
	default:
		printf("Reserved unused error code %d", status);
		break;
	}

	printf("\n");


	return status;
}



static void rmap_process_read_reply(uint8_t *pkt)
{
	uint32_t i;

	uint32_t len = 0;


	len |= ((uint32_t) pkt[RMAP_DATALEN_BYTE0]) << 16;
	len |= ((uint32_t) pkt[RMAP_DATALEN_BYTE1]) <<  8;
	len |= ((uint32_t) pkt[RMAP_DATALEN_BYTE2]) <<  0;

	printf("\tData length is %u bytes:\n\t", len);


	for (i = 0; i < len; i++)
		printf("%02x:", pkt[RMAP_DATA_START + i]);

	printf("\b \n");
}




static void rmap_parse_cmd_pkt(uint8_t *pkt)
{
	(void) pkt;
	printf("\trmap_parse_cmd_pkt() not implemented\n");
}


static void rmap_parse_reply_pkt(uint8_t *pkt)
{
	struct rmap_instruction *ri;


	ri = (struct rmap_instruction *) &pkt[RMAP_INSTRUCTION];

	printf("\tInstruction: ");

	switch (ri->cmd) {

	case RMAP_READ_ADDR_SINGLE:
		printf("Read single address\n");
		rmap_process_read_reply(pkt);
		break;
	case RMAP_READ_ADDR_INC:
		printf("Read incrementing address\n");
		rmap_process_read_reply(pkt);
		break;
	case RMAP_READ_MODIFY_WRITE_ADDR_INC:
		printf("RMW incrementing address verify reply\n");
		break;
	case RMAP_WRITE_ADDR_INC_VERIFY_REPLY:
		printf("Write incrementing address verify reply\n");
		break;
	case RMAP_WRITE_ADDR_INC_REPLY:
		printf("Write incrementing address reply\n");
		break;
	default:
		printf("decoding of instruction 0x%02X not implemented\n",
		       ri->cmd);
		break;
	}
}


/**
 * parse an RMAP packet:
 *
 * expected format: <logical address> <protocol id> ...
 */

void rmap_parse_pkt(uint8_t *pkt)
{
	struct rmap_instruction *ri;

	if (pkt[RMAP_PROTOCOL_ID] != RMAP_PROTOCOL_ID) {
		printf("\nNot an RMAP packet, got %x but expected %x\n",
		       pkt[RMAP_PROTOCOL_ID], RMAP_PROTOCOL_ID);
		return;
	}


	ri = (struct rmap_instruction *) &pkt[RMAP_INSTRUCTION];

	if (ri->cmd_resp) {
		printf("This is a command packet\n");
		if (!rmap_check_status(pkt[RMAP_REPLY_STATUS]))
			rmap_parse_cmd_pkt(pkt);
	} else {
		printf("This is a reply packet\n");
		if (!rmap_check_status(pkt[RMAP_REPLY_STATUS]))
			rmap_parse_reply_pkt(pkt);
	}
}

