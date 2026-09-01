/**
 * @file arch/sparc/lib/floatdidf.c
 *
 * @brief conversion of a 64-bit signed integer to a double
 */

#define WORD_SIZE (sizeof (unsigned int) * 8)
#define HIGH_HALFWORD_COEFF (((unsigned long long) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((unsigned long long) 1) << WORD_SIZE)


/**
 * @brief convert a 64-bit signed integer to a double
 *
 * @param i the 64-bit signed integer to convert
 *
 * @return the double representation of i
 */

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
