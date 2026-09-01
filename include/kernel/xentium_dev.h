/**
 * @file include/kernel/xentium_dev.h
 *
 * @ingroup xentium_driver
 *
 * @brief Xentium device-memory, mailbox, and tightly-coupled-memory layout.
 *
 * These constants describe the hardware-facing side consumed by
 * `kernel/xentium.c`. The current base address, device count, and memory map
 * are target-specific; portability to another Xentium arrangement needs review.
 *
 * @note this file may also be included in xentium kernel code
 */

#ifndef _KERNEL_XENTIUM_DEV_H_
#define _KERNEL_XENTIUM_DEV_H_



/**
 * @brief Xentium device configuration constants
 */

/** @brief Xentium base address */
#define XEN_BASE_ADDR  0x20000000
/** @brief Xentium block size */
#define XEN_BLOCK_SIZE 0x00100000

/** @brief number of mailboxes */
#define XEN_MAILBOXES	4
/** @brief number of signals */
#define XEN_SIGNALS	8
/** @brief number of timers */
#define XEN_TIMERS	2

/** @brief Xentium-local base address */
#define XEN_BASE_LOCAL	0x00000000UL
/** @brief Xentium device register offset from base */
#define XEN_DEV_OFFSET	0x00080000UL

/** @brief number of tightly-coupled memory (TCM) banks */
#define XEN_TCM_BANKS		4
/** @brief size of each TCM bank in bytes */
#define XEN_TCM_BANK_SIZE	8192
/** @brief total TCM size in bytes */
#define XEN_TCM_SIZE		(XEN_TCM_BANKS * XEN_TCM_BANK_SIZE)

/**
 * @brief Xentium device registers: status, signal & control
 *        located at Xentium base address + XEN_DEV_OFFSET
 */
struct xen_dev_mem {
	unsigned long mbox[XEN_MAILBOXES];	/*!< mailboxes */
	unsigned long sig[XEN_SIGNALS];		/*!< signals */
	unsigned long dma_irq;			/*!< DMA IRQ control */
	unsigned long unused_1;
	unsigned long timer[XEN_TIMERS];	/*!< timers */
	unsigned long msg_irq;			/*!< message IRQ control */
	unsigned long unused_2;
	unsigned long status;			/*!< status register */
	unsigned long pc;			/*!< program counter */
	unsigned long fsm_state;		/*!< finite state machine state */
};

/* the xentium-local device memory */
/** @brief pointer to the Xentium-local device registers */
static struct xen_dev_mem *xen_dev_local = (struct xen_dev_mem *)
					   (XEN_BASE_LOCAL + XEN_DEV_OFFSET);

/**
 * @brief Xentium tightly-coupled memory (TCM)
 */
__extension__
struct xen_tcm {
	union {
		char tcm[XEN_TCM_SIZE];		/*!< full TCM as a flat array */
		struct {
			char bank1[XEN_TCM_BANK_SIZE];	/*!< TCM bank 1 */
			char bank2[XEN_TCM_BANK_SIZE];	/*!< TCM bank 2 */
			char bank3[XEN_TCM_BANK_SIZE];	/*!< TCM bank 3 */
			char bank4[XEN_TCM_BANK_SIZE];	/*!< TCM bank 4 */
		};
	};
};

/* the xentium-local tightly-coupled memory */
/* this must be volatile, or xentium-clang will crap its pants, because
 * this is effectively a NULL pointer */
/** @brief pointer to the Xentium-local tightly-coupled memory */
static volatile struct xen_tcm *xen_tcm_local = (struct xen_tcm *) XEN_BASE_LOCAL;




#endif /* _KERNEL_XENTIUM_DEV_H_ */
