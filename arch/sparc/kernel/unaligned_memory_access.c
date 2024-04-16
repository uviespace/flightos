/**
 * @file   arch/sparc/kernel/unaligned_memory_access.c
 *
 * @ingroup sparc
 *
 * @brief handle unaligned memory access
 *
 * @note this is a form of FDIR, as all unaligned access on this
 *	 platform should be considered an implementation error
 */

#include <stdlib.h>

#include <compiler.h>
#include <asm/leon.h>
#include <stacktrace.h>
#include <kernel/printk.h>
#include <kernel/kernel.h>
#include <asm/ttable.h>


/* XXX TODO sysctl interface */
static size_t   mna_cnt;
static uint32_t mna_pc_last;
static uint32_t mna_addr_last;



static int mna_can_handle(uint32_t insn)
{
	uint32_t op3;


	/* all SPARCv8 memory instructions are op type 3,
	 * see v8 manual, Figure 5-1 Summary of Instruction Format
	 */
	op3 = (insn & 0x01F80000) >> 19;

	/**
	 * 0b001001: ldsb	0b000101: stb	0b001101: ldstub
	 * 0b001010: ldsh	0b000110: sth	0b011101: ldstuba
	 * 0b000001: ldub	0b000100: st
	 * 0b000010: lduh	0b000111: std	0b001111: swap
	 * 0b000000: ld		0b010101: stba	0b011111: swapa
	 * 0b000011: ldd	0b010110: stha
	 * 0b011001: ldsba	0b010100: sta
	 * 0b011010: ldsha	0b010111: stda
	 * 0b010001: lduba	     ^
	 * 0b010010: lduha	     |
	 * 0b010000: lda	     |
	 * 0b010011: ldda	     |
	 *      ^--------------------
	 *
	 * - bit 2 is low for all regular loads and high for all regular
	 *   stores;
	 * - bit 3 indicates signedness (high == signed); this only concerns
	 *   loads of bytes and half-words (for sign extension in CPU)
	 * - the ldstub atomics cannot be unaligned, an hence need not be
	 *   considered
	 * - cannot handle swap (0xF) or swapa (0x1f)
	 *
	 *
	 * 0b100000: ldf	0b100100: stf
	 * 0b100011: lddf       0b100111: stdd
	 * 0b100001: ldfsr      0b100101: stfsr
	 *   ^                  0b100110: stdfq
	 *  |---------------------^
	 *
	 * no handler for float mna, insn are marked by bit 5 high
	 */


	if (op3 & 0x20)
		return -1;

	if (op3 == 0xf)
		return -1;

	if (op3 == 0x1f)
		return -1;

	if (op3 & 0x4)
		return 1;	/* load */


	return 0;		/* store */
}


static int mna_get_access_size(uint32_t insn)
{
	/* the lowest 2 bits of op3 of any mem instruction
	 * encode the access size:
	 *
	 * 0b00: 4 (word)
	 * 0b10: 2 (half-word)
	 * 0b11: 8 (double-word)
	 */

	insn = (insn & 0x00180000) >> 19;

	if(!insn)
		return 4;

	if(insn == 2)
		return 2;

	return 8;
}

static uint32_t get_reg_value(struct pt_regs *regs, int reg)
{
	struct sparc_stackf *sf;


	if (!reg)
		return 0;	/* %g0 */

	if(reg < 0xf)
		return regs->u_regs[reg];

	/* must access the previous frame */
	sf = (struct sparc_stackf *) regs->u_regs[UREG_FP];

	reg -= 16;

	if (reg < 8)
		return sf->locals[reg];

	reg -= 8;

	if (reg < 6)
		return sf->ins[reg];

	if (reg == 6)
		return (int) sf->fp;

	return sf->callers_pc;

}


