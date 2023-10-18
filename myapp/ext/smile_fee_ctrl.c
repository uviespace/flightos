/**
 * @file   smile_fee_ctrl.c
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
 * @brief RMAP SMILE FEE control library
 * @see SMILE-MSSL-PL-Register_map_v0.10_Draft
 *
 * USAGE:
 *
 *	Access to the local mirror is provided by _get or _set calls, so
 *	to configure a register in the SMILE_FEE, one would call the sequence:
 *
 *	set_register_xyz(someargument);
 *	sync_register_xyz_to_fee();
 *
 *	while(smile_fee_sync_status())
 *		wait_busy_or_do_something_else_your_choice();
 *
 *	[sync_comple]
 *
 *	and to read a register:
 *
 *	sync_register_uvw_to_dpu()
 *
 *	while(smile_fee_sync_status())
 *		wait_busy_or_do_something_else_your_choice();
 *
 *	[sync_comple]
 *
 *	value = get_register_uvw();
 *
 *
 * WARNING: This has not been thoroughly tested and is in need of inspection
 *	    with regards to the specification, in order to locate any
 *	    register transcription errors or typos.
 *	    Note that the FEE register layout may have been changed during the
 *	    development process. Make sure to inspect the latest register
 *	    map for changes.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rmap.h>
#include <smile_fee_cmd.h>
#include <smile_fee_ctrl.h>
#include <smile_fee_rmap.h>


static struct smile_fee_mirror *smile_fee;



/**
 * @brief get the start of vertical row shared with charge injection
 */

uint16_t smile_fee_get_vstart(void)
{
	return (uint16_t) (smile_fee->cfg_reg_0 & 0xFFFFUL);
}


/**
 * @brief set the start of vertical row shared with charge injection
 */

void smile_fee_set_vstart(uint16_t vstart)
{
	smile_fee->cfg_reg_0 &= ~0xFFFFUL;
	smile_fee->cfg_reg_0 |=  0xFFFFUL & ((uint32_t) vstart);
}

/**
 * @brief get the end of vertical row with charge injection
 */

uint16_t smile_fee_get_vend(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_0 >> 16) & 0xFFFFUL);
}


/**
 * @brief set the end of vertical row with charge injection
 */

void smile_fee_set_vend(uint16_t vend)
{
	smile_fee->cfg_reg_0 &= ~(0xFFFFUL << 16);
	smile_fee->cfg_reg_0 |=  (0xFFFFUL & ((uint32_t) vend)) << 16;
}


/**
 * @brief get the charge injection width
 */

uint16_t smile_fee_get_charge_injection_width(void)
{
	return (uint16_t) (smile_fee->cfg_reg_1 & 0xFFFFUL);
}


/**
 * @brief set the charge injection width
 */

void smile_fee_set_charge_injection_width(uint16_t width)
{
	smile_fee->cfg_reg_1 &= ~0xFFFFUL;
	smile_fee->cfg_reg_1 |=  0xFFFFUL & ((uint32_t) width);
}


/**
 * @brief get the charge injection gap
 */

uint16_t smile_fee_get_charge_injection_gap(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_1 >> 16) & 0xFFFFUL);
}


/**
 * @brief set the charge injection gap
 */

void smile_fee_set_charge_injection_gap(uint16_t gap)
{
	smile_fee->cfg_reg_1 &= ~(0xFFFFUL << 16);
	smile_fee->cfg_reg_1 |=  (0xFFFFUL & ((uint32_t) gap)) << 16;
}

/**
 * @brief get the duration of a parallel overlap period (TOI)
 */

uint16_t smile_fee_get_parallel_toi_period(void)
{
	return (uint16_t) (smile_fee->cfg_reg_2 & 0xFFFUL);
}


/**
 * @brief set the duration of a parallel overlap period (TOI)
 */

void smile_fee_set_parallel_toi_period(uint16_t period)
{
	smile_fee->cfg_reg_2 &= ~0xFFFUL;
	smile_fee->cfg_reg_2 |=  0xFFFUL & ((uint32_t) period);
}


/**
 * @brief get the extra parallel clock overlap
 */

