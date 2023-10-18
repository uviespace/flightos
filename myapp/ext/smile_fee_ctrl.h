/**
 * @file   smile_fee_ctrl.h
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2020
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
 * @brief SMILE FEE control interface
 */
#ifndef _SMILE_FEE_CTRL_H_
#define _SMILE_FEE_CTRL_H_

#include <stdint.h>

__extension__
struct smile_fee_mirror {

	/* FEE RW registers (SMILE-MSSL-PL-Register_map_v0.10_Draft) */
	/* (includes unused) */
	uint32_t cfg_reg_0;
	uint32_t cfg_reg_1;
	uint32_t cfg_reg_2;
	uint32_t cfg_reg_3;
	uint32_t cfg_reg_4;
	uint32_t cfg_reg_5;
	uint32_t cfg_reg_6;
	uint32_t cfg_reg_7;
	uint32_t cfg_reg_8;
	uint32_t cfg_reg_9;
	uint32_t cfg_reg_10;
	uint32_t cfg_reg_11;
	uint32_t cfg_reg_12;
	uint32_t cfg_reg_13;
	uint32_t cfg_reg_14;
	uint32_t cfg_reg_15;
	uint32_t cfg_reg_16;
	uint32_t cfg_reg_17;
	uint32_t cfg_reg_18;
	uint32_t cfg_reg_19;
	uint32_t cfg_reg_20;
	uint32_t cfg_reg_21;
	uint32_t cfg_reg_22;
	uint32_t cfg_reg_23;
	uint32_t cfg_reg_24;

	/* FEE  RO registers (SMILE-MSSL-PL-Register_map_v0.10_Draft) */

	uint32_t unused[1696];	/* TODO */

	uint32_t hk_reg_0;
	uint32_t hk_reg_1;
	uint32_t hk_reg_2;
	uint32_t hk_reg_3;
	uint32_t hk_reg_4;
	uint32_t hk_reg_5;
	uint32_t hk_reg_6;
	uint32_t hk_reg_7;
	uint32_t hk_reg_8;
	uint32_t hk_reg_9;
	uint32_t hk_reg_10;
	uint32_t hk_reg_11;
	uint32_t hk_reg_12;
	uint32_t hk_reg_13;
	uint32_t hk_reg_14;
	uint32_t hk_reg_15;
	uint32_t hk_reg_16;
	uint32_t hk_reg_17;
	uint32_t hk_reg_18;
	uint32_t hk_reg_19;
	uint32_t hk_reg_20;
	uint32_t hk_reg_21;
	uint32_t hk_reg_22;
	uint32_t hk_reg_23;
	uint32_t hk_reg_24;
	uint32_t hk_reg_25;
	uint32_t hk_reg_26;
	uint32_t hk_reg_27;
	uint32_t hk_reg_28;
	uint32_t hk_reg_29;
	uint32_t hk_reg_30;
	uint32_t hk_reg_31;
	uint32_t hk_reg_32;
	uint32_t hk_reg_33;
	uint32_t hk_reg_34;
	uint32_t hk_reg_35;

	/* arbitaray ram area */
	/* uint8_t *sram; */
};

/* register sync */

/**
 * @note FEE2DPU is for "read" commands
 *	 DPU2FEE is for "write" commands
 */
enum sync_direction {FEE2DPU, DPU2FEE};

/* whole registers */
int smile_fee_sync_cfg_reg_0(enum sync_direction dir);
int smile_fee_sync_cfg_reg_1(enum sync_direction dir);
int smile_fee_sync_cfg_reg_2(enum sync_direction dir);
int smile_fee_sync_cfg_reg_3(enum sync_direction dir);
int smile_fee_sync_cfg_reg_4(enum sync_direction dir);
int smile_fee_sync_cfg_reg_5(enum sync_direction dir);
int smile_fee_sync_cfg_reg_18(enum sync_direction dir);
int smile_fee_sync_cfg_reg_19(enum sync_direction dir);
int smile_fee_sync_cfg_reg_20(enum sync_direction dir);
int smile_fee_sync_cfg_reg_21(enum sync_direction dir);
int smile_fee_sync_cfg_reg_22(enum sync_direction dir);
int smile_fee_sync_cfg_reg_24(enum sync_direction dir);

