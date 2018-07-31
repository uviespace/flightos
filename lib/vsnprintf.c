/* SPDX-License-Identifier: GPL-2.0 */
/*+
 * @file lib/vsnprint.c
 *
 * @ingroup string
 *
 * @brief implements vsnprintf()
 *
 * @note we split this out, otherwise string.c would be way too messy
 *
 * a format specifier looks like this:
 * %[flags][width][.precision][length]type
 *
 * - we do not support the POSIX parameter field extension
 * - we support only a subset of length field flags (l, h, hh)
 * - we support a %b base-2 type, because sometimes its nice to print bitfields
 * - %f specifiers are forced to %g if the floating-point value is > 2^32-1
 *
 * - happy bug hunting, I bet there are lots
 */

#include <kernel/kmem.h>
#include <kernel/export.h>
#include <kernel/types.h>
#include <kernel/string.h>
#include <kernel/bitops.h>


#define STACK_BUF_SIZE 32



/* leading flags */
#define VSN_LEFT	(1 << 0)	/* left-align output */
#define VSN_PLUS	(1 << 1)	/* show leading plus */
#define VSN_SPACE	(1 << 2)	/* show space if plus */
#define VSN_ZEROPAD	(1 << 3)	/* pad with zero */
#define VSN_HASH	(1 << 4)	/* trailing zeros, decimal points, 0x */

/* precision field flag */
#define VSN_PRECISION	(1 << 5)	/* post-comma precision */

/* length field flags */
#define VSN_CHAR	(1 << 6)	/* arg is int, upcast from char */
#define VSN_SHORT	(1 << 7)	/* arg is int, upcast from short */
#define VSN_LONG	(1 << 8)	/* arg is long type */
#define VSN_LONG_LONG	(1 << 10)	/* arg is long long type */

/* type flags */
#define VSN_UPPERCASE	(1 << 9)	/* %x or %X */


struct fmt_spec {
	unsigned int flags;
	unsigned int width;
	unsigned int base;
	unsigned int prec;
};



#define TREADY 4

static volatile int *console = (int *)0x80000100;

static int putchar(int c)
{
	while (!(console[1] & TREADY));

	console[0] = 0x0ff & c;

	if (c == '\n') {
		while (!(console[1] & TREADY));
		console[0] = (int) '\r';
	}

	return c;
}



/**
 * @brief print a single character
 *
 * @note if str is NULL, this prints to stdout
 *
 * @note putchar() must be provided by the architecture in some form
 *	 e.g. this could be implemented in a kernel module for more efficient
 *	 use of the 8-byte UART FIFO on the GR712RC and patched after
 *	 the initial boot process
 */

static void _sprintc(int c, char **str, const char *end)
{
	if (end) {
		if ((char *) str > end)
			return;
	}

	if (str) {
		(**str) = (char) c;
		(*str)++;
	} else {
		putchar(c);
	}
}


/**
 * @brief print a one or more character to str
 *
 * @param str	the destionation buffer
 * @param buf	the source buffer
 * @param n	the number of bytes to write
 *
 * @returns the number of bytes written
 *
 * @note if str is NULL, this prints to stdout
 *
 * @note putchar() must be provided by the architecture in some form
 *	 e.g. this could be implemented in a kernel module for more efficient
 *	 use of the 8-byte UART FIFO on the GR712RC and patched after
 *	 the initial boot process
 */

static size_t _printn(char **str, const char *buf, size_t n)
{
	size_t i;

	if (str) {
		memcpy((*str), buf, n);
	} else {
		for (i = 0; i < n; i++)
			putchar(buf[i]);
	}

	return n;
}


/**
 * @brief the result of an incestuous relationship of atoi() and strtol()
 *
 * @param nptr	the pointer to a string (possibly) representing a number
 * @param endptr if not NULL, stores the pointer to the first character not part
 *		 of the number
 *
 * @return the converted number
 *
 * @note if no number has been found, the function will return 0 and
 *	 nptr == endptr
 *
 */