uint16_t smile_fee_get_parallel_clk_overlap(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_2 >> 12) & 0xFFFUL);
}


/**
 * @brief set the extra parallel clock overlap
 */

void smile_fee_set_parallel_clk_overlap(uint16_t overlap)
{
	smile_fee->cfg_reg_2 &= ~(0xFFFUL << 12);
	smile_fee->cfg_reg_2 |=  (0xFFFUL & ((uint32_t) overlap)) << 12;
}


/**
 * @brief get CCD read-out
 *
 * @param ccd_id the id of the CCD (1 == CCD1 , 2 == CCD2, all others reserved)
 *
 * @note 1 indicates that CCD is read-out
 *
 * @returns 1 if ccd is read-out, 0 otherwise; always returns 0 if reserved
 *	    CCD ID is issued
 */

uint32_t smile_fee_get_ccd_readout(uint32_t ccd_id)
{
	if (!ccd_id)
		return 0;

	if (ccd_id > 2)
		return 0;

	/* note: bit index starts at 0, but ccd id starts at 1, hence the
	 * subtraction
	 */
	return (smile_fee->cfg_reg_2 >> 24) & (1 << (ccd_id - 1));
}


/**
 * @brief set CCD read-out
 *
 * @param ccd_id the id of the CCD (1 == CCD1 , 2 == CCD2, all others reserved)
 * @param status the read-out status to set, either 0 or any bit set
 *
 * @note 1 indicates that CCD is read-out
 *
 */

void smile_fee_set_ccd_readout(uint32_t ccd_id, uint32_t status)
{
	if (!ccd_id)
		return;

	if (ccd_id > 2)
		return;

	if (status)
		status = 1;

	/* note: bit index starts at 0, but ccd id starts at 1, hence the
	 * subtraction
	 */

	smile_fee->cfg_reg_2 &= ~((status << (ccd_id - 1)) << 24);
	smile_fee->cfg_reg_2 |=  ((status << (ccd_id - 1)) << 24);
}


/**
 * @brief get the amount of lines to be dumped after readout
 */

uint16_t smile_fee_get_n_final_dump(void)
{
	return (uint16_t) (smile_fee->cfg_reg_3 & 0xFFFFUL);
}


/**
 * @brief set the amount of lines to be dumped after readout
 */

void smile_fee_set_n_final_dump(uint16_t lines)
{
	smile_fee->cfg_reg_3 &= ~0xFFFFUL;
	smile_fee->cfg_reg_3 |=  0xFFFFUL & ((uint32_t) lines);
}


/**
 * @brief get the amount of serial register transfers
 */

uint16_t smile_fee_get_h_end(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_3 >> 16) & 0xFFFUL);
}


/**
 * @brief set the amount of serial register transfers
 */

void smile_fee_set_h_end(uint16_t transfers)
{
	smile_fee->cfg_reg_2 &= ~(0xFFFUL << 16);
	smile_fee->cfg_reg_2 |=  (0xFFFUL & ((uint32_t) transfers)) << 16;
}


/**
 * @brief get charge injection mode
 *
 * @returns 0 if disabled, bits set otherwise
 */

uint32_t smile_fee_get_charge_injection(void)
{
	return (smile_fee->cfg_reg_3 >> 28) & 0x1UL;
}


/**
 * @brief set the charge injection mode
 *
 * @param mode enable/disable injection; either 0 to disable or any bit set to
 *	       enable
 */

void smile_fee_set_charge_injection(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_3 &= ~(0x1UL << 28);
	smile_fee->cfg_reg_3 |=  (mode << 28);
}


/**
 * @brief get parallel clock generation
 *
 * @returns 1 if bi-levels parallel clocks are generated, 0 if tri-level
 *	    parallel clocks are generated
 */

uint32_t smile_fee_get_tri_level_clk(void)
{
	return (smile_fee->cfg_reg_3 >> 29) & 0x1UL;
}


/**
 * @brief set parallel clock generation
 *
 * @param mode set 0 for tri-level parallel clock generation,
 *	       any bit set for bi-level parallel clock generation
 */

