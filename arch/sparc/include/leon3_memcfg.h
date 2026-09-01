/**
 * @file   arch/sparc/include/leon3_memcfg.h
 * @brief  LEON3 Memory Configuration Register definitions
 * @ingroup leon3_memcfg
 * @ingroup kmem
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   March, 2016
 */

#ifndef LEON3_MEMCFG_H
#define LEON3_MEMCFG_H

#include <kernel/types.h>


/**
 * @see GR712-UM v2.3 pp. 66
 */



#define LEON3_MEMCFG1_PROM_READ_WS_MASK		0x0000000F

#define LEON3_MEMCFG1_PROM_WRITE_WS_OFF		         4
#define LEON3_MEMCFG1_PROM_WRITE_WS_MASK	0x000000F0

#define LEON3_MEMCFG1_PROM_WIDTH_OFF		         8
#define LEON3_MEMCFG1_PROM_WIDTH_MASK		0x00000300

#define LEON3_MEMCFG1_PWEN			0x00000800

#define LEON3_MEMCFG1_ROMBANKSZ_OFF		        14
#define LEON3_MEMCFG1_ROMBANKSZ_MASK		0x00003C00

#define LEON3_MEMCFG1_IOEN			0x00080000

#define LEON3_MEMCFG1_IO_WAITSTATES_OFF		        20
#define LEON3_MEMCFG1_IO_WAITSTATES_MASK	0x00F00000

#define LEON3_MEMCFG1_BEXCN			0x02000000
#define LEON3_MEMCFG1_IBRDY			0x04000000

#define LEON3_MEMCFG1_IOBUSW_OFF			27
#define LEON3_MEMCFG1_IOBUSW_MASK		0x18000000

#define LEON3_MEMCFG1_ABRDY			0x20000000
#define LEON3_MEMCFG1_PBRDY			0x40000000




#define LEON3_MEMCFG2_RAM_READ_WS_MASK		0x00000003

#define LEON3_MEMCFG2_RAM_WRITE_WS_OFF		         2
#define LEON3_MEMCFG2_RAM_WRITE_WS_MASK		0x0000000C

#define LEON3_MEMCFG2_RAM_WIDTH_OFF		         4
#define LEON3_MEMCFG2_RAM_WIDTH_MASK		0x00000030

#define LEON3_MEMCFG2_RMW			0x00000040

#define LEON3_MEMCFG2_RAM_BANK_SIZE_OFF		         9
#define LEON3_MEMCFG2_RAM_BANK_SIZE_MASK	0x00001E00

#define LEON3_MEMCFG2_SI			0x00002000
#define LEON3_MEMCFG2_SE			0x00004000


#define LEON3_MEMCFG3_PROM_EDAC			0x00000100
#define LEON3_MEMCFG3_RAM_EDAC			0x00000200
#define LEON3_MEMCFG3_RB_EDAC			0x00000400
#define LEON3_MEMCFG3_WB_EDAC			0x00000800
#define LEON3_MEMCFG3_TCB			0x0000007F



/* memory configuration for flash + SRAM */
#define LEON3_MEMCFG1_FLASH 0x101aca11
#define LEON3_MEMCFG2_SRAM  0x00001665
#define LEON3_MEMCFG3_SRAM  0x08000300

#define LEON3_MEMCFG1_PROMACCESS 0x10180011



/**
 * @brief enable the ram edac
 */

void leon3_memcfg_enable_ram_edac(void);

/**
 * @brief disable the ram edac
 */

void leon3_memcfg_disable_ram_edac(void);

/**
 * @brief enable edac ram bypass write
 */

void leon3_memcfg_enable_edac_write_bypass(void);

/**
 * @brief disable edac ram bypass write
 */

void leon3_memcfg_disable_edac_write_bypass(void);

/**
 * @brief enable edac ram bypass read
 */

void leon3_memcfg_enable_edac_read_bypass(void);

/**
 * @brief disable edac ram bypass read
 */

void leon3_memcfg_disable_edac_read_bypass(void);

/**
 * @brief perform a bypass read and retrieve the checkbits
 *
 * @param addr the address pointer to read from
 * @param tcb checkbits storage
 *
 * @return data word at address
 */

uint32_t leon3_memcfg_bypass_read(void *addr, uint8_t *tcb);

/**
 * @brief set custom checkbits and perform a bypass write
 *
 * @param addr the address pointer to write to
 * @param value the data word to write
 * @param tcb the checkbits to set
 */

void leon3_memcfg_bypass_write(void *addr, uint32_t value, uint8_t tcb);

/**
 * @brief enable flash and ram access for use with IASW/IBSW
 */

void leon3_memcfg_configure_sram_flash_access(void);


/**
 * @brief set RAM bank size
 *
 * @param bank_size the RAM bank size value to set
 *
 * @note bank_size = lg2(kilobytes / 8)
 */

void leon3_memcfg_set_ram_bank_size(uint32_t bank_size);

/**
 * @brief set RAM width
 *
 * @param ram_width the RAM width value to set
 *
 * @note 00 = 8 bits, 1X = 32 bits
 */

void leon3_memcfg_set_ram_width(uint32_t ram_width);

/**
 * @brief set RAM read waitstates
 *
 * @param wait_states the number of read wait states to set
 *
 * @note 00 = 0, 01 = 1, 10 = 2, 11 = 3
 */

void leon3_memcfg_set_ram_read_waitstates(uint32_t wait_states);

/**
 * @brief set RAM write waitstates
 *
 * @param wait_states the number of write wait states to set
 *
 * @note 00 = 0, 01 = 1, 10 = 2, 11 = 3
 */

void leon3_memcfg_set_ram_write_waitstates(uint32_t wait_states);

/**
 * @brief enable read-modify-write cycles
 */

void leon3_memcfg_set_read_modify_write(void);

/**
 * @brief enable SRAM
 */

void leon3_memcfg_clear_sram_disable(void);


#endif /* LEON3_MEMCFG_H */
