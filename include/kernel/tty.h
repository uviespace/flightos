/**
 * @file    include/kernel/tty.h
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup tty
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
 */

#ifndef _KERNEL_TTY_H_
#define _KERNEL_TTY_H_

#if 0
/* not available */
int tty_read(void *buf, size_t nbyte);
#endif


struct queue {
	size_t head;
	size_t tail;
	size_t wrap;
	char *buf;
};

size_t queue_left(struct queue *q);
size_t queue_empty(struct queue *q);
size_t queue_used(struct queue *q);
size_t queue_left(struct queue *q);
size_t queue_full(struct queue *q);
size_t queue_get(struct queue *q, char *c);
size_t queue_put(struct queue *q, char c);

int tty_write(void *buf, size_t nbyte);

#endif /* _KERNEL_TTY_H_ */
