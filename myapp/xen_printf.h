/**
 * @file    xen_printf.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 * @date    November, 2013
 *
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
 */

#ifndef X_PRINTF_H
#define X_PRINTF_H

#include <stdarg.h>

#define PAD_RIGHT 0x1
#define PAD_ZERO  0x2


int x_printf(const char *format, ...);
int x_sprintf(char *str, const char *format, ...);


#endif