void smile_fee_set_tri_level_clk(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_3 &= ~(0x1UL << 29);
	smile_fee->cfg_reg_3 |=  (mode  << 29);
}


/**
 * @brief get image clock direction
 *
 * @returns 1 if reverse parallel clocks are generated, 0 if normal forward
 *	    parallel clocks are generated
 */

uint32_t smile_fee_get_img_clk_dir(void)
{
	return (smile_fee->cfg_reg_3 >> 30) & 0x1UL;
}


/**
 * @brief set image clock direction

 *
 * @param mode set 0 for normal forward parallel clock generation
 *	       any bit set for reverse parallel clock generation
 */

void smile_fee_set_img_clk_dir(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_3 &= ~(0x1UL << 30);
	smile_fee->cfg_reg_3 |=  (mode  << 30);
}


/**
 * @brief get serial clock direction
 *
 * @returns 1 if reverse serial clocks are generated, 0 if normal forward
 *	    serial clocks are generated
 */

uint32_t smile_fee_get_clk_dir(void)
{
	return (smile_fee->cfg_reg_3 >> 31) & 0x1UL;
}


/**
 * @brief set serial clock direction
 *
 * @param mode set 0 for normal forward serial clock generation
 *	       any bit set for reverse serial clock generation
 */

void smile_fee_set_clk_dir(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_3 &= ~(0x1UL << 31);
	smile_fee->cfg_reg_3 |=  (mode  << 31);
}


/**
 * @brief get packet size
 *
 * @returns 10 byte packed header + number of load bytes; number of load bytes
 *	    are multiple of 4.
 */

uint16_t smile_fee_get_packet_size(void)
{
	return (uint16_t) (smile_fee->cfg_reg_4 & 0xFFFFUL);
}

/**
 * @brief set packet size
 *
 * @param pkt_size 10 byte packed header + number of load bytes; number of load
 *		   bytes must be a multiple of 4
 */

void smile_fee_set_packet_size(uint16_t pkt_size)
{
	smile_fee->cfg_reg_4 &= ~0xFFFFUL;
	smile_fee->cfg_reg_4 |=  0xFFFFUL & ((uint32_t) pkt_size);
}


/**
 * @brief get the integration period
 */

uint16_t smile_fee_get_int_period(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_4 >> 16) & 0xFFFFUL);
}


/**
 * @brief set the integration period
 */

void smile_fee_set_int_period(uint16_t period)
{
	smile_fee->cfg_reg_4 &= ~(0xFFFFUL << 16);
	smile_fee->cfg_reg_4 |=  (0xFFFFUL & ((uint32_t) period)) << 16;
}


/**
 * @brief get internal mode sync
 *
 * @returns 1 if internal mode sync is enabvle, 0 if disabled
 */

uint32_t smile_fee_get_sync_sel(void)
{
	return (smile_fee->cfg_reg_5 >> 20) & 0x1UL;
}


/**
 * @brief set internal mode sync
 *
 * @param mode set 0 to disable internal mode sync
 *	       any bit set to enable internal mode sync
 */

void smile_fee_set_sync_sel(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_5 &= ~(0x1UL << 20);
	smile_fee->cfg_reg_5 |=  (mode  << 20);
}


/**
 * @brief get the readout node(s)
 *
 * @returns 0x1 if read-out via F-side
 *          0x2 if read-out via E-side
 *          0x3 if read-out via both F- and E-side
 */

uint16_t smile_fee_get_readout_nodes(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_5 >> 21) & 0x3UL);
}


/**
 * @brief set the readout node(s)
 *
 * @param nodes set 0x1 for read-out via F-side,
 *		    0x2 if read-out via E-side,
 *		    0x3 if read-out via both F- and E-side
 */

void smile_fee_set_readout_nodes(uint32_t nodes)
{
	if (!nodes)
		return;

	if (nodes > 3)
		return;

	smile_fee->cfg_reg_5 &= ~(0x3UL << 21);
	smile_fee->cfg_reg_5 |=  (0x3UL & nodes) << 21;
}


/**
 * @brief get digitise mode
 *
 * @returns 1 if digitise data is transferred to DPU during an image mode,
 *	      else image mode is executed but not pixel data is transferred
 */