static int _vsn_strtoi(const char *nptr, char **endptr)
{
	int res = 0;
	int neg = 0;

	unsigned int d = -1;

	const char *start = nptr;



	for (; isspace(*nptr); ++nptr);

	if (*nptr == '-') {
		nptr++;
		neg = 1;
	} else if (*nptr == '+') {
		nptr++;
	}

	while (1) {
		d = (*nptr) - '0';

		if (d > 9)
			break;

		res = res * 10 + (int) d;
		nptr++;
	}

	if (neg)
		res = -res;

	if (endptr) {
		if (d == -1)
			(*endptr) = (char *) start;
		else
			(*endptr) = (char *) nptr;
	}

	return res;
}


/**
 * @brief evaluate any flags in the fmt string
 *
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 *
 * @returns	the number of bytes read
 */

static size_t eval_flags(const char *fmt, struct fmt_spec *spec)
{
	bool done = false;

	const char *begin = fmt;


	spec->flags = 0;

	do {
		switch ((*fmt)) {
		case '-':
			spec->flags |= VSN_LEFT;
			fmt++;
			break;

		case '+':
			spec->flags |= VSN_PLUS;
			fmt++;
			break;

		case ' ':
			spec->flags |= VSN_SPACE;
			fmt++;
			break;

		case '0':
			spec->flags |= VSN_ZEROPAD;
			fmt++;
			break;

		case '#':
			spec->flags |= VSN_HASH;
			fmt++;
			break;

		default:
			done = true;
			break;
		}

	} while (!done);


	return (size_t) (fmt - begin);
}


/**
 * @brief evaluate the width field
 *
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 * @param args the arguments to the format string
 *
 * @returns	the number of bytes read
 */

static size_t eval_width(const char *fmt, struct fmt_spec *spec, va_list *args)
{
	int w;

	char *end;

	const char *begin = fmt;


	if (isdigit((*fmt))) {

		w = _vsn_strtoi(fmt, &end);
		fmt = (const char *) end;

		/* leading zeroes evaluate as zero-pad flag */
		if (!w)
			spec->flags |= VSN_ZEROPAD;

	} else if ((*fmt) == '*') {

		/* the width is part of the argument */
		w = (int) va_arg((*args), int);

		if (w < 0) {
			spec->flags |= VSN_LEFT;
			w = -w;
		}

		fmt++;
	}

	spec->width = (unsigned int) w;

	return (size_t) (fmt - begin);
}


/**
 * @brief evaluate the precision field
 *
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 * @param args the arguments to the format string
 *
 * @returns	the number of bytes read
 */

static size_t eval_prec(const char *fmt, struct fmt_spec *spec, va_list *args)
{
	char *end;

	const char *begin = fmt;


	/* default */
	spec->prec = 0;

	if ((*fmt) != '.')
		return 0;


	spec->flags |= VSN_PRECISION;
	fmt++;

	if (isdigit((*fmt))) {

		spec->prec = _vsn_strtoi(fmt, &end);
		fmt = (const char *) end;

	} else if ((*fmt) == '*') {

		spec->prec = va_arg((*args), unsigned int);
		fmt++;
	}

	return (size_t) (fmt - begin);
}


/**
 * @brief evaluate the length field
 *
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 *
 * @returns	the number of bytes read
 */

static size_t eval_length(const char *fmt, struct fmt_spec *spec)
{
	const char *begin = fmt;


	switch ((*fmt)) {
	case 'h':

		fmt++;
		if ((*fmt) == 'h') {
			spec->flags |= VSN_CHAR;
			fmt++;
		} else {
			spec->flags |= VSN_SHORT;
		}

		break;

	case 'l':
		/* we don't support long long specifiers */
		if (fmt[1] == 'l') {
			spec->flags |= VSN_LONG_LONG;
			fmt++;
		} else {
			spec->flags |= VSN_LONG;
		}
		fmt++;
		break;

	default:
		break;
	}

	return (size_t) (fmt - begin);
}


