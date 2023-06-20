/**
 * @file arch/sparc/kernel/traps/fpe.c
 * @author Roland Ottensamer (roland.ottensamer@univie.ac.at),
 *	   Armin Luntzer     (armin.luntzer@univie.ac.at)
 */


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