uint32_t smile_fee_get_digitise(void)
{
	return (smile_fee->cfg_reg_5 >> 23) & 0x1UL;
}


/**
 * @brief set digitise mode
 *
 * @param mode set 0 to disable pixel data transfer to DPU
 *	       any bit set to enable pixel data transfer
 */

void smile_fee_set_digitise(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_5 &= ~(0x1UL << 23);
	smile_fee->cfg_reg_5 |=  (mode  << 23);
}


/**
 * @brief get correction bypass
 *
 * @returns 1 if no correction is applied,
 *	    else background correction is applied to the read-out pixel data
 */

uint32_t smile_fee_get_correction_bypass(void)
{
	return (smile_fee->cfg_reg_5 >> 24) & 0x1UL;
}


/**
 * @brief set correction bypass
 *
 * @param mode set 0 to enable pixel data background correction
 *	       any bit set to disable background correction
 */

void smile_fee_set_correction_bypass(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_5 &= ~(0x1UL << 24);
	smile_fee->cfg_reg_5 |=  (mode  << 24);
}


/**
 * @brief get vod_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_vod_config(void)
{
	return (uint16_t) (smile_fee->cfg_reg_18 & 0xFFFUL);
}


/**
 * @brief set vod_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_vod_config(uint16_t vod)
{
	smile_fee->cfg_reg_18 &= ~0xFFFUL;
	smile_fee->cfg_reg_18 |=  0xFFFUL & ((uint32_t) vod);
}


/**
 * @brief get ccd1_vrd_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_ccd1_vrd_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_18 >> 12) & 0xFFFUL);
}

/**
 * @brief set ccd1_vrd_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_ccd1_vrd_config(uint16_t vrd)
{
	smile_fee->cfg_reg_18 &= ~(0xFFFUL << 12);
	smile_fee->cfg_reg_18 |=  (0xFFFUL & ((uint32_t) vrd)) << 12;
}


/**
 * @brief get ccd2_vrd_config
 *
 * @note no description available in register map document
 *
 * @warn this is a case of extreme idiocy from a user's perspective.
 *	 make sure to sync both reg 18 and 19 to DPU before using this function
 */

uint16_t smile_fee_get_ccd2_vrd_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_18 >> 24) & 0xFFUL)
			| ((smile_fee->cfg_reg_19 & 0x3UL) << 8);
}

/**
 * @brief set ccd2_vrd_config
 *
 * @note no description available in register map document
 *
 * @warn see smile_fee_get_ccd2_vrd_config; make sure to sync both reg 18 and 19
 *	 to FEE after using this function
 */

void smile_fee_set_ccd2_vrd_config(uint16_t vrd)
{
	smile_fee->cfg_reg_18 &= ~(0xFFUL << 24);
	smile_fee->cfg_reg_18 |=  (0xFFUL & ((uint32_t) vrd)) << 24;

	smile_fee->cfg_reg_19 &= ~0x3UL;
	smile_fee->cfg_reg_19 |=  0x3UL & (((uint32_t) vrd) >> 8);
}


/**
 * @brief get ccd3_vrd_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_ccd3_vrd_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_19 >> 4) & 0xFFFUL);
}

/**
 * @brief set ccd3_vrd_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_ccd3_vrd_config(uint16_t vrd)
{
	smile_fee->cfg_reg_19 &= ~(0xFFFUL << 4);
	smile_fee->cfg_reg_19 |=  (0xFFFUL & ((uint32_t) vrd)) << 4;
}


/**
 * @brief get ccd4_vrd_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_ccd4_vrd_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_19 >> 16) & 0xFFFUL);
}


/**
 * @brief set ccd4_vrd_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_ccd4_vrd_config(uint16_t vrd)
{
	smile_fee->cfg_reg_19 &= ~(0xFFFUL << 16);
	smile_fee->cfg_reg_19 |=  (0xFFFUL & ((uint32_t) vrd)) << 16;
}


/**
 * @brief get ccd_vgd_config
 *
 * @note no description available in register map document
 *
 * @warn great, another word boundary overlap...
 *	 make sure to sync both reg 19 and 20 to DPU before using this function
 */

