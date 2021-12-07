/**
 * @file   smile_fee_rmap.h
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
 */

#ifndef _SMILE_FEE_RMAP_H_
#define _SMILE_FEE_RMAP_H_

#include <stdint.h>



int smile_fee_submit_tx(uint8_t *cmd,  int cmd_size,
			const uint8_t *data, int data_size);


int smile_fee_gen_cmd(uint16_t trans_id, uint8_t *cmd,
		      uint8_t rmap_cmd_type,
		      uint32_t addr, uint32_t size);

int smile_fee_sync(int (*fn)(uint16_t trans_id, uint8_t *cmd),
		   void *addr, int data_len);

int smile_fee_sync_data(int (*fn)(uint16_t trans_id, uint8_t *cmd,
				  uint32_t addr, uint32_t data_len),
			uint32_t addr, void *data, uint32_t data_len, int read);

int smile_fee_package(uint8_t *blob,
		      uint8_t *cmd,  int cmd_size,
		      const uint8_t non_crc_bytes,
		      const uint8_t *data, int data_size);

void smile_fee_set_destination_logical_address(uint8_t addr);

int smile_fee_set_destination_path(uint8_t *path, uint8_t len);
int smile_fee_set_return_path(uint8_t *path, uint8_t len);
void smile_fee_set_source_logical_address(uint8_t addr);
void smile_fee_set_destination_key(uint8_t key);

int smile_fee_rmap_sync_status(void);

void smile_fee_rmap_reset_log(void);

int smile_fee_rmap_init(int mtu,
			int32_t (*tx)(void *hdr,  uint32_t hdr_size,
				      const uint8_t non_crc_bytes,
				      const void *data, uint32_t data_size),
			uint32_t (*rx)(uint8_t *pkt));



#endif /* _SMILE_FEE_RMAP_H_ */
