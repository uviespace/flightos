/**
 * @file    leon3_dsu.h
 * @brief   LEON3 Debug Support Unit interface definitions
 * @ingroup leon3_dsu
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   February, 2016
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

#ifndef LEON3_DSU_H
#define LEON3_DSU_H

#include <stdint.h>
/*
 * helper macro to fill unused 4-byte slots with "unusedXX" variables
 * yes, this needs to be done exactly like that, or __COUNTER__ will not be expanded
 */

#define CONCAT(x, y)            x##y
#define MACRO_CONCAT(x, y)      CONCAT(x, y)
#define UNUSED_UINT32_SLOT       uint32_t MACRO_CONCAT(unused, __COUNTER__)



#ifndef NWINDOWS
#define NWINDOWS	8	/* number of register windows */
#endif


/**
 * @see GR712-UM v2.3 pp. 81
 */


#define DSU_CTRL		0x90000000
#define DSU_TIMETAG		0x90000008

#define DSU_BREAK_STEP		0x90000020	/* mapped only in CPU0 DSU */
#define DSU_MODE_MASK		0x90000024	/* mapped only in CPU0 DSU */

#define DSU_AHB_TRACE_CTRL	0x90000040
#define DSU_AHB_TRACE_IDX	0x90000044

#define DSU_AHB_BP_ADDR_1	0x90000050
#define DSU_AHB_MASK_1		0x90000054
#define DSU_AHB_BP_ADDR_2	0x90000058
#define DSU_AHB_MASK_2		0x9000005c

#define DSU_INST_TRCE_BUF_START	0x90100000
#define DSU_INST_TRCE_BUF_LINES	256
#define DSU_INST_TRCE_CTRL	0x90110000

#define DSU_AHB_TRCE_BUF_START	0x90200000
#define DSU_AHB_TRCE_BUF_LINES	256

#define DSU_IU_REG		0x90300000

#define DSU_IU_REG_CHECKBITS	0x90300800

#define DSU_FPU_REG		0x90301000

#define DSU_IU_SPECIAL_REG	0x90400000

#define DSU_REG_Y		0x90400000
#define DSU_REG_PSR		0x90400004
#define DSU_REG_WIM		0x90400008
#define DSU_REG_TBR		0x9040000C
#define DSU_REG_PC		0x90400010
#define DSU_REG_NPC		0x90400014
#define DSU_REG_FSR		0x90400018
#define DSU_REG_CPSR		0x9040001C




#define DSU_OFFSET_CPU(x)	((x & 0x07) << 24)
#define DSU_BASE(x)		(DSU_CTRL + DSU_OFFSET_CPU(x))


/**
 * @see GR712-UM v2.3 p. 82
 */
#define DSU_CTRL_TE		(1 <<  0)	/* trace enable            */
#define DSU_CTRL_BE		(1 <<  1)	/* break on error          */
#define DSU_CTRL_BW		(1 <<  2)	/* break on IU watchpoint  */
#define DSU_CTRL_BS		(1 <<  3)	/* break on s/w breakpoint */
#define DSU_CTRL_BX		(1 <<  4)	/* break on any trap       */
#define DSU_CTRL_BZ		(1 <<  5)	/* break on error trap     */
#define DSU_CTRL_DM		(1 <<  6)	/* RO: debug mode status   */
					/* bits 7, 8 are unused    */
#define DSU_CTRL_PE		(1 <<  9)	/* processor error mode    */
#define DSU_CTRL_HL		(1 << 10)	/* processor halt mode     */
#define DSU_CTRL_PW		(1 << 11)	/* processor power mode    */






/**
 * maps the break and step register
 * @see GR712-UM v2.3 p.82
 */
__extension__
struct dsu_break_step {
	union {
		struct {
			uint16_t single_step;
			uint16_t break_now;
		};
		uint32_t reg;
	};
};

/**
 * maps the debug mode maskregister
 * @see GR712-UM v2.3 p.83
 */
__extension__
struct dsu_mode_mask {
	union {
		struct {
			uint16_t debug_mode;
			uint16_t enter_debug;
		};
		uint32_t reg;
	};
};



/**
 * maps the AHB trace buffer registers from DSU_AHB_TRACE_CTRL to DSU_AHB_MASK_2
 * must be pointed to DSU_AHB_TRACE_CTRL
 */

struct ahb_bp_reg {
	uint32_t addr;
	uint32_t mask;
};

struct dsu_ahb_trace_ctrl {
	uint32_t ctrl;
	uint32_t idx;
	UNUSED_UINT32_SLOT;
	UNUSED_UINT32_SLOT;
	UNUSED_UINT32_SLOT;
	struct ahb_bp_reg bp[2];
};


/**
 * a single line in the instruction trace buffer, see GR712-UM v2.3 p.80
 */
__extension__
struct instr_trace_buffer_line {

	union {
		struct {
			uint32_t unused		  :1;
			uint32_t multi_cycle_inst :1;
			uint32_t timetag	  :30;
			uint32_t load_store_param :32;
			uint32_t program_cntr	  :30;
			uint32_t instr_trap	  :1;
			uint32_t proc_error_mode  :1;
			uint32_t opcode		  :32;
		};
		uint32_t field[4];
	};
}__attribute__((packed));

/**
 * maps the instruction trace buffer, must be pointed to DSU_INST_TRCE_BUF_START
 */

struct dsu_instr_trace_buffer {
	struct instr_trace_buffer_line line[DSU_INST_TRCE_BUF_LINES];
};


/**
 * a single line in the AHB trace buffer, see GR712-UM v2.3 p.79
 */
__extension__
struct ahb_trace_buffer_line {