/* values contained in registers */
int smile_fee_sync_vstart(enum sync_direction dir);
int smile_fee_sync_vend(enum sync_direction dir);
int smile_fee_sync_charge_injection_width(enum sync_direction dir);
int smile_fee_sync_charge_injection_gap(enum sync_direction dir);
int smile_fee_sync_parallel_toi_period(enum sync_direction dir);
int smile_fee_sync_parallel_clk_overlap(enum sync_direction dir);
int smile_fee_sync_ccd_readout(enum sync_direction dir);
int smile_fee_sync_n_final_dump(enum sync_direction dir);
int smile_fee_sync_h_end(enum sync_direction dir);
int smile_fee_sync_charge_injection(enum sync_direction dir);
int smile_fee_sync_tri_level_clk(enum sync_direction dir);
int smile_fee_sync_img_clk_dir(enum sync_direction dir);
int smile_fee_sync_clk_dir(enum sync_direction dir);
int smile_fee_sync_packet_size(enum sync_direction dir);
int smile_fee_sync_int_period(enum sync_direction dir);
int smile_fee_sync_sync_sel(enum sync_direction dir);
int smile_fee_sync_readout_nodes(enum sync_direction dir);
int smile_fee_sync_digitise(enum sync_direction dir);
int smile_fee_sync_correction_bypass(enum sync_direction dir);
int smile_fee_sync_vod_config(enum sync_direction dir);
int smile_fee_sync_ccd1_vrd_config(enum sync_direction dir);
int smile_fee_sync_ccd2_vrd_config(enum sync_direction dir);
int smile_fee_sync_ccd3_vrd_config(enum sync_direction dir);
int smile_fee_sync_ccd4_vrd_config(enum sync_direction dir);
int smile_fee_sync_ccd_vgd_config(enum sync_direction dir);
int smile_fee_sync_ccd_vog_config(enum sync_direction dir);
int smile_fee_sync_ccd_ig_hi_config(enum sync_direction dir);
int smile_fee_sync_ccd_ig_lo_config(enum sync_direction dir);
int smile_fee_sync_h_start(enum sync_direction dir);
int smile_fee_sync_ccd_mode_config(enum sync_direction dir);
int smile_fee_sync_ccd_mode2_config(enum sync_direction dir);
int smile_fee_sync_event_detection(enum sync_direction dir);
int smile_fee_sync_clear_error_flag(enum sync_direction dir);
int smile_fee_sync_ccd1_single_pixel_treshold(enum sync_direction dir);
int smile_fee_sync_ccd2_single_pixel_treshold(enum sync_direction dir);
int smile_fee_sync_execute_op(enum sync_direction dir);



