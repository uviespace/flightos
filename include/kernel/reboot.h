/**
 * @file include/kernel/reboot.h
 */

#ifndef _KERNEL_REBOOT_H_
#define _KERNEL_REBOOT_H_

/** @brief reset reason: unaligned memory access */
#define REBOOT_MEM_UNALIGNED	0xA1
/** @brief reset reason: memory bounds violation */
#define REBOOT_MEM_BOUNDS	0xA2
/** @brief reset reason: division by zero */
#define REBOOT_DIV0		0xA3
/** @brief reset reason: watchdog timeout */
#define REBOOT_WATCHDOG		0xA4
/** @brief reset reason: EDAC error */
#define REBOOT_EDAC		0xA5
/** @brief reset reason: null pointer dereference */
#define REBOOT_NULLPTR		0xA6
/** @brief reset reason: out of memory */
#define REBOOT_OUT_OF_MEM	0xA7
/** @brief reset reason: unknown cause */
#define REBOOT_UNKNOWN		0xFF


/**
 * @brief halt the machine with a reset reason code
 * @param reason: the reason code for the halt (REBOOT_* constant)
 */
extern void machine_halt(uint8_t reason);

/**
 * @brief trigger a system crash/die sequence
 */
extern void die(void);

#endif /* _KERNEL_REBOOT_H_ */