/**
 * @brief configure flags and set base for specifier
 *
 * @param fmt	a format string buffer
 */

static void config_specifier(const char *fmt, struct fmt_spec *spec)
{
	switch ((*fmt)) {

	case 'd':
		spec->flags &= ~(VSN_HASH);
	case 'i':
		spec->base = 10;
		break;

	case 'u':
		spec->base = 10;
		spec->flags &= ~(VSN_PLUS | VSN_SPACE | VSN_HASH);
		break;

	case 'p':
	case 'x':
		spec->base = 16;
		spec->flags &= ~(VSN_PLUS | VSN_SPACE);
		break;

	case 'X':
		spec->base = 16;
		spec->flags |= VSN_UPPERCASE;
		spec->flags &= ~(VSN_PLUS | VSN_SPACE);
		break;

	case 'o':
		spec->base = 8;
		spec->flags &= ~(VSN_PLUS | VSN_SPACE);
		break;

	case 'b':
		spec->base = 2;
		spec->flags &= ~(VSN_PLUS | VSN_SPACE);
		break;

	default:
		spec->base = 0;
		break;
	}
}


/**
 * @brief
 *
 * @param str	the destination buffer, NULL to print to stdout
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_final(char **str, const char *end, bool sign,
			   char *buf, size_t n, struct fmt_spec *spec)
{
	size_t i;
	size_t len = 0;

	/* first, pad */
	if (!(spec->flags & VSN_LEFT)) {
		for (; n < STACK_BUF_SIZE && n < spec->prec; n++)
			buf[n] = '0';

		if (spec->flags & VSN_ZEROPAD) {
			for (; n < STACK_BUF_SIZE && n < spec->width; n++)
				buf[n] = '0';
		}
	}


	/* now treat the hash */
	if (spec->flags & VSN_HASH) {

		if (n) {
			if ((n == spec->prec) || (n == spec->width)) {

				n--;

				if (n && (spec->base == 16))
					n--;
			}
		}

		if (n < STACK_BUF_SIZE) {
			if (spec->base == 16) {
				if (spec->flags & VSN_UPPERCASE)
					buf[n++] = 'x';
				else
					buf[n++] = 'X';
			}
		}

		if (n < STACK_BUF_SIZE)
			buf[n++] = '0';
	}


	/* now the sign */
	if (sign || (spec->flags & VSN_PLUS) || (spec->flags & VSN_SPACE)) {
		if (n == spec->width)
			n--;
	}

	if (n < STACK_BUF_SIZE) {
		if (sign)
			buf[n++] = '-';
		else if (spec->flags & VSN_PLUS)
			buf[n++] = '+';
		else if (spec->flags & VSN_SPACE)
			buf[n++] = ' ';
	}

	/* pad spaces to given width */
	if (!(spec->flags & (VSN_LEFT | VSN_ZEROPAD))) {
		for (i = n; i < spec->width; i++) {
			_sprintc(' ', (char **) str, end);
			len++;
		}
	}

	/* reverse buffer */
	for (i = 0; i < n; i++) {
		_sprintc(buf[n-i-1], (char **) str, end);
		len++;
	}

	// append pad spaces up to given width
	if (spec->flags & VSN_LEFT) {
		while (len < spec->width) {
			_sprintc(' ', (char **) str, end);
			len++;
		}
	}

	return len;
}


/**
 * @brief
 *
 * @param usign whether the value is treated as signed or unsigned
 * @param str	the destination buffer, NULL to print to stdout
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_xlong_to_ascii(bool usign, long value, char **str,
				    const char *end, struct fmt_spec *spec)
{
	size_t n = 0;

	bool sign = false;

	char digit;
	char buf[STACK_BUF_SIZE];


	if (!usign) {
		if (value < 0) {
			sign = true;
			value = -value;
		}
	}

	do {

		digit = (char) (value % spec->base);

		if (digit < 10) {

			buf[n] = '0' + digit;

		} else { /* base 16 hex (we assume) */
			if (spec->flags & VSN_UPPERCASE)
				buf[n] = 'A' + digit - 10;
			else
				buf[n] = 'a' + digit - 10;
		}

		n++;
		value /= spec->base;
	} while (value && (n < STACK_BUF_SIZE));

	return render_final(str, end, sign, buf, n, spec);
}

