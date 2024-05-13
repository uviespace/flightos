/**
 * @file   leon3_memcfg.c
 * @ingroup leon3_memcfg
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date   March, 2016
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
 * @defgroup leon3_memcfg Memory Configuration Register access
 *
 * @brief Implements memory configuration register access functions for the
 *        GR712RC
 *
 * ## Overview
 *
 * This module implements functionality to access or modify the memory
 * configuration registers of the GR712RC.
 *
 * In addition, it provides functions to perform EDAC bypass reads and writes
 * for fault injection.
 *
 * @see GR712-UM v2.7 5.14.1 - 5.14.4
 *
 * ## Mode of Operation
 *
 * None
 *
 * ## Error Handling
 *
 * None
 *
 * ## Notes
 *
 * - functionality will be added as needed
 * - does not provide support for the FTAHB memory configuration register!
 *
 */


#include <asm/io.h>
#include <leon3_memcfg.h>
#include <asm/leon_reg.h>


/**
 * @brief set bits in the memory configuration register 1 by mask
 *
 * @param mask the bitmask to apply
 */

static void leon3_memcfg_set_mcfg1(uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg1);
	tmp |= mask;
	iowrite32be(tmp, &memctrl->mcfg1);

}


/**
 * @brief get bits in the memory configuration register 1 by mask
 *
 * @param mask the bitmask to apply
 */
__attribute__((unused))
static uint32_t leon3_memcfg_get_mcfg1(uint32_t mask)
{
	struct leon3_ftmctrl_registermap const *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	return ioread32be(&memctrl->mcfg1) & mask;
}


/**
 * @brief clear bits in the memory configuration register 1 by mask
 *
 * @param mask the bitmask to apply (bits to clear are HIGH!)
 */

static void leon3_memcfg_clear_mcfg1(uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg1);
	tmp &= ~mask;
	iowrite32be(tmp, &memctrl->mcfg1);

}


/**
 * @brief replace bits in the memory configuration register 1 by mask
 *
 * @param bits the bits to set
 * @param mask the bitmask used to clear the field
 */

static void leon3_memcfg_replace_mcfg1(uint32_t bits, uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg1);
	tmp &= ~mask;
	tmp |= bits;
	iowrite32be(tmp, &memctrl->mcfg1);

}


/**
 * @brief set bits in the memory configuration register 2 by mask
 *
 * @param mask the bitmask to apply
 */

static void leon3_memcfg_set_mcfg2(uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg2);
	tmp |= mask;
	iowrite32be(tmp, &memctrl->mcfg2);

}


/**
 * @brief get bits in the memory configuration register 2 by mask
 *
 * @param mask the bitmask to apply
 */

__attribute__((unused))
static uint32_t leon3_memcfg_get_mcfg2(uint32_t mask)
{
	struct leon3_ftmctrl_registermap const *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	return ioread32be(&memctrl->mcfg2) & mask;
}


/**
 * @brief clear bits in the memory configuration register 2 by mask
 *
 * @param mask the bitmask to apply (bits to clear are HIGH!)
 */

static void leon3_memcfg_clear_mcfg2(uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg2);
	tmp &= ~mask;
	iowrite32be(tmp, &memctrl->mcfg2);

}


/**
 * @brief replace bits in the memory configuration register 2 by mask
 *
 * @param bits the bits to set
 * @param mask the bitmask used to clear the field
 */

static void leon3_memcfg_replace_mcfg2(uint32_t bits, uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg2);
	tmp &= ~mask;
	tmp |= bits;
	iowrite32be(tmp, &memctrl->mcfg2);

}

/**
 * @brief set bits in the memory configuration register 3 by mask
 *
 * @param mask the bitmask to apply
 */

static void leon3_memcfg_set_mcfg3(uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg3);
	tmp |= mask;
	iowrite32be(tmp, &memctrl->mcfg3);

}


/**
 * @brief get bits in the memory configuration register 3 by mask
 *
 * @param mask the bitmask to apply
 */

