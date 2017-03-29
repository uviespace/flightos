/**
 * @file include/kernel/xentium_dev.h
 *
 * @note this file may also be included in xentium kernel code
 */

#ifndef _KERNEL_XENTIUM_DEV_H_
#define _KERNEL_XENTIUM_DEV_H_



/**
 * Xentium device configuration
 */

#define XEN_MAILBOXES	4
#define XEN_SIGNALS	8
#define XEN_TIMERS	2

#define XEN_BASE_LOCAL	0x00000000UL
#define XEN_DEV_OFFSET	0x00080000UL

#define XEN_TCM_BANKS		4
#define XEN_TCM_BANK_SIZE	8192
#define XEN_TCM_SIZE		(XEN_TCM_BANKS * XEN_TCM_BANK_SIZE)

/**
 * Xentium device registers: status, signal & control
 * located at Xentium base address + XEN_DEV_OFFSET
 */

struct xen_dev_mem {
	unsigned long mbox[XEN_MAILBOXES];
	unsigned long sig[XEN_SIGNALS];
	unsigned long dma_irq;
	unsigned long unused_1;
	unsigned long timer[XEN_TIMERS];
	unsigned long msg_irq;
	unsigned long unused_2;
	unsigned long status;
	unsigned long pc;
	unsigned long fsm_state;
};

/* the xentium-local device memory */ 
static struct xen_dev_mem *xen_dev_local = (struct xen_dev_mem *)
					   (XEN_BASE_LOCAL + XEN_DEV_OFFSET);

__extension__
struct xen_tcm {
	union {
		char *tcm[XEN_TCM_SIZE];

		char *bank1[XEN_TCM_BANK_SIZE];
		char *bank2[XEN_TCM_BANK_SIZE];
		char *bank3[XEN_TCM_BANK_SIZE];
		char *bank4[XEN_TCM_BANK_SIZE];
	};
};

/* the xentium-local tightly-coupled memory */ 
static struct xen_tcm *xen_tcm_local = (struct xen_tcm *) XEN_BASE_LOCAL;




#endif /* _KERNEL_XENTIUM_DEV_H_ */
