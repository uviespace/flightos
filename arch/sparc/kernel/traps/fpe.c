/**
 * @file arch/sparc/kernel/traps/fpe.c
 * @author Roland Ottensamer (roland.ottensamer@univie.ac.at),
 *	   Armin Luntzer     (armin.luntzer@univie.ac.at)
 */



#include <kernel/sysctl.h>
#include <kernel/string.h>
#include <kernel/init.h>


#if (__sparc__)
#define UINT32_T_FORMAT		"%lu"
#else
#define UINT32_T_FORMAT		"%u"
#endif


static unsigned long fpe_trap_last_pc_addr;

static ssize_t fpe_trap_show(__attribute__((unused)) struct sysobj *sobj,
			     __attribute__((unused)) struct sobj_attribute *sattr,
			     char *buf)
{
	if (!strcmp(sattr->name, "pc"))
		return sprintf(buf, UINT32_T_FORMAT, fpe_trap_last_pc_addr);

	return 0;
}


__extension__
static struct sobj_attribute last_trap_pc_attr = __ATTR(pc,
							fpe_trap_show,
							NULL);
__extension__
static struct sobj_attribute *fpe_trap_attributes[] = {&last_trap_pc_attr,
						       NULL};



/**
 * @brief initialise the sysctl entries for the fpe_trap
 *
 * @return -1 on error, 0 otherwise
 *
 * @note we set this up as a late initcall since we need sysctl to be
 *	 configured first
 */

static int fpe_trap_init_sysctl(void)
{
	struct sysobj *sobj;


	sobj = sysobj_create();

	if (!sobj)
		return -1;

	sobj->sattr = fpe_trap_attributes;

	sysobj_add(sobj, NULL, sysctl_root(), "fpe_trap");

	return 0;
}
late_initcall(fpe_trap_init_sysctl);



/* depth of the FP queue
 * XXX this is implementation specific, fixup for supported platforms
 */

#define MAX_FQ 8

/*
 * @brief high level floating point exception trap handler.
 */

void fpe_trap(void)
{
	unsigned int i;
	unsigned int fsrval = 0;

	double d;


	/* for convenience */
#define CURE_F_REG(num) do {						\
	volatile unsigned int value = 0;				\
		__asm__ __volatile__("st %%f" #num ", [%0] \n\t"	\
			     "nop; nop \n\t"				\
			     ::"r"(&value));				\
	if ((value & 0x7fffffff) & ((value & 0x7F800000) == 0)) {	\
		__asm__ __volatile__("ld [%%g0], %%f" #num " \n\t"	\
				     :::);				\
	}								\
} while(0);


__diag_push();
__diag_ignore(GCC, 7, "-Wframe-address", "we're fully aware that __builtin_return_address can be problematic");
	/* update offending %pc */
	fpe_trap_last_pc_addr = (unsigned long) __caller(1);
__diag_pop()

	/* the FQ must be emptied until the FSR says empty */
	for (i = 0; i < MAX_FQ; i++) {

		/* read FSR */
		__asm__ __volatile__("st %%fsr, [%0] \n\t"
				     "nop; nop \n\t"
				     ::"r"(&fsrval));

		/* FQ is set in FSR */
		if (fsrval & 0x00002000) {
			__asm__ __volatile__("std %%fq, [%0] \n\t"
					     "nop; nop \n\t"
					     ::"r"(&d));
			continue;
		}

		break;
	}

	/* cure all denormalized numbers in the 32 f-registers by checking
	 * if (for nonzero numbers) the exponent is all 0, then setting to 0.0
	 * we don't check the instruction that caused this and simply treat
	 * everything as 4-byte floats
	 */

	CURE_F_REG(0);
	CURE_F_REG(1);
	CURE_F_REG(2);
	CURE_F_REG(3);
	CURE_F_REG(4);
	CURE_F_REG(5);
	CURE_F_REG(6);
	CURE_F_REG(7);
	CURE_F_REG(8);
	CURE_F_REG(9);
	CURE_F_REG(10);
	CURE_F_REG(11);
	CURE_F_REG(12);
	CURE_F_REG(13);
	CURE_F_REG(14);
	CURE_F_REG(15);
	CURE_F_REG(16);
	CURE_F_REG(17);
	CURE_F_REG(18);
	CURE_F_REG(19);
	CURE_F_REG(20);
	CURE_F_REG(21);
	CURE_F_REG(22);
	CURE_F_REG(23);
	CURE_F_REG(24);
	CURE_F_REG(25);
	CURE_F_REG(26);
	CURE_F_REG(27);
	CURE_F_REG(28);
	CURE_F_REG(29);
	CURE_F_REG(30);
	CURE_F_REG(31);
}
