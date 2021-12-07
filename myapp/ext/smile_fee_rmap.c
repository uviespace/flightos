/**
 * @file   smile_fee_rmap.c
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
 * @brief RMAP FEE link interface
 *
 *
 * Ideally, we would use asynchronous operations here, with a transaction log
 * that is served by another thread. However, we cannot assume that such
 * features will be available, so we'll do this by maintaining a mirror of the
 * FEE's registers and memory, where instead of actively blocking with get()
 * and set() RMAP calls, they operate on the local copy and the user issues
 * sync() calls.
 *
 * To monitor the syncronisation status, we maintaining a transaction log
 * tracking the submitted command set. Response packets could be processed
 * by interrupt (or thread), but in this variant, we process the return packets
 * when the user calls smile_fee_ctrl_sync_status()
 *
 * Note that for simplicity , we assume that there is a working heap allocator
 * available, please adapt all malloc/free calls to your needs, or ask us
 * to do that for you.
 
 * NOTE: We don't have to serve more than one FEE at any given time, so we
 *	 track addresses and paths internally in a single instance. This also
 *	 makes the interface less cluttered. Besides, I'm lazy.
 *
 *
 * @warn when operational, we expect to have exclusive control of the SpW link
 *
 * TODO: RMAP command response evaluation
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rmap.h>
#include <smile_fee_rmap.h>



static uint8_t smile_fee_addr;
static uint8_t icu_addr;

static uint8_t *dpath;	/* destination path (to the FEE) */
static uint8_t *rpath;	/* return path (to the ICU) */
static uint8_t dpath_len;
static uint8_t rpath_len;

static uint8_t dst_key;	/* destination command key */




/* generic calls, functions must be provided to init() */
static int32_t (*rmap_tx)(void *hdr,  uint32_t hdr_size,
			  const uint8_t non_crc_bytes,
			  const void *data, uint32_t data_size);
static uint32_t (*rmap_rx)(uint8_t *pkt);

static int data_mtu;	/* maximum data transfer size per unit */










/* For now we maintain a simple transaction log that works like this:
 * we allow up to 128 transfers, simply because this is how many response
 * packets the GRSPW2 SpW descriptor table can hold at any one time without
 * blocking the link.
 *
 * Every time a new transfer is to be submitted, we grab the first free
 * slot we encounter in the "in_use" array and use the index as the transaction
 * identifier of the RMAP packet. (Yes, this is potentially slow, as we may have
 * to iterate over the whole array every time if we're very busy with
 * transmissions, so there is room for improvement.)
 *
 * Every time a slot is retrieved, the "pending" counter is incremented to
 * have a fast indicator of the synchronisation status, i.e. if "pending"
 * is not set, the synchronisation procedure is complete and the local data may
 * be read, or the remote data has been written and further commands may may
 * be issued.
 *
 * The local (mirror) start address of the requested remote address is stored
 * into the same slot in the "local_addr" array, so we'll know where to put the
 * data if we issue an RMAP_read command. This may be omitted for write
 * commands.
 *
 * Every time a response packet is received, the data (if any) is written to the
 * local address, using the length specified by RMAP packet, so be careful where
 * you place your buffers or registers. On success, the "in_use" slot is cleared
 * and the pending counter is improved.
 *
 * XXX: careful, no locking is used on any of the log data, so this is
 * single-thread-use only!
 *
 */
#define TRANS_LOG_SIZE 64	/* GRSPW2 TX descriptor limit */
static struct {

	uint8_t  in_use[TRANS_LOG_SIZE];
	void    *local_addr[TRANS_LOG_SIZE];

	int pending;
} trans_log;


/**
 * @brief grab a slot in the transaction log
 *
 * @param local_addr the local memory address
 *
 * @returns -1 on no slots, >= 0 for the transaction id
 */

static int trans_log_grab_slot(void *local_addr)
{
	int i;
	int slot = -1;

	for (i = 0; i < TRANS_LOG_SIZE; i++) {

		if (trans_log.in_use[i])
			continue;

		/* got one */
		slot = i;
		trans_log.in_use[slot] = 1;
		trans_log.local_addr[slot] = local_addr;
		trans_log.pending++;
		break;
	}

	return slot;
}


