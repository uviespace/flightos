/**
 * @file include/kernel/xentium_io.h
 *
 * @note this file may also be included in xentium kernel code
 */

#ifndef _KERNEL_XENTIUM_IO_H_
#define _KERNEL_XENTIUM_IO_H_


#include <noc_dma.h>
#include <data_proc_net.h>

/**
 * used in Xentium kernels to define their parameters and capabilities
 * at compile-time
 */
struct xen_kernel_cfg {
	char *name;
	unsigned long op_code;

	unsigned long crit_buf_lvl;

	void  *data;	/* permanent kernel storage, allocated by driver */
	size_t size;	/* bytes to allocate */
};

/**
 * Xentium commands; some map to data proc net return codes
 * extra parameter(s) may be passed in cmd_param of xen_msg_data structure,
 * e.g. the number of bytes requested for TASK_DATA_REALLOC
 */

enum xen_cmd {
	TASK_SUCCESS = PN_TASK_SUCCESS,	/* ready for next task in node */
	TASK_STOP    = PN_TASK_STOP,    /* success, but abort processing node */
	TASK_DETACH  = PN_TASK_DETACH,  /* task tracking is internal */
	TASK_RESCHED = PN_TASK_RESCHED, /* requeue this task and abort */
	TASK_SORTSEQ = PN_TASK_SORTSEQ, /* TASK_RESCHED and sort by seq cntr */
	TASK_DESTROY = PN_TASK_DESTROY, /* something is wrong, destroy task */
	TASK_NEW,			/* allocate a new task */
	TASK_DATA_REALLOC,		/* (re)allocate task data pointer */
	TASK_EXIT,			/* Xentium exiting */
};


/**
 * structure for message passing between host and Xentium
 */

struct xen_msg_data {

	struct proc_task *t;
	unsigned long xen_id;		/* the Xentium's id */

	struct noc_dma_channel *dma;	/* the reserved DMA channel */

	enum xen_cmd  cmd;		/* command request */
	unsigned long cmd_param;	/* command parameter */
};



/**
 *  wait/status masks for the Xentium mailboxes
 */

#define XEN_WAITMASK_MBOX_0  1
#define XEN_WAITMASK_MBOX_1  2
#define XEN_WAITMASK_MBOX_2  4
#define XEN_WAITMASK_MBOX_3  8


/**
 * xentium <> host I/O
 *
 * we need two mailboxes to exchange command, otherwise x_wait() may trigger on
 * the write done by the xentíum itself
 */

#define XEN_EP_MBOX		0

#define XEN_CMD_MBOX		1
#define XEN_CMD_MBOX_MASK	XEN_WAITMASK_MBOX_1

#define XEN_MSG_MBOX		2


#endif /* _KERNEL_XENTIUM_IO_H_ */