/**
 * @brief
 *
 * @param usign whether the value is treated as signed or unsigned
 * @param str	the destination buffer, NULL to print to stdout
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_xlong_long_to_ascii(bool usign, long long value, char **str,
				    const char *end, struct fmt_spec *spec)
{
	size_t n = 0;

	long upper, lower;


	upper = (long) (value >> 32) & 0xFFFFFFFFUL;
	lower = (long) (value      ) & 0xFFFFFFFFUL;

	/* only if set, otherwise we have a leading zero */
	if (upper)
		n = render_xlong_to_ascii(usign, upper, str, end, spec);

	n += render_xlong_to_ascii(true, lower, str, end, spec);

	return n;
}



/**
 * @brief render signed integer types
 *
 * @param usign whether the value is treated as signed or unsigned
 * @param str	the destination buffer, NULL to print to stdout
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_xsigned_integer(bool usign, char **str, const char *end,
				     struct fmt_spec *spec, va_list *args)
{
	int  i;
	long l;
	long long ll;


	/* argument is a long long, will turn into a long long */
	if (spec->flags & VSN_LONG_LONG) {
		ll = va_arg((*args), long long);
		return render_xlong_long_to_ascii(usign, ll, str, end, spec);
	}

	/* argument is a long, will turn into a long */
	if (spec->flags & VSN_LONG) {
		l = va_arg((*args), long);
		return render_xlong_to_ascii(usign, l, str, end, spec);
	}

	/* argument is an int, which has been upcast from short */
	if (spec->flags & VSN_SHORT) {
		i = (short int) va_arg((*args), int);
		return render_xlong_to_ascii(usign, (long) i, str, end, spec);
	}

	/* argument is an int, which has been upcast from char */
	if (spec->flags & VSN_CHAR) {
		i = (char) va_arg((*args), int);
		return render_xlong_to_ascii(usign, (long) i, str, end, spec);
	}


	/* argument is an int */
	i = (int) va_arg((*args), int);
	return render_xlong_to_ascii(usign, (long) i, str, end, spec);
}


/**
 * @brief render a pointer
 *
 * @param str	the destination buffer, NULL to print to stdout
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_pointer(char **str, const char *end,
			     struct fmt_spec *spec, va_list *args)
{
	long l;


	l = (long) ((uintptr_t) va_arg((*args), void *));

	return render_xlong_to_ascii(true, l, str, end, spec);
}


/**
 * @brief render integer types
 *
 * @param str	the destination buffer, NULL to print to stdout
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_integer(char **str, const char *end, const char *fmt,
			     struct fmt_spec *spec, va_list *args)
{
	unsigned int n = 0;


	switch ((*fmt)) {
	case 'd':
	case 'i':
		n = render_xsigned_integer(false, str, end, spec, args);
		break;
	case 'u':
	case 'x':
	case 'X':
	case 'o':
	case 'b':
		n = render_xsigned_integer(true, str, end, spec, args);
		break;
	case 'p':
		n = render_pointer(str, end, spec, args);
		break;
	default:
		break;
	}

	return  n;
}


/**
 * @brief renders a mantissa into a buffer
 *
 * @param buf	the destination buffer
 * @param n	the offset into the buffer
 * @param size	the size of the buffer
 *
 * @return the new offset into the buffer
 */