/**
 * @brief release a slot in the transaction log
 *
 * @param slot the id of the slot
 *
 */

static void trans_log_release_slot(int slot)
{

	if (slot < 0)
		return;

	if (slot >= TRANS_LOG_SIZE)
		return;

	if (!trans_log.in_use[slot])
		return;

	trans_log.in_use[slot] = 0;
	trans_log.pending--;
}


/**
 * @brief get the local address for a slot
 *
 * @param slot the id of the slot
 *
 * @returns the address or NULL if not found/slot not in use
 */

static void *trans_log_get_addr(int slot)
{
	if (slot < 0)
		return NULL;

	if (slot >= TRANS_LOG_SIZE)
		return NULL;

	if (!trans_log.in_use[slot])
		return NULL;

	return trans_log.local_addr[slot];
}

/**
 * @brief n rmap command transaction
 *
 * @returns number of packets processed or < 0 on error
 */

static int smile_fee_process_rx(void)
{
	int n;
	int cnt = 0;

	void *local_addr;

	uint8_t *spw_pckt;

	struct rmap_pkt *rp;


	if (!rmap_rx)
		return -1;

	/* process all pending responses */
	while ((n = rmap_rx(NULL))) {
		/* we received something, allocate enough space for the packet */
		spw_pckt = (uint8_t *) malloc(n);
		if(!spw_pckt) {
			printf("malloc() for packet failed!\n");
			return -1;
		}

		/* read the packet */
		n = rmap_rx(spw_pckt);
		if (!n) {
			printf("Unknown error in rmap_rx()\n");
			free(spw_pckt);
			return -1;
		}

		cnt++;

		if (0)
			rmap_parse_pkt(spw_pckt);

		/* convert format */
		rp = rmap_pkt_from_buffer(spw_pckt);
		free(spw_pckt);

		if (!rp) {
			printf("Error converting to RMAP packet\n");
			continue;
		}

		local_addr = trans_log_get_addr(rp->tr_id);

		if (!local_addr) {
			printf("warning: response packet received not in"
			       "transaction log\n");
			rmap_erase_packet(rp);
			continue;
		}

		if (rp->data_len)
			memcpy(local_addr, rp->data, rp->data_len);

		trans_log_release_slot(rp->tr_id);
		rmap_erase_packet(rp);
	}

	return cnt;
}


/**
 * @brief submit an rmap command transaction
 *
 * @param cmd the rmap command
 * @param cmd_size the size of the rmap command
 * @param data the payload (may be NULL)
 * @param data_size the size of the payload
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_submit_tx(uint8_t *cmd,  int cmd_size,
			const uint8_t *data, int data_size)
{
	/* try to process pending responses */
	smile_fee_process_rx();

	if (!rmap_tx)
		return -1;

	if (0)
		printf("Transmitting RMAP command\n");

	if (rmap_tx(cmd, cmd_size, dpath_len, data, data_size)) {
		printf("rmap_tx() returned error!");
		return -1;
	}

	return 0;
}


/**
 * @brief generate an rmap command packet
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param rmap_cmd_type the rmap command type of the packet
 *
 * @param addr the address to read from or write to
 *
 * @param size the number of bytes to read or write
 *
 * @returns the size of the command data buffer or 0 on error
 */

int smile_fee_gen_cmd(uint16_t trans_id, uint8_t *cmd,
		      uint8_t rmap_cmd_type,
		      uint32_t addr, uint32_t size)
{
	int n;

	struct rmap_pkt *pkt;

	pkt = rmap_create_packet();
	if (!pkt) {
		printf("Error creating packet\n");
		return 0;
	}

	rmap_set_dst(pkt, smile_fee_addr);
	rmap_set_src(pkt, icu_addr);
	rmap_set_dest_path(pkt, dpath, dpath_len);
	rmap_set_reply_path(pkt, rpath, rpath_len);
	rmap_set_key(pkt, dst_key);
	rmap_set_cmd(pkt, rmap_cmd_type);
	rmap_set_tr_id(pkt, trans_id);
	rmap_set_data_addr(pkt, addr);
	rmap_set_data_len(pkt, size);

	/* determine header size */
	n = rmap_build_hdr(pkt, NULL);

	if (!cmd) {
		rmap_erase_packet(pkt);
		return n;
	}

	bzero(cmd, n);

	n = rmap_build_hdr(pkt, cmd);

	rmap_erase_packet(pkt);

	return n;
}





