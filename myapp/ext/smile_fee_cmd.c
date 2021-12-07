/**
 * @file   fee_cmd.c
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
 * @brief SMILE FEE RMAP command library
 * @see SMILE FEE to DPU Interface Requirements Document MSSL-SMILE-SXI-IRD-0001
 *
 * @note I realise that the actual register accessors are somewhat redundant
 *	 since they only reference the registers by a numeric id and could be
 *	 converted into a single function in principle. This file is however
 *	 an adaptation from code for the PLATO RDUC, so this was the easiest
 *	 approach, as I might create a more generic lower/intermediate
 *	 (i.e. rmap command generation and transaction control) library,
 *	 since the methods used here and for PLATO are IMO the smartest way
 *	 to approach remote instrument RMAP control. This does however require
 *	 a specific call interface, where I don't want va_args, so I can't just
 *	 add another parameter.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rmap.h>
#include <smile_fee_cmd.h>
#include <smile_fee_rmap.h>


/**
 * @brief generate a read command for an arbitrary register (internal)
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the register address
 *
 * @note this will configure a multi-address, 4 byte wide read command, because
 *	 the IWF RMAP core does not support single address read commands
 */

static int fee_read_cmd_register_internal(uint16_t trans_id, uint8_t *cmd,
					   uint32_t addr)
{
	return smile_fee_gen_cmd(trans_id, cmd, RMAP_READ_ADDR_INC, addr, 4);
}


/**
 * @brief generate a write command for an arbitrary register (internal)
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the register address
 *
 * @returns the size of the command data buffer or 0 on error
 *
 * @note this will configure a multi-address, 4 byte wide write command, because
 *	 the IWF RMAP core does not support single address write commands with
 *	 reply and CRC check enabled
 */

static int fee_write_cmd_register_internal(uint16_t trans_id, uint8_t *cmd,
					    uint32_t addr)
{
	return smile_fee_gen_cmd(trans_id, cmd, RMAP_WRITE_ADDR_INC_VERIFY_REPLY,
			    addr, 4);
}


/**
 * @brief create a command to write arbitrary data to the RDCU (internal)
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the address to write to
 * @param size the number of bytes to write
 *
 * @returns the size of the command data buffer or 0 on error
 *
 * @note this will configure a multi-address write command with reply enabled
 */

static int fee_write_cmd_data_internal(uint16_t trans_id, uint8_t *cmd,
					uint32_t addr, uint32_t size)
{
	return smile_fee_gen_cmd(trans_id, cmd, RMAP_WRITE_ADDR_INC_REPLY,
			    addr, size);
}


/**
 * @brief create a command to read arbitrary data to the RDCU (internal)
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the address to read from
 * @param size the number of bytes to read
 *
 * @returns the size of the command data buffer or 0 on error
 *
 * @note this will configure a multi-address read command
 *
 */

static int fee_read_cmd_data_internal(uint16_t trans_id, uint8_t *cmd,
				       uint32_t addr, uint32_t size)
{
	return smile_fee_gen_cmd(trans_id, cmd,
			    RMAP_READ_ADDR_INC, addr, size);
}


/**
 * @brief create a command to write arbitrary data to the RDCU
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the address to write to
 * @param size the number of bytes to write
 *
 * @returns the size of the command data buffer or 0 on error
 *
 * @note this will configure a multi-address write command with reply enabled
 */

int fee_write_cmd_data(uint16_t trans_id, uint8_t *cmd,
			uint32_t addr, uint32_t size)
{
	return fee_write_cmd_data_internal(trans_id, cmd, addr, size);
}


/**
 *
 * @brief create a command to read arbitrary data to the RDCU
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the address to read from
 * @param size the number of bytes to read
 *
 * @returns the size of the command data buffer or 0 on error
 *
 * @note this will configure a multi-address read command
 *
 */

int fee_read_cmd_data(uint16_t trans_id, uint8_t *cmd,
		       uint32_t addr, uint32_t size)
{
	return fee_read_cmd_data_internal(trans_id, cmd, addr, size);
}




/**
 * @brief generate a read command for an arbitrary register
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the register address
 *
 * @note this will configure a single address, 4 byte wide read command
 */

int fee_read_cmd_register(uint16_t trans_id, uint8_t *cmd, uint32_t addr)
{
	return fee_read_cmd_register_internal(trans_id, cmd, addr);
}


/**
 * @brief generate a write command for an arbitrary register
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @param addr the register address
 *
 * @returns the size of the command data buffer or 0 on error
 *
 * @note this will configure a single address, 4 byte wide write command with
 *	 reply and CRC check enabled
 */

int fee_write_cmd_register(uint16_t trans_id, uint8_t *cmd, uint32_t addr)
{
	return fee_write_cmd_register_internal(trans_id, cmd, addr);
}


/**
 * @brief create a command to read the FEE configuration register 0
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_0(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_0);
}


/**
 * @brief create a command to read the FEE configuration register 1
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_1(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_1);
}


/**
 * @brief create a command to read the FEE configuration register 2
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_2(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_2);
}


/**
 * @brief create a command to read the FEE configuration register 3
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_3(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_3);
}


/**
 * @brief create a command to read the FEE configuration register 4
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_4(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_4);
}


/**
 * @brief create a command to read the FEE configuration register 5
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_5(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_5);
}


/**
 * @brief create a command to read the FEE configuration register 18
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_18(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_18);
}


/**
 * @brief create a command to read the FEE configuration register 19
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_19(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_19);
}


/**
 * @brief create a command to read the FEE configuration register 20
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_20(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_20);
}


/**
 * @brief create a command to read the FEE configuration register 21
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_21(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_21);
}


/**
 * @brief create a command to read the FEE configuration register 22
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_22(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_22);
}


/**
 * @brief create a command to read the FEE configuration register 24
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_read_cmd_cfg_reg_24(uint16_t trans_id, uint8_t *cmd)
{
	return fee_read_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_24);
}


/**
 * @brief create a command to write the FEE configuration register 0
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_0(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_0);
}


/**
 * @brief create a command to write the FEE configuration register 1
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_1(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_1);
}


/**
 * @brief create a command to write the FEE configuration register 2
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_2(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_2);
}


/**
 * @brief create a command to write the FEE configuration register 3
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_3(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_3);
}


/**
 * @brief create a command to write the FEE configuration register 4
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_4(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_4);
}


/**
 * @brief create a command to write the FEE configuration register 5
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_5(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_5);
}


/**
 * @brief create a command to write the FEE configuration register 18
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_18(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_18);
}


/**
 * @brief create a command to write the FEE configuration register 19
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_19(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_19);
}


/**
 * @brief create a command to write the FEE configuration register 20
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_20(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_20);
}


/**
 * @brief create a command to write the FEE configuration register 21
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_21(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_21);
}


/**
 * @brief create a command to write the FEE configuration register 22
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_22(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_22);
}


/**
 * @brief create a command to write the FEE configuration register 24
 *
 * @param trans_id a transaction identifier
 *
 * @param cmd the command buffer; if NULL, the function returns the needed size
 *
 * @returns the size of the command data buffer or 0 on error
 */

int fee_write_cmd_cfg_reg_24(uint16_t trans_id, uint8_t *cmd)
{
	return fee_write_cmd_register_internal(trans_id, cmd, FEE_CFG_REG_24);
}