static uint32_t mna_get_addr(struct pt_regs *regs, uint32_t insn)
{
	int i;
	int rd;
	int rs1;
	int rs2 = 0;
	int simm13;


	i   = (insn & 0x00002000) >> 13;
	rd  = (insn & 0x03E000000) >> 25;
        rs1 = (insn & 0x00007C000) >> 14;

	/* operand or immediate ? */
	if (i) {
	        simm13 = insn & 0x00001FFF;
		/* need to extend sign over whole upper 19 bits
		 * this works since this is a signed int
		 */
		simm13 = (simm13 << 19) >> 19;
	} else {
		rs2 = insn & 0x0000201F;
	}

	/*
	 * the SAVE_ALL whenever a trap is entered only stores
	 * the global and input registers of the particular window, so
	 * locals and out registers of the current frame are not necessarily
	 * in sync with the in-memory stack
	 * this means that if any destination register rd and any of the
	 * source registers rs1 and rs2 refer to and address > 0xF, we must
	 * first execute a windows flush before we attempt to extract the
	 * actual address of the attempted memory access
	 * remember %g0-%g7 are synonymous to %r00-%r07, while %o0-%o7
	 * correspond ot %r08-%r15, %l0-%l7 to %r16-%r23, and %i0-%l7 to
	 * %r24-%31
	 *
	 * note that in case this instruction encodes a 13 bit sign-extended
	 * immediate, we ignore rs2
	 */
	if (rd > 0xF || rs1 > 0xF || rs2 > 0xF)
		leon_reg_win_flush();


	if (i)
		return get_reg_value(regs, rs1) + simm13;

	return get_reg_value(regs, rs1) + get_reg_value(regs, rs2);
}


/**
 * @note dst is the address of the particular destination register in the
 *	 stack frame and therefore by design always aligned
 */

static void mna_load(uint8_t *dst, uint8_t *src, int size, int sign)
{
	int i;
	int hw;


	if (size == 2) {
		hw = (src[0] << 8) | src[1];

		if (sign) /* extend to 32 bits */
			hw = (hw << 16) >> 16;

		/* must store register equivalent */
		src  = (uint8_t *)&hw;
		size = 4;
	}

	for (i = 0; i < size; i++)
		dst[i] = src[i];
}


static void mna_store(uint8_t *dst, uint8_t *src, int size)
{
	int i;


	/* for half words, we only take the 2 LSBs */
	if (size == 2)
		src += 2;

	for (i = 0; i < size; i++)
		dst[i] = src[i];
}



void kernel_mna_trap(struct pt_regs *regs, uint32_t insn)
{
	int type;
	int size;
	int sign = 0;

	uint32_t rd;
	uint32_t addr;

	uint32_t g0[2] = {0};


	type = mna_can_handle(insn);
	if (type < 0) {
		pr_err("Unsupported unaligned access at pc %08x\n", regs->pc);
		panic(); /* we're boned */
	}

	size = mna_get_access_size(insn);
	if (size == 2)	/* only half-wors have relevant signedness */
		sign = (insn & 0x400000) != 0;

	addr = mna_get_addr(regs, insn);

	/* update stats */
	mna_cnt++;
	mna_pc_last = regs->pc;
	mna_addr_last = addr;

	/* get register holding src/dest argument */
      	rd = (insn & 0x03E000000) >> 25;

	if (type == 1) {
		mna_load((uint8_t *)get_reg_value(regs, rd), (uint8_t *)addr, size, sign);
		goto exit;
	}

	if (rd) {
		mna_store((uint8_t *)addr, (uint8_t *)get_reg_value(regs, rd), size);
		goto exit;
	}

	/* source is %g0 */
	if (size == 8)
		g0[1] = get_reg_value(regs, 1);	/* also need %g1 for ldd */

	mna_store((uint8_t *)addr, (uint8_t *)&g0, size);

exit:
	regs->pc   = regs->npc;
	regs->npc += 4;
}

void user_mna_trap(struct pt_regs *regs, uint32_t insn)
{
	/* XXX TODO: need special handler for trap from user mode? */
	kernel_mna_trap(regs, insn);
}