/**
 * @brief submit a sync command
 *
 * @param fn the FEE command generation function
 * @param addr the local address of the corresponding remote address
 * @param data_len the length of the data payload (0 for read commands)
 *
 * @return 0 on success, otherwise error
 */


int smile_fee_sync(int (*fn)(uint16_t trans_id, uint8_t *cmd),
		   void *addr, int data_len)
{
	int n;
	int slot;

	uint8_t *rmap_cmd;



	slot = trans_log_grab_slot(addr);
	if (slot < 0)
		return -1;


	/* determine size of command */
	n = fn(slot, NULL);

	rmap_cmd = (uint8_t *) malloc(n);
	if (!rmap_cmd) {
		printf("Error allocating rmap cmd");
		return -1;
	}

	/* now fill actual command */
	n = fn(slot, rmap_cmd);
	if (!n) {
		printf("Error creating command packet\n");
		free(rmap_cmd);
		return -1;
	}

	n = smile_fee_submit_tx(rmap_cmd, n, addr, data_len);
	free(rmap_cmd);

	return n;
}



/**
 * @brief submit a data sync command
 *
 * @param fn a FEE data transfer generation function
 * @param addr the remote address
 * @param data the local data address
 * @param data_len the length of the data payload
 * @param read 0: write, otherwise read
 *
 * @return 0 on success, < 0: error, > 0: retry
 *
 * @note this one is a little redundant, but otherwise we'd have a lot of
 *	 unused parameters on most of the control functions
 *
 * XXX need a paramter for read...meh...must think of something else
 */


int smile_fee_sync_data(int (*fn)(uint16_t trans_id, uint8_t *cmd,
				  uint32_t addr, uint32_t data_len),
			uint32_t addr, void *data, uint32_t data_len, int read)
{
	int n;
	int slot;

	uint8_t *rmap_cmd;



	smile_fee_process_rx();

	slot = trans_log_grab_slot(data);
	if (slot < 0) {
		if (0)
			printf("Error: all slots busy!\n");
		return 1;
	}


	/* determine size of command */
	n = fn(slot, NULL, addr, data_len);

	rmap_cmd = (uint8_t *) malloc(n);
	if (!rmap_cmd) {
		printf("Error allocating rmap cmd");
		return -1;
	}

	/* now fill actual command */
	n = fn(slot, rmap_cmd, addr, data_len);
	if (!n) {
		printf("Error creating command packet\n");
		free(rmap_cmd);
		return -1;
	}

	if (read)
		n = smile_fee_submit_tx(rmap_cmd, n, NULL, 0);
	else
		n = smile_fee_submit_tx(rmap_cmd, n, data, data_len);

	free(rmap_cmd);

	return n;
}



/**
 * @brief create a complete package from header and payload data including CRC8
 *
 * @note this is a helper function to generate complete binary RMAP packet dumps
 *
 * @param blob the blob buffer; if NULL, the function returns the needed size
 *
 * @param[in]  cmd an rmap command buffer
 * @param[in]  cmd_size the size of the rmap command buffer
 * @param[in]  non_crc_bytes leading bytes in the header not path of the CRC
 * @param[in]  data a data buffer (may be NULL)
 * @param[in]  data_size the size of the data buffer (ignored if data is NULL)
 *
 * @returns the size of the blob or 0 on error
 */