uint16_t smile_fee_get_ccd_vgd_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_19 >> 28) & 0x3UL)
			| ((smile_fee->cfg_reg_20 & 0xFFUL) << 3);
}

/**
 * @brief set ccd_vgd_config
 *
 * @note no description available in register map document
 *
 * @warn see smile_fee_get_ccd_vgd_config; make sure to sync both reg 19 and 20
 *	 to FEE after using this function
 */

void smile_fee_set_ccd_vgd_config(uint16_t vgd)
{
	smile_fee->cfg_reg_19 &= ~(0x3UL << 28);
	smile_fee->cfg_reg_19 |=  (0x3UL & ((uint32_t) vgd)) << 28;

	smile_fee->cfg_reg_20 &= ~0xFFUL;
	smile_fee->cfg_reg_20 |=  0xFFUL & (((uint32_t) vgd) >> 3);
}


/**
 * @brief get ccd_vog_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_ccd_vog_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_20 >> 8) & 0xFFFUL);
}


/**
 * @brief set ccd_vog_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_ccd_vog_config(uint16_t vog)
{
	smile_fee->cfg_reg_20 &= ~(0xFFFUL << 8);
	smile_fee->cfg_reg_20 |=  (0xFFFUL & ((uint32_t) vog)) << 8;
}


/**
 * @brief get ccd_ig_hi_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_ccd_ig_hi_config(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_20 >> 20) & 0xFFFUL);
}


/**
 * @brief set ccd_ig_hi_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_ccd_ig_hi_config(uint16_t cfg)
{
	smile_fee->cfg_reg_20 &= ~(0xFFFUL << 20);
	smile_fee->cfg_reg_20 |=  (0xFFFUL & ((uint32_t) cfg)) << 20;
}


/**
 * @brief get ccd_ig_lo_config
 *
 * @note no description available in register map document
 */

uint16_t smile_fee_get_ccd_ig_lo_config(void)
{
	return (uint16_t) (smile_fee->cfg_reg_21 & 0xFFFUL);
}


/**
 * @brief set ccd_ig_lo_config
 *
 * @note no description available in register map document
 */

void smile_fee_set_ccd_ig_lo_config(uint16_t cfg)
{
	smile_fee->cfg_reg_21 &= ~0xFFFUL;
	smile_fee->cfg_reg_21 |=  0xFFFUL & (uint32_t) cfg;
}


/**
 * @brief get start row number
 */

uint16_t smile_fee_get_h_start(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_21 >> 12) & 0xFFFUL);
}


/**
 * @brief set start row number
 */

void smile_fee_set_h_start(uint16_t row)
{
	smile_fee->cfg_reg_21 &= ~(0xFFFUL << 12);
	smile_fee->cfg_reg_21 |=  (0xFFFUL & ((uint32_t) row)) << 12;
}


/**
 * @brief get next mode of operation
 *
 * @note as per register map document, the following return values
 *	 are currently valid:
 *	 0x0	on mode
 *	 0x1	full frame pattern mode
 *	 0x4	stand-by mode
 *	 0xd	immediate on mode (this is a command, not a mode)
 *	 0xf	frame transfer mode
 *	 0xe	full frame mode
 */

uint8_t smile_fee_get_ccd_mode_config(void)
{
	return (uint8_t) ((smile_fee->cfg_reg_21 >> 24) & 0xFUL);
}


/**
 * @brief set next mode of operation
 *
 * @param mode the mode to set
 *
 * @note as per register map document, the following return values
 *	 are currently valid:
 *	 0x0	on mode
 *	 0x1	full frame pattern mode
 *	 0x4	stand-by mode
 *	 0xd	immediate on mode (this is a command, not a mode)
 *	 0xf	frame transfer mode
 *	 0xe	full frame mode
 *
 * @warn input parameter is not checked for validity
 */

void smile_fee_set_ccd_mode_config(uint8_t mode)
{
	smile_fee->cfg_reg_21 &= ~(0xFUL << 24);
	smile_fee->cfg_reg_21 |=  (0xFUL & ((uint32_t) mode)) << 24;
}


