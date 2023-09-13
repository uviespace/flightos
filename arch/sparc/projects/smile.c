#include <kernel/init.h>
#include <kernel/log2.h>
#include <leon3_memcfg.h>
#include <kernel/printk.h>

#define SMILE_MRAM_BANK_SIZE		8192
#define SMILE_MRAM_BUS_WIDTH		0	/* 8 bit width as per GR712RC-UM v2.7 p67 */
#define SMILE_MRAM_RD_WAIT_STATES	3
#define SMILE_MRAM_WR_WAIT_STATES	3

static int smile_mem_cfg(void)
{
	leon3_memcfg_set_ram_bank_size(ilog2(SMILE_MRAM_BANK_SIZE / 8));
	leon3_memcfg_set_ram_width(0);
	leon3_memcfg_set_ram_read_waitstates(SMILE_MRAM_RD_WAIT_STATES);
	leon3_memcfg_set_ram_write_waitstates(SMILE_MRAM_WR_WAIT_STATES);
	leon3_memcfg_set_read_modify_write();
	leon3_memcfg_enable_ram_edac();
	leon3_memcfg_clear_sram_disable();

	return 0;
}


core_initcall(smile_mem_cfg)
