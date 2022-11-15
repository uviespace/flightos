/**
 * @file    leon3_dsu.c
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
 * @defgroup leon3_dsu LEON3 DSU interface
 *
 * @brief Access to the DSU of the GR712RC LEON3FT (and possibly others)
 *
 *
 * ## Overview
 *
 * This component implements access to the debug support unit of the GR712RC
 * LEON3 processor.
 *
 * ## Mode of Operation
 *
 * @see _GR712RC user manual v2.7 chapter 9_ for information on the DSU
 *
 * ## Error Handling
 *
 * None
 *
 * ## Notes
 *
 * - functionality is added as needed
 * - this can be the basis for a grmon replacement
 *
 */

#include <leon3_dsu.h>
#include <kernel/printk.h>
#include <asm-generic/io.h>



/**
 * @brief calculate address of output register %on for a window
 * @see GR712-UM v2.3 pp. 81
 *
 * @param cpu the cpu number
 * @param n   the index of the output register %on
 * @param cwp the index of the register window
 *
 * @return 0 if error, otherwise address of register
 */

__attribute__((unused))
static uint32_t dsu_get_output_reg_addr(uint32_t cpu, uint32_t n, uint32_t cwp)
{
	if (cwp > NWINDOWS) {
		pr_err("ERR_DSU_CWP_INVALID\n");
		return 0;
	}

	return (DSU_BASE(cpu) + 0x300000
		+ (((cwp * 64) + 32 + (n * 4)) % (NWINDOWS * 64)));
}


/**
 * @brief calculate address of local register %ln for a window
 * @see GR712-UM v2.3 pp. 81
 *
 * @param cpu the cpu number
 * @param n   the index of the local register %ln
 * @param cwp the index of the register window
 *
 * @return 0 if error, otherwise address of register
 */

__attribute__((unused))
static uint32_t dsu_get_local_reg_addr(uint32_t cpu, uint32_t n, uint32_t cwp)
{
	if (cwp > NWINDOWS) {
		pr_err("ERR_DSU_CWP_INVALID\n");
		return 0;
	}

	return (DSU_BASE(cpu) + 0x300000
		+ (((cwp * 64) + 64 + (n * 4)) % (NWINDOWS * 64)));
}


/**
 * @brief calculate address of input register %in for a window
 * @see GR712-UM v2.3 pp. 81
 *
 * @param cpu the cpu number
 * @param n   the index of the input register %in
 * @param cwp the index of the register window
 *
 * @return 0 if error, otherwise address of register
 */

__attribute__((unused))
static uint32_t dsu_get_input_reg_addr(uint32_t cpu, uint32_t n, uint32_t cwp)
{
	if (cwp > NWINDOWS) {
		pr_err("ERR_DSU_CWP_INVALID\n");
		return 0;
	}

	return (DSU_BASE(cpu) + 0x300000
		+ (((cwp * 64) + 96 + (n * 4)) % (NWINDOWS * 64)));
}


/**
 * @brief calculate address of global register %gn
 * @see GR712-UM v2.3 pp. 81
 *
 * @param cpu the cpu number
 * @param n   the index of the global register %gn
 *
 * @return address of register
 */

__attribute__((unused))
static uint32_t dsu_get_global_reg_addr(uint32_t cpu, uint32_t n)
{
	return (DSU_BASE(cpu) + 0x300000 + n * 4);
}


/**
 * @brief calculate address of floating point register %fn
 * @see GR712-UM v2.3 pp. 81
 *
 * @param cpu the cpu number
 * @param n   the index of the floating-point register %fn
 *
 * @return address of register
 */

__attribute__((unused))
static uint32_t dsu_get_fpu_reg_addr(uint32_t cpu, uint32_t n)
{
	return (DSU_BASE(cpu) + 0x301000 + n * 4);
}


/**
 * @brief set bits in the DSU control register
 *
 * @param cpu   the cpu number
 * @param flags the bitmask to set
 */