static size_t render_mantissa(char *buf, size_t n, const size_t size,
			      unsigned long mantissa, struct fmt_spec *spec)
{
	int digits = (int) spec->prec;


	while (n < size) {

		buf[n++] = (char)('0' + (mantissa % 10));
		mantissa /= 10;
		digits--;

		if (!mantissa)
			break;
	}

	/* pad with zeroes up to precision */
	while (n < size) {

		if (digits < 1)
			break;

		digits--;
		buf[n++] = '0';
	}

	/* add decimal point */
	if (n < size)
		buf[n++] = '.';


	return n;
}


/**
 * @brief renders a float exponent into a buffer
 *
 * @param buf	the destination buffer
 * @param n	the offset into the buffer
 * @param size	the size of the buffer
 * @param pow	the value of the exponent
 *
 * @return the new offset into the buffer
 */

static size_t render_exponent(char *buf, size_t n, const size_t size,
				    int exp)
{
	int d;

	bool sign = false;



	if (exp < 0) {
		sign = true;
		exp = -exp;
	}

	d = exp;

	while (n < size) {
		buf[n++] = (char)('0' + (d % 10));
		d /= 10;

		if (!d)
			break;
	}

	if (exp < 10)
		if (n < size)
			buf[n++] = '0';

	if (n < size) {
		if (sign)
			buf[n++] = '-';
		else
			buf[n++] = '+';
	}

	if (n < size)
		buf[n++] = 'e';



	return n;
}


/**
 * @brief renders a float characteristic into a buffer
 *
 * @param buf		the destination buffer
 * @param n		the offset into the buffer
 * @param size		the size of the buffer
 * @param characteristic the value of the characteristic
 *
 * @return the new offset into the buffer
 */

static size_t render_characteristic(char *buf, size_t n, const size_t size,
				    unsigned long characteristic)
{
	while (n < size) {
		buf[n++] = (char)('0' + (characteristic % 10));
		characteristic /= 10;

		if (!characteristic)
			break;
	}

	return n;
}


/**
 * @brief adjust a float value and parameters for "%g" format
 *
 * @param value		the value to adjust
 * @param[out] exp	the value of the exponent
 * @param pow_max	the maximum power-of-10
 * @param prec_max	the maximum precision
 * @param spec	a struct fmt_spec
 *
 * @return the adjusted float
 */

static double get_exp_float_val_param(double value, int *exp,
				      const unsigned int pow_max,
				      const unsigned int prec_max,
				      struct fmt_spec *spec)
{
	int e = 0;

	double tmp;



	if (value > 1e6) {
		while (value > 10.0) {
			value *= 0.1;
			e++;
		}

	} else if (value < 1e-4) {
		while (value < 1.0) {
			value *= 10.0;
			e--;
		}
	} else if (value > 1.0) {
		if (!(spec->flags & VSN_PRECISION))
			spec->prec = 0;
	}


	(*exp) = e;

	/* adjust precision to be at most prec_max (stops on first zero) */
	if (!(spec->flags & VSN_PRECISION)) {

		tmp = value - (double) ((int) value);

		spec->prec = 0;

		while (tmp > (1.0 / pow_max) ) {

			if (spec->prec >= prec_max)
				break;

			spec->prec++;

			if (e < 0)
				tmp *= 10.0;
			else
				tmp *= 0.1;

			tmp = tmp - (double) ((int) tmp);
		}
	}


	return value;
}


/**
 * @brief separate and round a float into characteristic and mantissa
 *
 * @param value			the value of the float
 * @param[out] characteristic	the value of the characteristic
 * @param[out] mantissa		the value of the mantissa
 * @param precision		the selected precision
 * @param pow10			the power-of-10 of the selected precision
 */