/* read SMILE_FEE register mirror */
uint16_t smile_fee_get_vstart(void);
uint16_t smile_fee_get_vend(void);
uint16_t smile_fee_get_charge_injection_width(void);
uint16_t smile_fee_get_charge_injection_gap(void);
uint16_t smile_fee_get_parallel_toi_period(void);
uint16_t smile_fee_get_parallel_clk_overlap(void);
uint32_t smile_fee_get_ccd_readout(uint32_t ccd_id);
uint16_t smile_fee_get_n_final_dump(void);
uint16_t smile_fee_get_h_end(void);
uint32_t smile_fee_get_charge_injection(void);
uint32_t smile_fee_get_tri_level_clk(void);
uint32_t smile_fee_get_img_clk_dir(void);
uint32_t smile_fee_get_clk_dir(void);
uint16_t smile_fee_get_packet_size(void);
uint16_t smile_fee_get_int_period(void);
uint32_t smile_fee_get_sync_sel(void);
uint16_t smile_fee_get_readout_nodes(void);
uint32_t smile_fee_get_digitise(void);
uint32_t smile_fee_get_correction_bypass(void);
uint16_t smile_fee_get_vod_config(void);
uint16_t smile_fee_get_ccd1_vrd_config(void);
uint16_t smile_fee_get_ccd2_vrd_config(void);
uint16_t smile_fee_get_ccd3_vrd_config(void);
uint16_t smile_fee_get_ccd4_vrd_config(void);
uint16_t smile_fee_get_ccd_vgd_config(void);
uint16_t smile_fee_get_ccd_vog_config(void);
uint16_t smile_fee_get_ccd_ig_hi_config(void);
uint16_t smile_fee_get_ccd_ig_lo_config(void);
uint16_t smile_fee_get_h_start(void);
uint8_t smile_fee_get_ccd_mode_config(void);
uint8_t smile_fee_get_ccd_mode2_config(void);
uint32_t smile_fee_get_event_detection(void);
uint32_t smile_fee_get_clear_error_flag(void);
uint16_t smile_fee_get_ccd1_single_pixel_treshold(void);
uint16_t smile_fee_get_ccd2_single_pixel_treshold(void);
uint32_t smile_fee_get_execute_op(void);



/* write SMILE_FEE register mirror */
void smile_fee_set_vstart(uint16_t vstart);
void smile_fee_set_vend(uint16_t vend);
void smile_fee_set_charge_injection_width(uint16_t width);
void smile_fee_set_charge_injection_gap(uint16_t width);
void smile_fee_set_parallel_toi_period(uint16_t period);
void smile_fee_set_parallel_clk_overlap(uint16_t overlap);
void smile_fee_set_ccd_readout(uint32_t ccd_id, uint32_t status);
void smile_fee_set_n_final_dump(uint16_t lines);
void smile_fee_set_h_end(uint16_t transfers);
void smile_fee_set_charge_injection(uint32_t mode);
void smile_fee_set_tri_level_clk(uint32_t mode);
void smile_fee_set_img_clk_dir(uint32_t mode);
void smile_fee_set_clk_dir(uint32_t mode);
void smile_fee_set_packet_size(uint16_t pkt_size);
void smile_fee_set_int_period(uint16_t period);
uint16_t smile_fee_get_int_period(void);
void smile_fee_set_readout_nodes(uint32_t nodes);
void smile_fee_set_digitise(uint32_t mode);
void smile_fee_set_correction_bypass(uint32_t mode);
void smile_fee_set_vod_config(uint16_t vod);
void smile_fee_set_ccd1_vrd_config(uint16_t vrd);
void smile_fee_set_ccd2_vrd_config(uint16_t vrd);
void smile_fee_set_ccd3_vrd_config(uint16_t vrd);
void smile_fee_set_ccd4_vrd_config(uint16_t vrd);
void smile_fee_set_ccd_vgd_config(uint16_t vgd);
void smile_fee_set_ccd_vog_config(uint16_t vog);
void smile_fee_set_ccd_ig_hi_config(uint16_t cfg);
void smile_fee_set_ccd_ig_lo_config(uint16_t cfg);
void smile_fee_set_h_start(uint16_t row);
void smile_fee_set_ccd_mode_config(uint8_t mode);
void smile_fee_set_ccd_mode2_config(uint8_t mode);
void smile_fee_set_event_detection(uint32_t mode);
void smile_fee_set_clear_error_flag(uint32_t mode);
void smile_fee_set_ccd1_single_pixel_treshold(uint16_t threshold);
void smile_fee_set_ccd2_single_pixel_treshold(uint16_t threshold);
void smile_fee_set_execute_op(uint32_t mode);



/* setup */
void smile_fee_ctrl_init(struct smile_fee_mirror *fee_mirror);


#endif /* _SMILE_FEE_CTRL_H_ */
