/**
 * @file   xentium/xen_printf.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   November, 2013
 * @brief  a printf function suitable for use in xentium programs
 * @note  there are many like it, but this one is mine...
 * @note the lack of a cross-processor locking mechanism obviously implies that x_printf and leon printf writes might interfere
 *
 * @copyright 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *
 */


#include <xen_printf.h>


#define TSE 0x2
#define THE 0x4

volatile char *uart1_data  	   = (char *) 0x80000070;
volatile unsigned int *uart1_stat  = (unsigned int*) 0x80000074;

/**
 * @brief what the function name says
 */

static void putchar(int c)
{

	// wait for last byte to be writtenyy
	while (! ((*uart1_stat) & THE));
	(*uart1_data) = (0xff & c);
	// wait until byte is sent
	while (! ((*uart1_stat) & TSE));
}

/**
 * @brief what the function name says
 */

static void x_sprintc(int c, char **str)
{
	//extern int putchar(int c);	// need to supply a UART TX register write
	
	if (str)			// print character to string 
	{
		(**str) = c;
		(*str)++;
	}
	else 
	{
		putchar(c);		// print it to 
	}
}

/**
 * @brief what the function name says
 */
static unsigned int prints(char **str, const char *p_string, const unsigned int pad_flag, unsigned int pad_width)
{
	unsigned int pad_char = ' ';
	unsigned int print_counter = 0;
	unsigned int string_length = 0;

	if(pad_width != 0)
	{

		const char *p_tmp = p_string;

		while(*p_tmp != 0)
		{
			string_length++;
			p_tmp++;
		}

		if(string_length >= pad_width)
		{
			pad_width = 0;
		}
		else
		{
			pad_width -= string_length;
		}

		if (pad_flag & PAD_ZERO)
		{
			pad_char = '0';
		}
	}

	if( !(pad_flag & PAD_RIGHT) )	
        {
		while(pad_width > 0)
		{
			x_sprintc(pad_char, str);
			pad_width--;
			print_counter++;
		}
	}

	while(*p_string != 0)
	{
		x_sprintc(*p_string, str);
		print_counter++;
	       	p_string++;
	}

	while(pad_width > 0)
	{
		x_sprintc(pad_char, str);
		print_counter++;
		pad_width--;
	}

	return print_counter;
}

/**
 * @brief what the function name says
 */
static int printi(char **str, int input, int base, int sign, const unsigned int pad_flag, unsigned int pad_width, int base_letter)
{
	int digit;
	unsigned int print_counter = 0;
	
	const int FIELDSIZE = 12; // fits sign + 32 bit int as char + '\0'
	unsigned int unsigned_input = input;
	
	char *p_buffer;
	char char_buffer[FIELDSIZE];

	
	if(!input) // the integer is zero
        {
		char_buffer[0] =  '0';
		char_buffer[1] = '\0';
		return prints(str, char_buffer, pad_flag, pad_width);
	}

	// point to end of buffer and terminate string
	p_buffer = char_buffer + FIELDSIZE-1;
	*p_buffer = '\0';


	// signed AND negative?
	if(sign && (base == 10) && (input < 0)) 
	{
		unsigned_input = -input;
	}

	// shift throught digits and convert to ascii 
	while(unsigned_input) 
	{
		p_buffer--;
		digit      = unsigned_input % base;
		(*p_buffer) = digit + (digit < 10 ? '0' : base_letter - 10);
		unsigned_input    /= base;
	}

	if(sign && (input < 0)) 
	{
		if( pad_width && (pad_flag & PAD_ZERO) ) {
			x_sprintc('-', str);
			print_counter++;
			pad_width--;
		}
		else {
			p_buffer--;
			(*p_buffer) = '-';
		}
	}

	return print_counter + prints(str, p_buffer, pad_flag, pad_width);
}

/**
 * @brief what the function name says
 */
static int print(char **str, const char *format, va_list args )
{
	unsigned int pad_width;
	unsigned int pad_flag;
	unsigned int print_counter = 0;

	char fmt;
	char *p_tmp;
	char tmp[2];


	while( (fmt=*(format++)) )	// parse format string
	{


		if(fmt  != '%')	// normal character
		{
			x_sprintc (fmt, str);
			print_counter++;
		}
		else			// format specifier found
	       	{

			fmt = *(format++);
			pad_width = pad_flag = 0;


			// string terminated
			if(fmt == '\0')
			{
				break;
			}
			// percent sign escaped
			if(fmt == '%')	
			{
				x_sprintc(fmt, str);
				print_counter++;
				continue;
			}

			if(fmt == '-')
		       	{
				pad_flag = PAD_RIGHT;
				fmt = *(format++);
			}

			while (fmt == '0') 
			{
				pad_flag |= PAD_ZERO;
				fmt = *(format++);
			}

			while((fmt >= '0') && (fmt <= '9'))
			{
				pad_width *= 10;
				pad_width += fmt - '0';
				fmt = *(format++);
			}

			switch (fmt) {

				case 's' :{
					p_tmp = va_arg(args, char *);
					print_counter += prints(str, p_tmp ? p_tmp :"(null)", pad_flag, pad_width);
					break;
					  }

				case 'd' :
					print_counter += printi(str, va_arg(args, int),   10, 1, pad_flag, pad_width, 'a');
					break;

				case 'x':
					print_counter += printi(str, va_arg(args, int), 0x10, 0, pad_flag, pad_width, 'a');
					break;

				case 'X':
					print_counter += printi(str, va_arg(args, int), 0x10, 0, pad_flag, pad_width, 'A');
					break;

				case 'u':
					print_counter += printi(str, va_arg(args, int),   10, 0, pad_flag, pad_width, 'a');
					break;

				case 'c':
					tmp[0] = (char) va_arg(args, int);
					tmp[1] = '\0';
					print_counter += prints(str, tmp, pad_flag, pad_width);
					break;
				default:
					// ignore all invalid
					break;


			}

		}
	}
	
	if(str != 0)
	{
	       	**str = '\0';
	}

	va_end(args);

	return print_counter;
}

/**
 * @brief 'man printf' (mostly)
 *
 */

int printk(const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return print( 0, format, args );
}



/**
 * @brief 'man sprintf' (mostly)
 *
 */
int x_sprintf(char *str, const char *format, ...)
{
        va_list args;
        
        va_start( args, format );
        return print( &str, format, args );
}
