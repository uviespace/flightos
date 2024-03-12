#include <kernel/init.h>
#include <kernel/log2.h>
#include <leon3_memcfg.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/watchdog.h>

#include <ahb.h>
#include <leon3_dsu.h>
#include <stacktrace.h>
#include <asm/processor.h>
#include <traps.h>
#include <kernel/irq.h>
#include <kernel/time.h>

#include <asm/leon.h>
#include <asm/ttable.h>


#define STACKTRACE_MAX_ENTRIES 7

#define SMILE_MRAM_BANK_SIZE		8192
#define SMILE_MRAM_BUS_WIDTH		0	/* 8 bit width as per GR712RC-UM v2.7 p67 */
#define SMILE_MRAM_RD_WAIT_STATES	3
#define SMILE_MRAM_WR_WAIT_STATES	3


#define EXCHANGE_AREA_BASE_ADDR		0x6003F000

#define WD_TIMER_IRL	11


struct cpu_reginfo {
	uint32_t psr;
	uint32_t wim;
	uint32_t pc;
	uint32_t npc;
	uint32_t fsr;
};


struct exchange_area {

	uint16_t reset_type;
	uint8_t err_cnt;
	uint8_t no_conn_rst_cnt;
	uint64_t reset_time;
	uint8_t trapnum_core0;
	uint8_t trapnum_core1;

	uint16_t sw_trap_id;

	struct cpu_reginfo regs_cpu0;
	struct cpu_reginfo regs_cpu1;
	uint32_t ahb_status_reg;
	uint32_t ahb_failing_addr_reg;

	uint32_t stacktrace_cpu0[STACKTRACE_MAX_ENTRIES];
	uint32_t stacktrace_cpu1[STACKTRACE_MAX_ENTRIES];
} __attribute__((packed));


/**
 * @brief the dpu reset function
 * @param reset_type a reset type
 * @note this writes the exchange area specified in SMILE-IWF-PL-RS-015
 * DPU-SSSIF-IF-5420
 */

irqreturn_t smile_write_reset_info(unsigned int irq, void *userdata)
{
	struct sparc_stackf *trace_frames[STACKTRACE_MAX_ENTRIES];
	struct pt_regs      *trace_cpuregs[STACKTRACE_MAX_ENTRIES];

	uint32_t i;

	uint32_t sp;
	uint32_t pc;

	struct timespec kt;
	uint32_t coarse;
	uint32_t fine;
	char *ts;

	register int sp_local asm("%sp");

	struct stack_trace trace;

	struct exchange_area *exchange = (struct exchange_area *) EXCHANGE_AREA_BASE_ADDR;

	/* re-enable traps, but mask all IRQs */
	put_psr(get_psr() | PSR_PIL | PSR_ET);

#if 0
	exchange->reset_type = 0x223;	/* EVT_RES_EXCEPT */
#endif

	/* fill uptime in CUC format */
	kt = get_ktime();
        coarse = kt.tv_sec;
        fine   = kt.tv_nsec / 1000;

	ts = (char *) &exchange->reset_time;

        ts[0] = (coarse >> 24) & 0xff;
        ts[1] = (coarse >> 16) & 0xff;
        ts[2] = (coarse >>  8) & 0xff;
        ts[3] =  coarse        & 0xff;
        ts[4] = (fine   >> 16) & 0xff;
        ts[5] = (fine   >>  8) & 0xff;
        ts[6] =  fine          & 0xff;
        ts[7] = 0;

	/* latest traps */
	exchange->trapnum_core0 = (dsu_get_reg_tbr(0) >> 4) & 0xff;
	exchange->trapnum_core1 = (dsu_get_reg_tbr(1) >> 4) & 0xff;

	/** REGISTERS CORE1 **/
	exchange->regs_cpu0.psr = dsu_get_reg_psr(0);
	exchange->regs_cpu0.wim = dsu_get_reg_wim(0);
	exchange->regs_cpu0.pc  = dsu_get_reg_pc(0);
	exchange->regs_cpu0.npc = dsu_get_reg_npc(0);
	exchange->regs_cpu0.fsr = dsu_get_reg_fsr(0);

	/** REGISTERS CORE2 **/
	exchange->regs_cpu1.psr = dsu_get_reg_psr(1);
	exchange->regs_cpu1.wim = dsu_get_reg_wim(1);
	exchange->regs_cpu1.pc  = dsu_get_reg_pc(1);
	exchange->regs_cpu1.npc = dsu_get_reg_npc(1);
	exchange->regs_cpu1.fsr = dsu_get_reg_fsr(1);

	/** AHB STATUS REGISTER and AHB FAILING ADDRESS REG **/
	exchange->ahb_status_reg       = ahbstat_get_status();
	exchange->ahb_failing_addr_reg = ahbstat_get_failing_addr();

	/* unused slots in the trace should be cleared */
	bzero(exchange->stacktrace_cpu0, sizeof(exchange->stacktrace_cpu0));
	bzero(exchange->stacktrace_cpu1, sizeof(exchange->stacktrace_cpu1));


	/** CALL STACK CORE 1 **/
	trace.max_entries = STACKTRACE_MAX_ENTRIES;
	trace.nr_entries  = 0;
	trace.frames      = trace_frames;
	trace.regs        = trace_cpuregs;


	/* The DSU of the GR712 appears to return the contents of a different register
	 * than requested. Usually it is the following entry, i.e. when requesting
	 * the register %o6 (== %sp) of a particular register file (CPU window) we
	 * get the contents %o7 (address of caller). This can also be seen in the AHB
	 * trace buffer. Hence, we use the local value of %sp (i.e. of the CPU calling
	 * this function). For the other CPU, we can't do much if the returned value
	 * is incorrect, so a stack trace may not be valid or contain meaningful
	 * information. Interestingly enough, this behaviour appears to depend on
	 * the length and operations of the current function, so this may really be
	 * a problem with the types and amounts of transfers crossing the bus.
	 */

	sp = sp_local;
	pc = dsu_get_reg_pc(0);	/* not critical if incorrect */
	save_stack_trace(&trace, sp, pc);


	/* first is always current %pc */
	exchange->stacktrace_cpu0[0] = pc;

	for (i = 1; i < trace.nr_entries - 1; i++)
		exchange->stacktrace_cpu0[i] = (uint32_t) trace.frames[i - 1]->callers_pc;


	/** CALL STACK CORE 2 **/
	trace.max_entries = STACKTRACE_MAX_ENTRIES;
	trace.nr_entries  = 0;

	sp = dsu_get_reg_sp(1, dsu_get_reg_psr(1) & 0x1f);
	pc = dsu_get_reg_pc(1);
	save_stack_trace(&trace, sp, pc);

	/* first is always current %pc */
	exchange->stacktrace_cpu1[0] = pc;

	for (i = 1; i < trace.nr_entries - 1; i++)
		exchange->stacktrace_cpu1[i] = (uint32_t) trace.frames[i - 1]->callers_pc;

	/* wait for sweet death to take us */
	die();

	return 0; /* we never will */
}

static void smile_watchdog_handler(void *userdata)
{
	smile_write_reset_info(0, userdata);
}

static void smile_write_reset_info_trap(void)
{
	smile_write_reset_info(0, NULL);
}

static int smile_cfg_reset_traps(void)
{
	/* called by machine_halt */
	trap_handler_install(0x82, smile_write_reset_info_trap);
	watchdog_set_handler(smile_watchdog_handler, NULL);

	return 0;
}
late_initcall(smile_cfg_reset_traps)



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
