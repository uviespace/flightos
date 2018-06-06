/**
 * @file asm/leon.h
 *
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date    2015
 *
 * @copyright GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief assembly functions for the leon3 target
 *
 */

#ifndef _SPARC_ASM_LEON_H_
#define _SPARC_ASM_LEON_H_



#define ASI_LEON3_SYSCTRL	0x02

#define ASI_LEON3_SYSCTRL_CCR	0x00
#define ASI_LEON3_SYSCTRL_ICFG	0x08
#define ASI_LEON3_SYSCTRL_DCFG	0x0c



__attribute__((unused))
static inline unsigned long leon3_asr17(void)
{
	unsigned long asr17;

	__asm__ __volatile__ (
			"rd	%%asr17, %0	\n\t"
			:"=r" (asr17)
			);

	return asr17;
}

__attribute__((unused))
static inline unsigned long leon3_cpuid(void)
{
	unsigned long cpuid;

	__asm__ __volatile__ (
			"rd	%%asr17, %0	\n\t"
			"srl	%0, 28, %0	\n\t"
			:"=r" (cpuid)
			:
			:"l1");
        return cpuid;
}

__attribute__((unused))
static inline void leon3_powerdown_safe(unsigned long phys_memaddr)
{
	__asm__ __volatile__(
			"wr	%%g0, %%asr19	\n\t"
			"lda	[%0] 0x1c, %%g0	\n\t"
			:
			:"r" (phys_memaddr)
			:"memory");
}

__attribute__((unused))
static inline void leon3_flush(void)
{
	__asm__ __volatile__(
			"flush			\n\t"
			"set	0x81000f, %%g1	\n\t"
			"sta	%%g1, [%0] %1	\n\t"
			:
			: "r" (ASI_LEON3_SYSCTRL_CCR),
			  "i" (ASI_LEON3_SYSCTRL)
			: "g1");
}

__attribute__((unused))
static inline void leon3_enable_icache(void)
{
	__asm__ __volatile__(
			"lda	[%0] %1, %%l1		\n\t"
			"set	0x3,     %%l2		\n\t"
			"or	%%l2,	  %%l1,	%%l2	\n\t"
			"sta	%%l2, [%0] %1		\n\t"
			:
			: "r" (ASI_LEON3_SYSCTRL_CCR),
			  "i" (ASI_LEON3_SYSCTRL)
			: "l1", "l2");
}

__attribute__((unused))
static inline void leon3_enable_dcache(void)
{
	__asm__ __volatile__(
			"lda	[%0] %1, %%l1		\n\t"
			"set	0xc,     %%l2		\n\t"
			"or	%%l2,	  %%l1,	%%l2	\n\t"
			"sta	%%l2, [%0] %1		\n\t"
			:
			: "r" (ASI_LEON3_SYSCTRL_CCR),
			  "i" (ASI_LEON3_SYSCTRL)
			: "l1", "l2");
}


__attribute__((unused))
static inline void leon3_enable_snooping(void)
{
	__asm__ __volatile__(
			"lda	[%0] %1, %%l1		\n\t"
			"set	0x800000, %%l2		\n\t"
			"or	%%l2,	  %%l1,	%%l2	\n\t"
			"sta	%%l2, [%0] %1		\n\t"
			:
			: "r" (ASI_LEON3_SYSCTRL_CCR),
			  "i" (ASI_LEON3_SYSCTRL)
			: "l1", "l2");
}

__attribute__((unused))
static inline void leon3_enable_fault_tolerant(void)
{
	__asm__ __volatile__(
			"lda	[%0] %1, %%l1		\n\t"
			"set	0x80000, %%l2		\n\t"
			"or	%%l2,	  %%l1,	%%l2	\n\t"
			"sta	%%l2, [%0] %1		\n\t"
			:
			: "r" (ASI_LEON3_SYSCTRL_CCR),
			  "i" (ASI_LEON3_SYSCTRL)
			: "l1", "l2");
}




__attribute__((unused))
static inline void leon_set_sp(unsigned long stack_addr)
{
	__asm__ __volatile__(
			"mov %0, %%sp\n\t"
			:
			:"r"(stack_addr)
			:"memory");
}

__attribute__((unused))
static inline void leon_set_fp(unsigned long stack_addr)
{
	__asm__ __volatile__(
			"mov %0, %%fp\n\t"
			:
			:"r" (stack_addr)
			:"memory");
}


__attribute__((unused))
static inline unsigned long leon_get_sp(void)
{
	unsigned long sp;

	__asm__ __volatile__ (
			"mov	%%sp, %0 \n\t"
			:"=r" (sp)
			);

	return sp;
}


__attribute__((unused))
static inline unsigned long leon_get_fp(void)
{
	unsigned long fp;

	__asm__ __volatile__ (
			"mov	%%fp, %0 \n\t"
			:"=r" (fp)
			);

	return fp;
}


__attribute__((unused))
static inline void leon_reg_win_flush(void)
{
	/* BCC/libgloss provide such functionality via SW trap 3, so does Linux
	 * and now we do too...
	 **/
	__asm__ __volatile__("ta 3");
}




#endif /* _SPARC_ASM_LEON_H_ */