static uint32_t leon3_memcfg_get_mcfg3(uint32_t mask)
{
	struct leon3_ftmctrl_registermap const *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	return ioread32be(&memctrl->mcfg3) & mask;
}


/**
 * @brief clear bits in the memory configuration register 3 by mask
 *
 * @param mask the bitmask to apply (bits to clear are HIGH!)
 */

static void leon3_memcfg_clear_mcfg3(uint32_t mask)
{
	uint32_t tmp;

	struct leon3_ftmctrl_registermap *memctrl =
		(struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;

	tmp  = ioread32be(&memctrl->mcfg3);
	tmp &= ~mask;
	iowrite32be(tmp, &memctrl->mcfg3);

}

/**
 * @brief set edac  test checkbits
 *
 * @param tcb the checkbits to set
 */

static void leon3_memcfg_set_checkbits(uint8_t tcb)
{
	leon3_memcfg_clear_mcfg3(LEON3_MEMCFG3_TCB);
	leon3_memcfg_set_mcfg3(((uint32_t) tcb) & LEON3_MEMCFG3_TCB);
}


/**
 * @brief get edac test checkbits
 *
 * @return the test checkbits
 */

static uint8_t leon3_memcfg_get_checkbits(void)
{
	return (uint8_t) leon3_memcfg_get_mcfg3(LEON3_MEMCFG3_TCB);
}



/**
 * @brief enable the prom edac
 */

void leon3_memcfg_enable_prom_edac(void)
{
	leon3_memcfg_set_mcfg3(LEON3_MEMCFG3_PROM_EDAC);
}


/**
 * @brief disable the prom edac
 */

void leon3_memcfg_disable_prom_edac(void)
{
	leon3_memcfg_clear_mcfg3(LEON3_MEMCFG3_PROM_EDAC);
}


/**
 * @brief enable the ram edac
 */

void leon3_memcfg_enable_ram_edac(void)
{
	leon3_memcfg_set_mcfg3(LEON3_MEMCFG3_RAM_EDAC);
}


/**
 * @brief disable the ram edac
 */

void leon3_memcfg_disable_ram_edac(void)
{
	leon3_memcfg_clear_mcfg3(LEON3_MEMCFG3_RAM_EDAC);
}


/**
 * @brief enable edac ram bypass read
 */

void leon3_memcfg_enable_edac_read_bypass(void)
{
	leon3_memcfg_set_mcfg3(LEON3_MEMCFG3_RB_EDAC);
}


/**
 * @brief disable edac ram bypass read
 */

void leon3_memcfg_disable_edac_read_bypass(void)
{
	leon3_memcfg_clear_mcfg3(LEON3_MEMCFG3_RB_EDAC);
}


/**
 * @brief enable edac ram bypass write
 */

void leon3_memcfg_enable_edac_write_bypass(void)
{
	leon3_memcfg_set_mcfg3(LEON3_MEMCFG3_WB_EDAC);
}


/**
 * @brief disable edac ram bypass write
 */

void leon3_memcfg_disable_edac_write_bypass(void)
{
	leon3_memcfg_clear_mcfg3(LEON3_MEMCFG3_WB_EDAC);
}


/**
 * @brief perform a bypass read and retrieve the checkbits
 *
 * @param addr the address pointer to read from
 * @param tcb checkbits storage
 *
 * @return data word at address
 */

uint32_t leon3_memcfg_bypass_read(void *addr, uint8_t *tcb)
{
	uint32_t val;

	leon3_memcfg_enable_edac_read_bypass();
	val = ioread32be(addr);
	(*tcb) = leon3_memcfg_get_checkbits();
	leon3_memcfg_disable_edac_read_bypass();

	return val;
}


/**
 * @brief set custom checkbits and perform a bypass write
 *
 * @param addr the address pointer to write to
 * @param value the data word to write
 * @param tcb the checkbits to set
 *
 * XXX note this will introduce extra errors if the MMU is enabled (the
 * context table entries will receive wrong checkbits),
 * since this mechanism causes checkbit updates for all write operations
 * on the bus; this means that this will also potentially cause edac errors
 * for any other transfer, such as by SpW or other IP cores
 *
 * disable the mmu if needed and use with great care
 *
 */

void leon3_memcfg_bypass_write(void *addr, uint32_t value, uint8_t tcb)
{
	leon3_memcfg_enable_edac_write_bypass();
	leon3_memcfg_set_checkbits(tcb);
	iowrite32be(value, addr);
	leon3_memcfg_disable_edac_write_bypass();
}


/**
 * @brief disable bus ready signalling for the prom area
 */

void leon3_memcfg_clear_prom_area_bus_ready(void)
{
	leon3_memcfg_clear_mcfg1(LEON3_MEMCFG1_PBRDY);
}


/**
 * @brief disable asynchronous bus ready
 */

void leon3_memcfg_clear_asynchronous_bus_ready(void)
{
	leon3_memcfg_clear_mcfg1(LEON3_MEMCFG1_ABRDY);
}


/**
 * @brief set IO bus width
 *
 * @note 0 (00) = 8 bits, 2 (01) = 32 bits
 */

void leon3_memcfg_set_io_bus_width(uint32_t bus_width)
{
	leon3_memcfg_replace_mcfg1((bus_width << LEON3_MEMCFG1_IOBUSW_OFF)
			     & LEON3_MEMCFG1_IOBUSW_MASK,
			     LEON3_MEMCFG1_IOBUSW_MASK);
}


/**
 * @brief disable bus ready signalling for IO area
 */

void leon3_memcfg_clear_io_bus_ready(void)
{
	leon3_memcfg_clear_mcfg1(LEON3_MEMCFG1_IBRDY);
}


/**
 * @brief disable bus error signalling
 */

void leon3_memcfg_clear_bus_error(void)
{
	leon3_memcfg_clear_mcfg1(LEON3_MEMCFG1_BEXCN);
}


/**
 * @brief set IO waitstates
 *
 * @note 0000 = 0, 0001 = 1,  0010 = 2,..., 1111 = 15
 */

void leon3_memcfg_set_io_waitstates(uint32_t wait_states)
{
	leon3_memcfg_replace_mcfg1((wait_states << LEON3_MEMCFG1_IO_WAITSTATES_OFF)
			     & LEON3_MEMCFG1_IO_WAITSTATES_MASK,
			     LEON3_MEMCFG1_IO_WAITSTATES_MASK);

}


/**
 * @brief enable access to memory IO bus area
 */

void leon3_memcfg_set_io(void)
{
	leon3_memcfg_set_mcfg1(LEON3_MEMCFG1_IOEN);
}


/**
 * @brief set PROM bank size
 *
 * @note 0000 = 256 MiB, all others: bank_size = lg2(kilobytes / 8)
 */

void leon3_memcfg_set_prom_bank_size(uint32_t bank_size)
{
	leon3_memcfg_replace_mcfg1((bank_size << LEON3_MEMCFG1_ROMBANKSZ_OFF)
			     & LEON3_MEMCFG1_ROMBANKSZ_MASK,
			     LEON3_MEMCFG1_ROMBANKSZ_MASK);
}


/**
 * @brief enable prom write cycles
 */

void leon3_memcfg_set_prom_write(void)
{
	leon3_memcfg_set_mcfg1(LEON3_MEMCFG1_PWEN);
}



/**
 * @brief set PROM data bus width
 *
 * @note 0 (00) = 8 bits, 2 (01) = 32 bits
 */

void leon3_memcfg_set_prom_width(uint32_t prom_width)
{
	leon3_memcfg_replace_mcfg1((prom_width << LEON3_MEMCFG1_PROM_WIDTH_OFF)
			     & LEON3_MEMCFG1_PROM_WIDTH_MASK,
			     LEON3_MEMCFG1_PROM_WIDTH_MASK);
}


/**
 * @brief set PROM write waitstates
 *
 * @note 0000 = 0, 0001 = 2, 0010 = 4,..., 1111 = 30
 */

void leon3_memcfg_set_prom_write_waitstates(uint32_t wait_states)
{
	leon3_memcfg_replace_mcfg1((wait_states << LEON3_MEMCFG1_PROM_WRITE_WS_OFF)
			     & LEON3_MEMCFG1_PROM_WRITE_WS_MASK,
			     LEON3_MEMCFG1_PROM_WRITE_WS_MASK);
}

/**
 * @brief set PROM read waitstates
 *
 * @note 0000 = 0, 0001 = 2, 0010 = 4,..., 1111 = 30
 */

void leon3_memcfg_set_prom_read_waitstates(uint32_t wait_states)
{
	leon3_memcfg_replace_mcfg1(wait_states & LEON3_MEMCFG1_PROM_READ_WS_MASK,
			     LEON3_MEMCFG1_PROM_READ_WS_MASK);
}


/**
 * @brief disable SDRAM controller
 */

void leon3_memcfg_clear_sdram_enable(void)
{
	leon3_memcfg_clear_mcfg2(LEON3_MEMCFG2_SE);
}


/**
 * @brief enable SRAM
 */

void leon3_memcfg_clear_sram_disable(void)
{
	leon3_memcfg_clear_mcfg2(LEON3_MEMCFG2_SI);
}


/**
 * @brief set RAM bank size
 *
 * @note bank_size = lg2(kilobytes / 8)
 */

void leon3_memcfg_set_ram_bank_size(uint32_t bank_size)
{
	leon3_memcfg_replace_mcfg2((bank_size << LEON3_MEMCFG2_RAM_BANK_SIZE_OFF)
			     & LEON3_MEMCFG2_RAM_BANK_SIZE_MASK,
			     LEON3_MEMCFG2_RAM_BANK_SIZE_MASK);
}


/**
 * @brief enable read-modify-write cycles
 */

void leon3_memcfg_set_read_modify_write(void)
{
	leon3_memcfg_set_mcfg2(LEON3_MEMCFG2_RMW);
}


/**
 * @brief set RAM width
 *
 * @note 00 = 8 bits, 1X = 32 bits
 */

void leon3_memcfg_set_ram_width(uint32_t ram_width)
{
	leon3_memcfg_replace_mcfg2((ram_width << LEON3_MEMCFG2_RAM_WIDTH_OFF)
			     & LEON3_MEMCFG2_RAM_WIDTH_MASK,
			     LEON3_MEMCFG2_RAM_WIDTH_MASK);
}



/**
 * @brief set RAM write waitstates
 *
 * @note 00 = 0, 01 = 1, 10 = 2, 11 = 3
 */

void leon3_memcfg_set_ram_write_waitstates(uint32_t wait_states)
{
	leon3_memcfg_replace_mcfg2((wait_states << LEON3_MEMCFG2_RAM_WRITE_WS_OFF)
			     & LEON3_MEMCFG2_RAM_WRITE_WS_MASK,
			     LEON3_MEMCFG2_RAM_WRITE_WS_MASK);
}

/**
 * @brief set RAM read waitstates
 *
 * @note 00 = 0, 01 = 1, 10 = 2, 11 = 3
 */

void leon3_memcfg_set_ram_read_waitstates(uint32_t wait_states)
{
	leon3_memcfg_replace_mcfg2(wait_states & LEON3_MEMCFG2_RAM_READ_WS_MASK,
			     LEON3_MEMCFG2_RAM_READ_WS_MASK);
}


/**
 * @brief enable flash and ram access for use with IASW/IBSW
 * @note does NOT change settings in the FPGA!
 */

void leon3_memcfg_configure_sram_flash_access(void)
{
	struct leon3_ftmctrl_registermap *memctrl =
	  (struct leon3_ftmctrl_registermap *) LEON3_BASE_ADDRESS_FTMCTRL;	

	/* NOTE: setting the bits individually does not work,
	   because some depend on others */

	iowrite32be(LEON3_MEMCFG1_FLASH, &memctrl->mcfg1);
	iowrite32be(LEON3_MEMCFG2_SRAM, &memctrl->mcfg2);
	iowrite32be(LEON3_MEMCFG3_SRAM, &memctrl->mcfg3);
}