int smile_fee_package(uint8_t *blob,
		      uint8_t *cmd,  int cmd_size,
		      const uint8_t non_crc_bytes,
		      const uint8_t *data, int data_size)
{
	int n;
	int has_data_crc = 0;
	struct rmap_instruction *ri;



	if (!cmd_size) {
		blob = NULL;
		return 0;
	}


	/* allocate space for header, header crc, data, data crc */
	n = cmd_size + 1;

	ri = (struct rmap_instruction *) &cmd[non_crc_bytes + RMAP_INSTRUCTION];

	/* see if the type of command needs a data crc field at the end */
	switch (ri->cmd) {
	case RMAP_READ_MODIFY_WRITE_ADDR_INC:
	case RMAP_WRITE_ADDR_SINGLE:
	case RMAP_WRITE_ADDR_INC:
	case RMAP_WRITE_ADDR_SINGLE_VERIFY:
	case RMAP_WRITE_ADDR_INC_VERIFY:
		has_data_crc = 1;
		n += 1;
		break;
	default:
		break;
	}


	if (data)
		n += data_size;

	if (!blob)
		return n;


	memcpy(&blob[0], cmd, cmd_size);

	blob[cmd_size] = rmap_crc8(&cmd[non_crc_bytes],
				   cmd_size - non_crc_bytes);

	if (data) {
		memcpy(&blob[cmd_size + 1], data, data_size);
		blob[cmd_size + 1 + data_size] = rmap_crc8(data, data_size);
	} else {
		/* if no data is present, data crc is 0x0 */
		if (has_data_crc)
			blob[cmd_size + 1] = 0x0;
	}


	return n;
}


/**
 * @brief sets the logical address of the FEE
 * @param addr the address
 */

void smile_fee_set_destination_logical_address(uint8_t addr)
{
	smile_fee_addr = addr;
}

/**
 * @brief sets the logical address of the ICU
 * @param addr the address
 */

void smile_fee_set_source_logical_address(uint8_t addr)
{
	icu_addr = addr;
}


/**
 * @brief set the destination path to the FEE
 * @param path a byte array containing the path (may be NULL)
 * @param len the number of elements in the array
 *
 * @returns 0 on success, otherwise error
 *
 * @note the path array is taken as a reference, make sure to keep it around
 *	 the maximum length of the path is 15 elements
 *	 setting either path NULL or len 0 disables destination path addressing
 */

int smile_fee_set_destination_path(uint8_t *path, uint8_t len)
{
	if (len > RMAP_MAX_PATH_LEN)
		return -1;

	if (!path || !len) {
		dpath     = NULL;
		dpath_len = 0;
		return 0;
	}

	dpath     = path;
	dpath_len = len;

	return 0;
}


/**
 * @brief set the return path to the ICU
 * @param path a byte array containing the path (may be NULL)
 * @param len the number of elements in the array
 *
 * @returns 0 on success, otherwise error
 *
 * @note the path array is taken as a reference, make sure to keep it around
 *	 the maximum length of the path is 12 elements
 *	 the number of elements must be a multiple of 4 (due to RMAP protocol)
 *	 setting either path NULL or len 0 disables return path addressing
 */

int smile_fee_set_return_path(uint8_t *path, uint8_t len)
{
	if (len > RMAP_MAX_REPLY_PATH_LEN)
		return -1;

	if (!path || !len) {
		rpath     = NULL;
		rpath_len = 0;
		return 0;
	}

	rpath     = path;
	rpath_len = len;

	return 0;
}


/**
 * @brief set the destination command key to use
 *
 * @param key the destination key
 */

void smile_fee_set_destination_key(uint8_t key)
{
	dst_key = key;
}


/**
 * @brief get the FEE <-> ICU mirror RMAP synchronisation status
 *
 * @returns 0: synchronised, > 0: operations pending
 */

int smile_fee_rmap_sync_status(void)
{
	/* try to process pending responses */
	smile_fee_process_rx();

	return trans_log.pending;
}


/**
 * @brief reset all entries in the RMAP transaction log
 */

void smile_fee_rmap_reset_log(void)
{
	bzero(trans_log.in_use, TRANS_LOG_SIZE);
	trans_log.pending = 0;
}


/**
 * @brief initialise the fee control library
 *
 * @param mtu the maximum data transfer size per unit
 *
 * @param rmap_tx a function pointer to transmit an rmap command
 * @param rmap_rx function pointer to receive an rmap command
 *
 * @note rmap_tx is expected to return 0 on success
 *	 rmap_rx is expected to return the number of packet bytes
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_rmap_init(int mtu,
			int32_t (*tx)(void *hdr,  uint32_t hdr_size,
				      const uint8_t non_crc_bytes,
				      const void *data, uint32_t data_size),
			uint32_t (*rx)(uint8_t *pkt))
{
	if (!tx)
		return -1;

	if (!rx)
		return -1;

	rmap_tx = tx;
	rmap_rx = rx;

	data_mtu = mtu;

	return 0;
}
