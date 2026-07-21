/**
 * @file include/kernel/reboot.h
 */

#ifndef _KERNEL_REBOOT_H_
#define _KERNEL_REBOOT_H_

/* XXX maybe rethink how we can transport our reset reasons later,
 * perhaps make an opaque type and let the arch define
 */

#include <stdint.h>

#define REBOOT_MEM_UNALIGNED	0xA1
#define REBOOT_MEM_BOUNDS	0xA2
#define REBOOT_DIV0		0xA3
#define REBOOT_UNKNOWN		0xFF


extern void machine_halt(uint8_t reason);
extern void die(void);

#endif /* _KERNEL_REBOOT_H_ */
