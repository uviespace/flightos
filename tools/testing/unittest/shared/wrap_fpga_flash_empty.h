/**
 * @file   wrap_fpga_flash_empty.h
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2015
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
 */

#ifndef WRAP_FPGA_FLASH_BLOCK_EMPTY
#define WRAP_FPGA_FLASH_BLOCK_EMPTY

#include <shared.h>
#include <stdint.h>
#include <iwf_fpga.h>

enum wrap_status fpga_flash_empty_wrapper(enum wrap_status ws);

int32_t __real_fpga_flash_empty(uint32_t unit, uint32_t block);

int32_t __wrap_fpga_flash_empty(uint32_t unit, uint32_t block);



#endif
