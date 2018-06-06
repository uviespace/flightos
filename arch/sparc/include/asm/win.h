/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SPARC_WIN_H
#define _SPARC_WIN_H


/* register window offsets */
#define RW_L0     0x00
#define RW_L1     0x04
#define RW_L2     0x08
#define RW_L3     0x0C
#define RW_L4     0x10
#define RW_L5     0x14
#define RW_L6     0x18
#define RW_L7     0x1C

#define RW_I0     0x20
#define RW_I1     0x24
#define RW_I2     0x28
#define RW_I3     0x2C
#define RW_I4     0x30
#define RW_I5     0x34
#define RW_I6     0x38
#define RW_I7     0x3C


#define LOAD_WINDOW(reg) \
	ldd	[%reg + RW_L0], %l0; \
	ldd	[%reg + RW_L2], %l2; \
	ldd	[%reg + RW_L4], %l4; \
	ldd	[%reg + RW_L6], %l6; \
	ldd	[%reg + RW_I0], %i0; \
	ldd	[%reg + RW_I2], %i2; \
	ldd	[%reg + RW_I4], %i4; \
	ldd	[%reg + RW_I6], %i6;

#define STORE_WINDOW(reg) \
	std	%l0, [%reg + RW_L0]; \
	std	%l2, [%reg + RW_L2]; \
	std	%l4, [%reg + RW_L4]; \
	std	%l6, [%reg + RW_L6]; \
	std	%i0, [%reg + RW_I0]; \
	std	%i2, [%reg + RW_I2]; \
	std	%i4, [%reg + RW_I4]; \
	std	%i6, [%reg + RW_I6];


/* LOAD/STORE of trap frame */

#define LOAD_PT_INS(base_reg) \
        ldd     [%base_reg + STACKFRAME_SZ + PT_I0], %i0; \
        ldd     [%base_reg + STACKFRAME_SZ + PT_I2], %i2; \
        ldd     [%base_reg + STACKFRAME_SZ + PT_I4], %i4; \
        ldd     [%base_reg + STACKFRAME_SZ + PT_I6], %i6;

#define LOAD_PT_GLOBALS(base_reg) \
        ld      [%base_reg + STACKFRAME_SZ + PT_G1], %g1; \
        ldd     [%base_reg + STACKFRAME_SZ + PT_G2], %g2; \
        ldd     [%base_reg + STACKFRAME_SZ + PT_G4], %g4; \
        ldd     [%base_reg + STACKFRAME_SZ + PT_G6], %g6;

#define LOAD_PT_YREG(base_reg, scratch) \
        ld      [%base_reg + STACKFRAME_SZ + PT_Y], %scratch; \
        wr      %scratch, 0x0, %y;

#define LOAD_PT_PRIV(base_reg, pt_psr, pt_pc, pt_npc) \
        ld      [%base_reg + STACKFRAME_SZ + PT_PSR], %pt_psr; \
        ld      [%base_reg + STACKFRAME_SZ + PT_PC], %pt_pc; \
        ld      [%base_reg + STACKFRAME_SZ + PT_NPC], %pt_npc;

#define LOAD_PT_ALL(base_reg, pt_psr, pt_pc, pt_npc, scratch) \
        LOAD_PT_YREG(base_reg, scratch) \
        LOAD_PT_INS(base_reg) \
        LOAD_PT_GLOBALS(base_reg) \
        LOAD_PT_PRIV(base_reg, pt_psr, pt_pc, pt_npc)




#define STORE_PT_INS(base_reg) \
        std     %i0, [%base_reg + STACKFRAME_SZ + PT_I0]; \
        std     %i2, [%base_reg + STACKFRAME_SZ + PT_I2]; \
        std     %i4, [%base_reg + STACKFRAME_SZ + PT_I4]; \
        std     %i6, [%base_reg + STACKFRAME_SZ + PT_I6];

#define STORE_PT_GLOBALS(base_reg) \
        st      %g1, [%base_reg + STACKFRAME_SZ + PT_G1]; \
        std     %g2, [%base_reg + STACKFRAME_SZ + PT_G2]; \
        std     %g4, [%base_reg + STACKFRAME_SZ + PT_G4]; \
        std     %g6, [%base_reg + STACKFRAME_SZ + PT_G6];

#define STORE_PT_YREG(base_reg, scratch) \
        rd      %y, %scratch; \
        st      %scratch, [%base_reg + STACKFRAME_SZ + PT_Y];

#define STORE_PT_PRIV(base_reg, pt_psr, pt_pc, pt_npc) \
        st      %pt_psr, [%base_reg + STACKFRAME_SZ + PT_PSR]; \
        st      %pt_pc,  [%base_reg + STACKFRAME_SZ + PT_PC]; \
        st      %pt_npc, [%base_reg + STACKFRAME_SZ + PT_NPC];

#define STORE_PT_ALL(base_reg, reg_psr, reg_pc, reg_npc, g_scratch) \
        STORE_PT_PRIV(base_reg, reg_psr, reg_pc, reg_npc) \
        STORE_PT_GLOBALS(base_reg) \
        STORE_PT_YREG(base_reg, g_scratch) \
        STORE_PT_INS(base_reg)



#endif /* _SPARC_WIN_H */