/**
 * @brief get degree of binning
 *
 *
 * @returns 0x1 if no binning
 *	    0x2 if 6x6 binning
 *	    0x3 if 24x24 binning
 */

uint8_t smile_fee_get_ccd_mode2_config(void)
{
	return (uint8_t) ((smile_fee->cfg_reg_21 >> 28) & 0x3UL);
}


/**
 * @brief set degree of binning
 *
 * @param mode the mode to set:
 *	       0x1 :  no binning
 *             0x2 : 6x6 binning
 *             0x3 : 24x24 binning
 */

void smile_fee_set_ccd_mode2_config(uint8_t mode)
{
	smile_fee->cfg_reg_21 &= ~(0x3UL << 28);
	smile_fee->cfg_reg_21 |=  (0x3UL & ((uint32_t) mode)) << 28;
}


/**
 * @brief get event detection execution
 *
 * @returns 1 if event detection is executed, 0 otherwise
 */

uint32_t smile_fee_get_event_detection(void)
{
	return (smile_fee->cfg_reg_21 >> 30) & 0x1UL;
}


/**
 * @brief set event detection execution
 *
 * @param mode set 0 to disable event detection
 *	       any bit set to enable event detection
 */

void smile_fee_set_event_detection(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_21 &= ~(0x1UL << 30);
	smile_fee->cfg_reg_21 |=  (mode  << 30);
}


/**
 * @brief get error flags clear
 *
 * @returns 1 if flag is set, 0 otherwise
 *
 */

uint32_t smile_fee_get_clear_error_flag(void)
{
	return (smile_fee->cfg_reg_21 >> 31) & 0x1UL;
}


/**
 * @brief set error flags clear
 *
 * @param mode set 0 to disable clear flags
 *	       any bit set to enable clear error flags
 *
 *
 * @warn when set to 1, all error flags generated by the FEE FPGA
 *	 for non-RMAP/SPW related functions are immediately cleared.
 *	 The bit is the reset by the FPGA to re-enable error flags
 *	 BE WARNED: if you set this flag to 1 locally, the FEE will clear all
 *	 flags everytime you sync this register from DPU -> FEE as long
 *	 as the bit is set in the local FEE mirror
 */

void smile_fee_set_clear_error_flag(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_21 &= ~(0x1UL << 31);
	smile_fee->cfg_reg_21 |=  (mode  << 31);
}


/**
 * @brief get ccd1 single pixel threshold
 *
 * @note this is the threshold above which a pixel may be considered a possible
 *	 soft X-ray event
 */

uint16_t smile_fee_get_ccd1_single_pixel_treshold(void)
{
	return (uint16_t) (smile_fee->cfg_reg_22 & 0xFFFUL);
}


/**
 * @brief set ccd1 single pixel threshold
 *
 * @param threshold the threshold above which a pixel may be considered
 *		    a possible soft X-ray event
 */

void smile_fee_set_ccd1_single_pixel_treshold(uint16_t threshold)
{
	smile_fee->cfg_reg_22 &= ~0xFFFUL;
	smile_fee->cfg_reg_22 |=  0xFFFUL & ((uint32_t) threshold);
}


/**
 * @brief get ccd2 single pixel threshold
 *
 * @note this is the threshold above which a pixel may be considered a possible
 *	 soft X-ray event
 */

uint16_t smile_fee_get_ccd2_single_pixel_treshold(void)
{
	return (uint16_t) ((smile_fee->cfg_reg_22 >> 16) & 0xFFFFUL);
}


/**
 * @brief set ccd2 single pixel threshold
 *
 * @param threshold the threshold above which a pixel may be considered
 *		    a possible soft X-ray event
 */

void smile_fee_set_ccd2_single_pixel_treshold(uint16_t threshold)
{
	smile_fee->cfg_reg_22 &= ~(0xFFFFUL << 16);
	smile_fee->cfg_reg_22 |=  (0xFFFFUL & ((uint32_t) threshold)) << 16;
}


