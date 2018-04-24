/* SPDX-License-Identifier: GPL-2.0 */


#include <asm/ttable.h>



.section	".head.text","ax"

        .global  _start
_start:

	.global _stext
_stext:

	.global  trapbase
trapbase:

#ifdef CONFIG_SMP
trapbase_cpu0:
#endif /* CONFIG_SMP */

	/* hardware traps (128 total) */
	! 0x00 reset trap (alternative: b gokernel; nop; nop; nop)
	b kernel_entry	! branch once on cold start, post setup inserts rd %tbr/%psr (see TRAP_EXCEPTION)
	 nop			! instructions to raise exception on warm reset
	ba __exception_entry
         and    %l3, TTBL_MASK, %l4

	SRMMU_TFAULT
	TRAP_ENTRY(SPARC_TRAP_II, ill_instruction_trap_handler)
	TRAP_ENTRY(SPARC_TRAP_PI, priv_instruction_trap_handler)
	TRAP_ENTRY(SPARC_TRAP_FPD, fpdis_trap_handler)
	WINDOW_OFLOW
	WINDOW_UFLOW
	TRAP_ENTRY(SPARC_TRAP_MNA, mna_trap_handler)
	TRAP_ENTRY(SPARC_TRAP_FPE, fpe_trap_handler)
	SRMMU_DFAULT		! Data Fault 
	TRAP_ENTRY(SPARC_TRAP_TOF, tag_overflow_trap_handler)
	TRAP_ENTRY(SPARC_TRAP_WPDT, watchpoint_det_trap_handler)

	.rept 5
	TRAP_EXCEPTION	! bad traps
	.endr

	TRAP_INTERRUPT(1)
	TRAP_INTERRUPT(2)
	TRAP_INTERRUPT(3)
	TRAP_INTERRUPT(4)
	TRAP_INTERRUPT(5)
	TRAP_INTERRUPT(6)
	TRAP_INTERRUPT(7)
	TRAP_INTERRUPT(8)
	TRAP_INTERRUPT(9)
	TRAP_INTERRUPT(10)
	TRAP_INTERRUPT(11)
	TRAP_INTERRUPT(12)
	TRAP_INTERRUPT(13)
	TRAP_INTERRUPT(14)
	TRAP_ENTRY(15, nmi_entry)	! treat NMI interrupt differntly

	TRAP_ENTRY(SPARC_TRAP_RAC, reg_access_trap_handler)
	TRAP_EXCEPTION		!  Instruction Access Error
	TRAP_EXCEPTION		!  bad trap
	TRAP_EXCEPTION		!  bad trap
	TRAP_ENTRY(SPARC_TRAP_CPDIS, cpdis_trap_handler)
	TRAP_ENTRY(SPARC_TRAP_BADFL, unimpl_flush_trap_handler)
	TRAP_EXCEPTION		!  bad trap
	TRAP_EXCEPTION		!  bad trap
	TRAP_ENTRY(SPARC_TRAP_CPEX, cpex_trap_handler)
	SRMMU_DFAULT		!  Data Access Error
	TRAP_ENTRY(SPARC_TRAP_DIVZ, hw_div0_trap_handler)
	TRAP_EXCEPTION		!  Data Store Error
	TRAP_EXCEPTION		!  Data Access MMU Miss

	.rept 15
	TRAP_EXCEPTION		! bad traps
	.endr

	TRAP_EXCEPTION		! Instruction Access MMU Miss

	.rept 67
	TRAP_EXCEPTION		! bad traps
	.endr

	/* software traps (128 total) */
	SYSCALL_TRAP		! 0x80 = SunOS syscall()
	TRAP_EXCEPTION		! 0x81
	TRAP_EXCEPTION		! 0x82
	TRAP_EXCEPTION		! 0x83
	TRAP_EXCEPTION		! 0x84
	TRAP_EXCEPTION		! 0x85

	.rept 122
	TRAP_EXCEPTION				! 0x86-0xFF unused
	.endr

	.globl	end_traptable
end_traptable:





#ifdef CONFIG_SMP
	.globl	trapbase_cpu1
trapbase_cpu1:

	/* XXX: stuff */

#endif /* CONFIG_SMP */


        .global  leonbare_trapsetup
leonbare_trapsetup:
	.global catch_interrupt
catch_interrupt:
	.global coldboot_reset
coldboot_reset:
	.global __exception_entry
__exception_entry:
	.global srmmu_fault
srmmu_fault:
	.global ill_instruction_trap_handler
ill_instruction_trap_handler:
	.global priv_instruction_trap_handler
priv_instruction_trap_handler:


	.global leonbare_trapreturn
leonbare_trapreturn:
	.global fpdis_trap_handler
fpdis_trap_handler:
	.global __wim_overflow
__wim_overflow:
	.global __wim_underflow
__wim_underflow:
	.global mna_trap_handler
mna_trap_handler:
	.global fpe_trap_handler
fpe_trap_handler:
	.global tag_overflow_trap_handler
tag_overflow_trap_handler:
	.global mna_trap_handler
mna_trap_handler:
	.global cpdis_trap_handler
cpdis_trap_handler:
	.global cpex_trap_handler
cpex_trap_handler:
	.global fpe_trap_handler
fpe_trap_handler:
	.global hw_div0_trap_handler
hw_div0_trap_handler:
	.global __interrupt_entry
__interrupt_entry:
	.global __clzsi2
__clzsi2:
	.global memset
memset:
	.global nmi_entry
nmi_entry:
	.global reg_access_trap_handler
reg_access_trap_handler:
	.global strchr
strchr:
	.global strtol
strtol:
	.global syscall_tbl
syscall_tbl:
	.global syscall_trap
syscall_trap:
	.global tag_overflow_trap_handler
tag_overflow_trap_handler:
	.global .udiv
.udiv:
	.global .umul
.umul:
	.global unimpl_flush_trap_handler
unimpl_flush_trap_handler:
	.global vprintf
vprintf:
	.global vsnprintf
vsnprintf:
	.global watchpoint_det_trap_handler
watchpoint_det_trap_handler:
