/**
 * @file include/data_proc_net.h
 */

#ifndef _DATA_PROC_NET_H_
#define _DATA_PROC_NET_H_

#include <kernel/types.h>
#include <list.h>
#include <data_proc_tracker.h>



/* these are the reserved processing node identifiers */
#define PN_OP_NODE_IN   0xFFFFFFFF
#define PN_OP_NODE_OUT  0x00000000


/* return codes for op functions */
#define PN_TASK_SUCCESS	0	/* move to next stage */
#define PN_TASK_STOP	1	/* success, but abort processing node  */
#define PN_TASK_DETACH	2	/* task is now tracked by op function */
#define PN_TASK_RESCHED	3	/* move back to queue */
#define PN_TASK_SORTSEQ	4	/* reschedule and sort tasks by seq counter */
#define PN_TASK_DESTROY	5	/* something is wrong, destroy this task */



struct proc_net;


int pt_track_execute_next(struct proc_tracker *pt);
void pn_input_task(struct proc_net *pn, struct proc_task *t);
void pn_queue_critical_trackers(struct proc_net *pn);
struct proc_tracker *pn_get_next_pending_tracker(struct proc_net *pn);
struct proc_task *pn_get_next_pending_task(struct proc_tracker *pt);
void pn_node_to_queue_head(struct proc_net *pn, struct proc_tracker *pt);
void pn_node_to_queue_tail(struct proc_net *pn, struct proc_tracker *pt);


int pn_eval_task_status(struct proc_net *pn, struct proc_tracker *pt,
			struct proc_task *t, int ret);

int pn_process_next(struct proc_net *pn);
int pn_process_inputs(struct proc_net *pn);
int pn_process_outputs(struct proc_net *pn);

int pn_create_output_node(struct proc_net *pn, op_func_t op);
int pn_add_node(struct proc_net *pn, struct proc_tracker *pt);
struct proc_net *pn_create(void);
void pn_destroy(struct proc_net *pn);

#endif /* _DATA_PROC_NET_H_ */