/**
 * @brief get execute op flag
 *
 * @returns 1 if execute op flag is set, 0 otherwise
 *
 * @note if this flag is set, the set operational parameters are expedited
 * @warn the register map document does not detail any behaviour, i.e. whether
 *	 this flag clears itself or must be cleared by the user
 *
 */

uint32_t smile_fee_get_execute_op(void)
{
	return smile_fee->cfg_reg_24 & 0x1UL;
}


/**
 * @brief set execute op flag
 *
 * @param mode set 0 to disable execute op
 *	       any bit set to enable execute op
 *
 * @note if this flag is set, the set operational parameters are expedited
 * @warn the register map document does not detail any behaviour, i.e. whether
 *	 this flag clears itself or must be cleared by the user
 *	 ASSUME YOU HAVE TO CLEAR THIS EXPLICITLY BEFORE CHANGING PARAMETERS OR
 *	 EXECUTING A DPU->FEE SYNC
 */

void smile_fee_set_execute_op(uint32_t mode)
{
	if (mode)
		mode = 1;


	smile_fee->cfg_reg_24 &= ~0x1UL;
	smile_fee->cfg_reg_24 |= mode;
}


/**
 * @brief sync configuration register 0
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_0(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_0,
				      &smile_fee->cfg_reg_0, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_0,
				      &smile_fee->cfg_reg_0, 4);

	return -1;
}


/**
 * @brief sync configuration register 1
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_1(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_1,
				      &smile_fee->cfg_reg_1, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_1,
				      &smile_fee->cfg_reg_1, 4);

	return -1;
}


/**
 * @brief sync configuration register 2
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_2(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_2,
				      &smile_fee->cfg_reg_2, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_2,
				      &smile_fee->cfg_reg_2, 4);

	return -1;
}


/**
 * @brief sync configuration register 3
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_3(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_3,
				      &smile_fee->cfg_reg_3, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_3,
				      &smile_fee->cfg_reg_3, 4);

	return -1;
}


/**
 * @brief sync configuration register 4
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_4(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_4,
				      &smile_fee->cfg_reg_4, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_4,
				      &smile_fee->cfg_reg_4, 4);

	return -1;
}


/**
 * @brief sync configuration register 5
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_5(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_5,
				      &smile_fee->cfg_reg_5, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_5,
				      &smile_fee->cfg_reg_5, 4);

	return -1;
}


/**
 * @brief sync configuration register 18
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_18(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_18,
				      &smile_fee->cfg_reg_18, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_18,
				      &smile_fee->cfg_reg_18, 4);

	return -1;
}


/**
 * @brief sync configuration register 19
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_19(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_19,
				      &smile_fee->cfg_reg_19, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_19,
				      &smile_fee->cfg_reg_19, 4);

	return -1;
}


/**
 * @brief sync configuration register 20
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_20(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_20,
				      &smile_fee->cfg_reg_20, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_20,
				      &smile_fee->cfg_reg_20, 4);

	return -1;
}


/**
 * @brief sync configuration register 21
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_21(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_21,
				      &smile_fee->cfg_reg_21, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_21,
				      &smile_fee->cfg_reg_21, 4);

	return -1;
}


/**
 * @brief sync configuration register 22
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_22(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_22,
				      &smile_fee->cfg_reg_22, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_22,
				      &smile_fee->cfg_reg_22, 4);

	return -1;
}


/**
 * @brief sync configuration register 24
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */
int smile_fee_sync_cfg_reg_24(enum sync_direction dir)
{
	if (dir == FEE2DPU)
		return smile_fee_sync(fee_read_cmd_cfg_reg_24,
				      &smile_fee->cfg_reg_24, 0);

	if (dir == DPU2FEE)
		return smile_fee_sync(fee_write_cmd_cfg_reg_24,
				      &smile_fee->cfg_reg_24, 4);

	return -1;
}