	union {
		struct {
			uint32_t ahb_bp_hi	  :1;
			uint32_t unused1	  :1;
			uint32_t timetag	  :30;
			uint32_t unused2	  :1;
			uint32_t hirq		  :15;
			uint32_t hwrite		  :1;
			uint32_t htrans		  :2;
			uint32_t hsize		  :3;
			uint32_t hburst		  :3;
			uint32_t hmaster	  :4;
			uint32_t hmastlock	  :1;
			uint32_t hresp		  :2;
			uint32_t load_store_data  :32;
			uint32_t load_store_addr  :32;
		};
		uint32_t field[4];
	};
}__attribute__((packed));


/**
 * maps the AHB trace buffer, must be pointed to DSU_AHB_TRCE_BUF_START
 */

struct dsu_ahb_trace_buffer {
	struct ahb_trace_buffer_line line[DSU_AHB_TRCE_BUF_LINES];
};




/**
 * @brief do not allow a processor to force other processors into debug mode
 *
 * @param cpu the cpu number
 */

void dsu_set_noforce_debug_mode(uint32_t cpu);

/**
 * @brief enable debug mode on error
 *
 * @param cpu the cpu number
 */

void dsu_set_cpu_debug_on_error(uint32_t cpu);

/**
 * @brief enable debug mode on IU watchpoint
 *
 * @param cpu the cpu number
 */

void dsu_set_cpu_break_on_iu_watchpoint(uint32_t cpu);

/**
 * @brief enable debug mode on breakpoint instruction (ta 1)
 *
 * @param cpu the cpu number
 */

void dsu_set_cpu_break_on_breakpoint(uint32_t cpu);

/**
 * @brief enable debug mode on trap
 *
 * @param cpu the cpu number
 */

void dsu_set_cpu_break_on_trap(uint32_t cpu);

/**
 * @brief enable debug mode on error trap
 *
 * @param cpu the cpu number
 */

void dsu_set_cpu_break_on_error_trap(uint32_t cpu);

/**
 * @brief force a processor into debug mode if Break on Watchpoint (BW) bit
 *        in DSU control register is set
 *
 * @param cpu the cpu number
 */

void dsu_set_force_debug_on_watchpoint(uint32_t cpu);

/**
 * @brief get %psr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %psr register
 */

uint32_t dsu_get_reg_psr(uint32_t cpu);

/**
 * @brief get %tbr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %tbr register
 */

uint32_t dsu_get_reg_tbr(uint32_t cpu);

/**
 * @brief set %psr register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_psr(uint32_t cpu, uint32_t val);

/**
 * @brief get %wim register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %wim register
 */

uint32_t dsu_get_reg_wim(uint32_t cpu);

/**
 * @brief get %pc register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %pc register
 */

uint32_t dsu_get_reg_pc(uint32_t cpu);

/**
 * @brief get stack pointer register (%o6) in a window of a cpu
 *
 * @param cpu the cpu number
 * @param cwp the window number
 *
 * @return the value of the stack pointer register or 0 if window/cpu is invalid
 */

uint32_t dsu_get_reg_sp(uint32_t cpu, uint32_t cwp);

/**
 * @brief get %fsr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %fsr register
 */

uint32_t dsu_get_reg_fsr(uint32_t cpu);


/**
 * @brief set %tbr register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_tbr(uint32_t cpu, uint32_t val);

/**
 * @brief set %pc register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_pc(uint32_t cpu, uint32_t val);

/**
 * @brief set %npc register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_npc(uint32_t cpu, uint32_t val);

/**
 * @brief get %npc register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %npc register
 */

uint32_t dsu_get_reg_npc(uint32_t cpu);

/**
 * @brief clear the Integer Units register file
 *
 * @param cpu the cpu number
 */

void dsu_clear_iu_reg_file(uint32_t cpu);

/**
 * @brief disable debug mode on error
 *
 * @param cpu the cpu number
 */

void dsu_clear_cpu_debug_on_error(uint32_t cpu);

/**
 * @brief disable debug mode on IU watchpoint
 *
 * @param cpu the cpu number
 */

void dsu_clear_cpu_break_on_iu_watchpoint(uint32_t cpu);

/**
 * @brief disable debug mode on breakpoint instruction
 *
 * @param cpu the cpu number
 */

void dsu_clear_cpu_break_on_breakpoint(uint32_t cpu);

/**
 * @brief disable debug mode on trap
 *
 * @param cpu the cpu number
 */

void dsu_clear_cpu_break_on_trap(uint32_t cpu);

/**
 * @brief disable debug mode on error trap
 *
 * @param cpu the cpu number
 */

void dsu_clear_cpu_break_on_error_trap(uint32_t cpu);

/**
 * @brief clear forcing debug mode if Break on Watchpoint (BW) bit in the
 *        DSU control register is set; resumes processor execution if in
 *        debug mode
 *
 * @param cpu the cpu number
 */

void dsu_clear_force_debug_on_watchpoint(uint32_t cpu);

/**
 * @brief set %wim register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_wim(uint32_t cpu, uint32_t val);

/**
 * @brief set stack pointer register (%o6) in a window of a cpu
 *
 * @param cpu the cpu number
 * @param cwp the window number
 * @param val the value to set
 */

void dsu_set_reg_sp(uint32_t cpu, uint32_t cwp, uint32_t val);

/**
 * @brief set frame pointer register (%i6) in a window of a cpu
 *
 * @param cpu the cpu number
 * @param cwp the window number
 * @param val the value to set
 */

void dsu_set_reg_fp(uint32_t cpu, uint32_t cwp, uint32_t val);

/**
 * @brief put cpu in halt mode
 *
 * @param cpu the cpu number
 */

void dsu_cpu_set_halt_mode(uint32_t cpu);


#endif /* LEON3_DSU_H */