static void separate_and_round(double value, unsigned long *characteristic,
			       unsigned long *mantissa, unsigned int precision,
			       double pw10)
{

	unsigned long ch;
	unsigned long m;
	double tmp;


	/* left of comma */
	ch = (unsigned long) value;
	tmp = (value - (double) ch) * pw10;

	/* right of comma */
	m = (unsigned long) tmp;


	/* check rounding of digits beyond precision */
	tmp = tmp - (double) m;

	if (tmp > 0.5) {
		m++;
		if (m >= pw10) {
			m = 0;
			ch++;
		}
	} else if (tmp == 0.5) {
		if (!m || (m & 0x1))
		    m++;
	}

	/* special rounding case: precision == 0 */
	if (!precision) {
		tmp = value - (double) ch;

		if (tmp > 0.5)
			ch++;
		else if ((tmp == 0.5) && (ch & 0x1))
			ch++;
	}


	(*characteristic) = ch;
	(*mantissa) = m;
}


/**
 * @brief render floating point types
 *
 * @param str	 the destination buffer, NULL to print to stdout
 * @param end	 the end of the destination buffer
 * @param pr_exp whether to render the float in exponential format
 * @param spec	 a struct fmt_spec
 * @param args	 the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_float(char **str, const char *end, bool pr_exp,
			   struct fmt_spec *spec, va_list *args)
{
	size_t n = 0;

	unsigned long characteristic;
	unsigned long mantissa;

	int exp = 0;

	double val;

	bool sign    = false;

	char buf[STACK_BUF_SIZE];

	/* we use unsigned long for the mantissa, so we'll have overflows for
	 * any power-of-10 > 1e9, as 2^32-1 gives us at most 10 digits
	 */
	static const double pow10[] = {1e0, 1e1, 1e2, 1e3, 1e4,
				       1e5, 1e6, 1e7, 1e8, 1e9};



	val = (double) va_arg((*args), double);

	if (val < 0.0) {
		sign = true;
		val = -val;
	}

	/* set default precision, see ISO C99 specification, 7.19.6.1/7 */
	if (!(spec->flags & VSN_PRECISION))
		spec->prec = 6;

	/* clamp precision */
	if (spec->prec >= ARRAY_SIZE(pow10))
		spec->prec = ARRAY_SIZE(pow10) - 1;

	/* force %g if value would overflow */
	if (val > (double) 0xFFFFFFF)
		pr_exp = true;

	if (pr_exp)
		val = get_exp_float_val_param(val, &exp,
					      pow10[spec->prec],
					      spec->prec, spec);

	separate_and_round(val, &characteristic, &mantissa, spec->prec,
			   pow10[spec->prec]);

	if (exp)
		n = render_exponent(buf, n, STACK_BUF_SIZE, exp);

	if (spec->prec)
		n = render_mantissa(buf, n, STACK_BUF_SIZE, mantissa, spec);

	n = render_characteristic(buf, n, STACK_BUF_SIZE, characteristic);


	return render_final(str, end, sign, buf, n, spec);
}


/**
 * @brief render a single character
 *
 * @param str	 the destination buffer, NULL to print to stdout
 * @param end	 the end of the destination buffer
 * @param spec	 a struct fmt_spec
 * @param args	 the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_char(char **str, const char *end, struct fmt_spec *spec,
			  va_list *args)
{
	size_t n = 1;

	char c = va_arg((*args), int);

	if (!(spec->flags & VSN_LEFT)) {
		for (; n < spec->width; n++)
			_sprintc(' ', (char **) str, end);
	}

	_sprintc(c, (char **) str, end);

	if (spec->flags & VSN_LEFT) {
		for (; n < spec->width; n++)
			_sprintc(' ', (char **) str, end);
	}

	return n;
}


/**
 * @brief render a string
 *
 * @param str	 the destination buffer, NULL to print to stdout
 * @param end	 the end of the destination buffer
 * @param spec	 a struct fmt_spec
 * @param args	 the arguments to the format string
 *
 * @return the number of bytes written
 */

