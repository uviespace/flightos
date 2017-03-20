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


struct proc_net;

int pt_track_execute_next(struct proc_tracker *pt);
void pn_input_task(struct proc_net *pn, struct proc_task *t);
int pn_process_next(struct proc_net *pn);
int pn_process_inputs(struct proc_net *pn);

int pn_create_output_node(struct proc_net *pn,
			  int (*op)(unsigned long op_code, struct proc_task *));
int pn_add_node(struct proc_net *pn, struct proc_tracker *pt);
struct proc_net *pn_create(size_t n_in_tasks_crit, size_t n_out_tasks_crit);
void pn_destroy(struct proc_net *pn);

#endif /* _DATA_PROC_NET_H_ */
