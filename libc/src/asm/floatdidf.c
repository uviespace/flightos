#include <compiler.h>

__diag_push()
__diag_ignore(GCC, 7, "-Wlong-long", "we need this for 64 bit types")

#define WORD_SIZE (sizeof (unsigned int) * 8)
#define HIGH_HALFWORD_COEFF (((unsigned long long) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((unsigned long long) 1) << WORD_SIZE)


double __floatdidf(long long i)
{
	double d;
	int neg = 0;

	if (i < 0) {
		i = -i;
		neg = 1;
	}

	d = (unsigned int) (i >> WORD_SIZE);
	d *= HIGH_HALFWORD_COEFF;
	d *= HIGH_HALFWORD_COEFF;
	d += (unsigned int) (i & (HIGH_WORD_COEFF - 1));

	if (neg)
		d = -d;

	return d;
}

__diag_pop()	/* -Wlong-long */