static size_t render_string(char **str, const char *end, struct fmt_spec *spec,
			  va_list *args)
{
	size_t len;
	size_t n = 0;

	const char *buf = va_arg((*args), const char *);


	if (!buf)
		return 0;


	len = strlen(buf);

	if (spec->flags & VSN_PRECISION) {
		if (len > spec->prec)
			len = spec->prec;
	}

	if (!(spec->flags & VSN_LEFT)) {
		for (; n < spec->width; n++)
			_sprintc(' ', (char **) str, end);
	}

	n = _printn(str, buf, len);

	if (spec->flags & VSN_LEFT) {
		for (; n < spec->width; n++)
			_sprintc(' ', (char **) str, end);
	}

	return n;
}


/**
 * @brief render the format specifier
 *
 * @param str	the destination buffer, NULL to print to stdout
 * @param fmt	a format string buffer
 * @param spec	a struct fmt_spec
 * @param args	the arguments to the format string
 *
 * @return the number of bytes rendered
 */

static size_t render_specifier(char **str, const char *end, const char *fmt,
			       struct fmt_spec *spec, va_list *args)
{
	config_specifier(fmt, spec);

	if (spec->base)
		return render_integer(str, end, fmt, spec, args);


	/* the other stuff */
	switch((*fmt)) {
	case 'f' :
	case 'F' :
		return render_float(str, end, false, spec, args);
	case 'g' :
		return render_float(str, end, true, spec, args);
	case 'c' :
		return render_char(str, end, spec, args);
	case 's' :
		return render_string(str, end, spec, args);

	default:
		/* just print whatever the type specifier is */
		_sprintc((*fmt), (char **) str, end);
		return 1;
	}
	return 0;
}


static size_t decode_format(const char *fmt, struct fmt_spec *spec, va_list *ap)
{
	const char *begin = fmt;

	fmt += eval_flags(fmt, spec);
	fmt += eval_width(fmt, spec, ap);
	fmt += eval_prec(fmt, spec, ap);
	fmt += eval_length(fmt, spec);

	return (size_t) (fmt - begin);
}


/**
 * @brief format a string and print it into a buffer or stdout
 *
 * @param str    the destination buffer, NULL to print to stdout
 * @param format the format string buffer
 * @param args   the arguments to the format string
 *
 * @return the number of characters written to buf
 *
 * @note this is a slightly improved version on xen_printf
 *
 * @note from the C99 Standard Document, Section 7.15, Footnote 221:
 *	 "It is permitted to create a pointer to a va_list and pass that
 *	  pointer to another function, in which case the original function
 *	  may make further use of the original list after the other function
 *	  returns."
 */

int vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	struct fmt_spec spec = {0};

	char *buf, *end;
	const char *tmp;



	/* clamp */
	if (size > INT_MAX)
		size = INT_MAX;


	buf = str;
	end = buf + size;

	/* make sure end > buf */
	if (end < buf) {
		end = ((void *) -1);
		size = end - buf;
	}


	while ((*format))  {

		/*
		 * if we encounter format specifier, we will evaluate
		 * it, otherwise they are normal characters and we
		 * will copy them
		 */

		tmp = format;
		for (; (*format); format++) {
			if ((*format) == '%')
				break;
		}

		if (!str) { /* stdout is infinte... */
			buf += _printn(NULL, tmp, format - tmp);
		} else {
			if ((format - tmp) < (end - buf))
				buf += _printn(&buf, tmp, format - tmp);
			else
				buf += _printn(&buf, tmp, end - buf);

			if (buf >= end)
				break;	/* out of buffer to write */
		}

		/* have we arrived at the end? */
		if (!(*format))
		    break;

		/* we have encountered a format specifier, try to evaluate it */
		format++;
		format += decode_format(format, &spec, &ap);

		/* and render */
		if (!str) /* to stdout? */
			buf += render_specifier(NULL, NULL, format, &spec, &ap);
		else
			buf += render_specifier(&buf, end, format, &spec, &ap);

		format++; /* the type is always a single char */
	}


	/* place termination char if we rendered to a buffer */
	if (size) {
		if (str) {
			if (buf < end)
				buf[0]  = '\0';
			else
				end[-1] = '\0';
		}
	}

	/* return written chars without terminating '\0' */
	return buf - str;
}