static void dsu_set_dsu_ctrl(uint32_t cpu, uint32_t flags)
{
	uint32_t tmp;


	tmp  = ioread32be((void *) (DSU_CTRL + DSU_OFFSET_CPU(cpu)));
	tmp |= flags;
	iowrite32be(tmp,  (void *) (DSU_CTRL + DSU_OFFSET_CPU(cpu)));
}


/**
 * @brief get the DSU control register
 *
 * @param cpu   the cpu number
 *
 * @return the contents of the DSU control register
 */

static uint32_t dsu_get_dsu_ctrl(uint32_t cpu)
{
	return ioread32be((void *) (DSU_CTRL + DSU_OFFSET_CPU(cpu)));
}


/**
 * @brief clear bits in the DSU control register
 *
 * @param cpu   the cpu number
 * @param flags the bitmask of flags to clear
 */

static void dsu_clear_dsu_ctrl(uint32_t cpu, uint32_t flags)
{
	uint32_t tmp;


	tmp  = ioread32be((void *) (DSU_CTRL + DSU_OFFSET_CPU(cpu)));
	tmp &= ~flags;
	iowrite32be(tmp,  (void *) (DSU_CTRL + DSU_OFFSET_CPU(cpu)));
}


/**
 * @brief clear the Integer Units register file
 *
 * @param cpu the cpu number
 */

void dsu_clear_iu_reg_file(uint32_t cpu)
{
	uint32_t i;
	/* (NWINDOWS * (%ln + %ion) + %gn) * 4 bytes */
	const uint32_t iu_reg_size = (NWINDOWS * (8 + 8) + 8) * 4;


	for (i = 0; i < iu_reg_size; i += 4)
		iowrite32be(0x0, (void *) (DSU_OFFSET_CPU(cpu)
					   + DSU_IU_REG + i));
}


/**
 * @brief enable forcing processor to enter debug mode if any other processor
 *        in the system enters debug mode
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 83
 */

void dsu_set_force_enter_debug_mode(uint32_t cpu)
{
	uint16_t tmp;

	struct dsu_mode_mask *dmm
		= (struct dsu_mode_mask *) DSU_MODE_MASK;


	tmp  = ioread16be(&dmm->enter_debug);
	tmp |= (1 << cpu);
	iowrite16be(tmp, &dmm->enter_debug);
}


/**
 * @brief disable forcing processor to enter debug mode if any other processor
 *        in the system enters debug mode
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 83
 */

void dsu_clear_force_enter_debug_mode(uint32_t cpu)
{
	uint16_t tmp;

	struct dsu_mode_mask *dmm
		= (struct dsu_mode_mask *) DSU_MODE_MASK;


	tmp  = ioread16be(&dmm->enter_debug);
	tmp &= ~(1 << cpu);
	iowrite16be(tmp, &dmm->enter_debug);
}


/**
 * @brief do not allow a processor to force other processors into debug mode
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 83
 */

void dsu_set_noforce_debug_mode(uint32_t cpu)
{
	uint16_t tmp;

	struct dsu_mode_mask *dmm
		= (struct dsu_mode_mask *) DSU_MODE_MASK;


	tmp  = ioread16be(&dmm->debug_mode);
	tmp |= (1 << cpu);
	iowrite16be(tmp, &dmm->debug_mode);
}


/**
 * @brief allow a processor to force other processors into debug mode
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 83
 */

void dsu_clear_noforce_debug_mode(uint32_t cpu)
{
	uint16_t tmp;

	struct dsu_mode_mask *dmm
		= (struct dsu_mode_mask *) DSU_MODE_MASK;


	tmp  = ioread16be(&dmm->debug_mode);
	tmp &= ~(1 << cpu);
	iowrite16be(tmp, &dmm->debug_mode);
}