/**
 * @brief sync register containing vstart
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_vstart(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_0(dir);
}


/**
 * @brief sync register containing vend
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_vend(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_0(dir);
}


/**
 * @brief sync register containing charge injection width
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_charge_injection_width(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_1(dir);
}

/**
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_charge_injection_gap(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_1(dir);
}


/**
 * @brief sync the duration of a parallel overlap period (TOI)
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_parallel_toi_period(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_2(dir);
}

/**
 * @brief sync the extra parallel clock overlap
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_parallel_clk_overlap(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_2(dir);
}


/**
 * @brief sync CCD read-out
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_readout(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_2(dir);
}


/**
 * @brief sync the amount of lines to be dumped after readout
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_n_final_dump(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_3(dir);
}


/**
 * @brief sync the amount of serial register transfers
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_h_end(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_3(dir);
}


/**
 * @brief sync charge injection mode
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_charge_injection(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_3(dir);
}


/**
 * @brief sync parallel clock generation
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_tri_level_clk(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_3(dir);
}


/**
 * @brief sync image clock direction
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_img_clk_dir(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_3(dir);
}


/**
 * @brief sync serial clock direction
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_clk_dir(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_3(dir);
}


/**
 * @brief sync packet size
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_packet_size(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_4(dir);
}


/**
 * @brief sync the integration period
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_int_period(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_4(dir);
}


/**
 * @brief sync internal mode sync
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_sync_sel(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_5(dir);
}


/**
 * @brief sync the readout node(s)
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_readout_nodes(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_5(dir);
}


/**
 * @brief sync digitise mode
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_digitise(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_5(dir);
}


/**
 * @brief sync correction bypass
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_correction_bypass(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_5(dir);
}


/**
 * @brief sync vod_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_vod_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_18(dir);
}


/**
 * @brief sync ccd1_vrd_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd1_vrd_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_18(dir);
}


/**
 * @brief sync ccd2_vrd_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd2_vrd_config(enum sync_direction dir)
{
	int ret;

	ret = smile_fee_sync_cfg_reg_18(dir);
	if (ret)
		return ret;

	return smile_fee_sync_cfg_reg_19(dir);
}


/**
 * @brief sync ccd3_vrd_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd3_vrd_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_19(dir);
}


/**
 * @brief sync ccd4_vrd_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd4_vrd_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_19(dir);
}


/**
 * @brief sync ccd_vgd_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_vgd_config(enum sync_direction dir)
{
	int ret;

	ret = smile_fee_sync_cfg_reg_19(dir);
	if (ret)
		return ret;

	return smile_fee_sync_cfg_reg_20(dir);
}


/**
 * @brief sync ccd_vog_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_vog_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_20(dir);
}


/**
 * @brief get ccd_ig_hi_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_ig_hi_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_20(dir);
}


/**
 * @brief sync ccd_ig_lo_config
 * @note no description available in register map document
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_ig_lo_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_21(dir);
}


/**
 * @brief sync start row number
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_h_start(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_21(dir);
}


/**
 * @brief sync next mode of operation
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_mode_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_21(dir);
}


/**
 * @brief sync degree of binning
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd_mode2_config(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_21(dir);
}


/**
 * @brief sync event detection execution
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_event_detection(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_21(dir);
}


/**
 * @brief sync error flags clear
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_clear_error_flag(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_21(dir);
}


/**
 * @brief sync ccd1 single pixel threshold
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd1_single_pixel_treshold(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_22(dir);
}


/**
 * @brief sync ccd2 single pixel threshold
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_ccd2_single_pixel_treshold(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_22(dir);
}


/**
 * @brief sync execute op flag
 *
 * @param dir the syncronisation direction
 *
 * @returns 0 on success, otherwise error
 */

int smile_fee_sync_execute_op(enum sync_direction dir)
{
	return smile_fee_sync_cfg_reg_24(dir);
}





/**
 * @brief initialise the smile_fee control library
 *
 * @param fee_mirror the desired FEE mirror, set NULL to allocate it for you
 */

void smile_fee_ctrl_init(struct smile_fee_mirror *fee_mirror)
{
	if (!fee_mirror)
		smile_fee = (struct smile_fee_mirror *)
				malloc(sizeof(struct smile_fee_mirror));
	else
		smile_fee = fee_mirror;

	if (!smile_fee) {
		printf("Error allocating memory for the SMILE_FEE mirror\n");
		return;
	}

	bzero(smile_fee, sizeof(struct smile_fee_mirror));
}