/**
 * @brief force a processors into debug mode if Break on Watchpoint (BW) bit
 *        in DSU control register is set
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_set_force_debug_on_watchpoint(uint32_t cpu)
{
	uint16_t tmp;

	struct dsu_break_step *dbs
		= (struct dsu_break_step *) DSU_BREAK_STEP;


	tmp  = ioread16be(&dbs->break_now);
	tmp |= (1 << cpu);
	iowrite16be(tmp, &dbs->break_now);
}


/**
 * @brief clear forcing debug mode if Break on
 *	  Watchpoint (BW) bit in the DSU control register is set;
 *	  resumes processor execution if in debug mode
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 83
 */

void dsu_clear_force_debug_on_watchpoint(uint32_t cpu)
{
	uint16_t tmp;

	struct dsu_break_step *dbs
		= (struct dsu_break_step *) DSU_BREAK_STEP;


	tmp  = ioread16be(&dbs->break_now);
	tmp &= ~(1 << cpu);
	iowrite16be(tmp, &dbs->break_now);
}


/**
 * @brief check if cpu is in error mode
 *
 * @param cpu the cpu number
 * @return 1 if processor in error mode, else 0
 *
 * @see GR712-UM v2.3 pp. 82
 */

uint32_t dsu_cpu_in_error_mode(uint32_t cpu)
{
	return (dsu_get_dsu_ctrl(cpu) & DSU_CTRL_PE);
}


/**
 * @brief clear debug and halt flag of processor
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_clear_cpu_error_mode(uint32_t cpu)
{
	dsu_clear_dsu_ctrl(cpu, DSU_CTRL_PE);
}


/**
 * @brief check if cpu is in halt mode
 *
 * @param cpu the cpu number
 *
 * @return 1 if processor in halt mode, else 0
 *
 * @see GR712-UM v2.3 pp. 82
 */

uint32_t dsu_cpu_in_halt_mode(uint32_t cpu)
{
	return (dsu_get_dsu_ctrl(cpu) & DSU_CTRL_HL);
}


/**
 * @brief  put cpu in halt mode
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_cpu_set_halt_mode(uint32_t cpu)
{
	dsu_set_dsu_ctrl(cpu, DSU_CTRL_HL);
}


/**
 * @brief  enable debug mode on error
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_set_cpu_debug_on_error(uint32_t cpu)
{
	dsu_set_dsu_ctrl(cpu, DSU_CTRL_BE);
}


/**
 * @brief  disable debug mode on error
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_clear_cpu_debug_on_error(uint32_t cpu)
{
	dsu_clear_dsu_ctrl(cpu, DSU_CTRL_BE);
}


/**
 * @brief  enable debug mode on IU watchpoint
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_set_cpu_break_on_iu_watchpoint(uint32_t cpu)
{
	dsu_set_dsu_ctrl(cpu, DSU_CTRL_BW);
}


/**
 * @brief  disable debug mode on IU watchpoint
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_clear_cpu_break_on_iu_watchpoint(uint32_t cpu)
{
	dsu_clear_dsu_ctrl(cpu, DSU_CTRL_BW);
}


/**
 * @brief  enable debug mode on breakpoint instruction (ta 1)
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_set_cpu_break_on_breakpoint(uint32_t cpu)
{
	dsu_set_dsu_ctrl(cpu, DSU_CTRL_BS);
}


/**
 * @brief  disable debug mode on breakpoint instruction
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_clear_cpu_break_on_breakpoint(uint32_t cpu)
{
	dsu_clear_dsu_ctrl(cpu, DSU_CTRL_BS);
}


/**
 * @brief  enable debug mode on trap
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_set_cpu_break_on_trap(uint32_t cpu)
{
	dsu_set_dsu_ctrl(cpu, DSU_CTRL_BX);
}


/**
 * @brief  disable debug mode on trap
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_clear_cpu_break_on_trap(uint32_t cpu)
{
	dsu_clear_dsu_ctrl(cpu, DSU_CTRL_BX);
}


/**
 * @brief  enable debug mode on error trap (all except:
 *         (priviledged_instruction, fpu_disabled, window_overflow,
 *         window_underflow, asynchronous_interrupt, ticc_trap)
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_set_cpu_break_on_error_trap(uint32_t cpu)
{
	dsu_set_dsu_ctrl(cpu, DSU_CTRL_BZ);
}


/**
 * @brief  disable debug mode on error trap (all except:
 *         (priviledged_instruction, fpu_disabled, window_overflow,
 *         window_underflow, asynchronous_interrupt, ticc_trap)
 *
 * @param cpu the cpu number
 *
 * @see GR712-UM v2.3 pp. 82
 */

void dsu_clear_cpu_break_on_error_trap(uint32_t cpu)
{
	dsu_clear_dsu_ctrl(cpu, DSU_CTRL_BZ);
}

/**
 * @brief  get %y register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %y register
 */

uint32_t dsu_get_reg_y(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_Y));
}


/**
 * @brief  set %y register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_y(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_Y));
}


/**
 * @brief  get %psr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %psr register
 */

uint32_t dsu_get_reg_psr(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_PSR));
}


/**
 * @brief  set %psr register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_psr(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_PSR));
}


/**
 * @brief  get %wim register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %wim register
 */

uint32_t dsu_get_reg_wim(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_WIM));
}


/**
 * @brief  set %wim register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_wim(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_WIM));
}


/**
 * @brief  get %tbr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %tbr register
 */

uint32_t dsu_get_reg_tbr(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_TBR));
}


/**
 * @brief  set %tbr register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_tbr(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_TBR));
}


/**
 * @brief  get %pc register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %pc register
 */

uint32_t dsu_get_reg_pc(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_PC));
}


/**
 * @brief  set %npc register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_pc(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_PC));
}


/**
 * @brief  get %npc register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %npc register
 */

uint32_t dsu_get_reg_npc(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_NPC));
}


/**
 * @brief  set %npc register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_npc(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_NPC));
}


/**
 * @brief  get %fsr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %fsr register
 */

uint32_t dsu_get_reg_fsr(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_FSR));
}


/**
 * @brief  set %fsr register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_fsr(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_FSR));
}


/**
 * @brief  get %cpsr register of cpu
 *
 * @param cpu the cpu number
 *
 * @return contents of the %cpsr register
 */

uint32_t dsu_get_reg_cpsr(uint32_t cpu)
{
	return ioread32be((void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_CPSR));
}

/**
 * @brief  set %cpsr register of cpu
 *
 * @param cpu the cpu number
 * @param val the value to set
 */

void dsu_set_reg_cpsr(uint32_t cpu, uint32_t val)
{
	iowrite32be(val, (void *) (DSU_OFFSET_CPU(cpu) + DSU_REG_CPSR));
}

/**
 * @brief  set stack pointer register (%o6) in a window of a cpu
 *
 * @param cpu the cpu number
 * @param cwp the window number
 * @param val the value to set
 */

void dsu_set_reg_sp(uint32_t cpu, uint32_t cwp, uint32_t val)
{
	uint32_t reg;

	reg = dsu_get_output_reg_addr(cpu, 6, cwp);

	if (reg)
		iowrite32be(val, (void *) reg);
}


/**
 * @brief  get stack pointer register (%o6) in a window of a cpu
 *
 * @param cpu the cpu number
 * @param cwp the window number
 * @return the value of the stack pointer register or 0 if window/cpu is invalid
 */

uint32_t dsu_get_reg_sp(uint32_t cpu, uint32_t cwp)
{
	uint32_t reg;

	reg = dsu_get_output_reg_addr(cpu, 6, cwp);

	if (reg)
		return ioread32be((void *) reg);
	else
		return 0;
}


/**
 * @brief  set frame pointer register (%i6) in a window of a cpu
 *
 * @param cpu the cpu number
 * @param cwp the window number
 * @param val the value to set
 */

void dsu_set_reg_fp(uint32_t cpu, uint32_t cwp, uint32_t val)
{
	uint32_t reg;

	reg = dsu_get_input_reg_addr(cpu, 6, cwp);

	if (reg)
		iowrite32be(val, (void *) reg);
}
