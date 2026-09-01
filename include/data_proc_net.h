/**
 * @file include/data_proc_net.h
 *
 * @ingroup data_proc_net
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


/**
 * @brief execute the next pending task for a tracker
 * @param pt: the processing tracker
 * @return 0 on success, negative error code on failure
 */
int pt_track_execute_next(struct proc_tracker *pt);

/**
 * @brief submit a task to a processing network's input node
 * @param pn: the processing network
 * @param t: the task to submit
 */
void pn_input_task(struct proc_net *pn, struct proc_task *t);

/**
 * @brief move critical trackers to the front of the network queue
 * @param pn: the processing network
 */
void pn_queue_critical_trackers(struct proc_net *pn);

/**
 * @brief get the next pending tracker from the network
 * @param pn: the processing network
 * @return pointer to the next tracker, or NULL if none available
 */
struct proc_tracker *pn_get_next_pending_tracker(struct proc_net *pn);

/**
 * @brief get the next pending task from a tracker
 * @param pt: the processing tracker
 * @return pointer to the next task, or NULL if none available
 */
struct proc_task *pn_get_next_pending_task(struct proc_tracker *pt);

/**
 * @brief move a tracker to the head of the network queue
 * @param pn: the processing network
 * @param pt: the tracker to move
 */
void pn_node_to_queue_head(struct proc_net *pn, struct proc_tracker *pt);

/**
 * @brief move a tracker to the tail of the network queue
 * @param pn: the processing network
 * @param pt: the tracker to move
 */
void pn_node_to_queue_tail(struct proc_net *pn, struct proc_tracker *pt);


/**
 * @brief evaluate the return code of a task's op function
 * @param pn: the processing network
 * @param pt: the tracker that processed the task
 * @param t: the task that was processed
 * @param ret: the return code from the op function
 * @return 0 on success, negative error code on failure
 */
int pn_eval_task_status(struct proc_net *pn, struct proc_tracker *pt,
			struct proc_task *t, int ret);

/**
 * @brief process the next task in the network
 * @param pn: the processing network
 * @return 0 on success, negative error code on failure
 */
int pn_process_next(struct proc_net *pn);

/**
 * @brief process any pending input tasks
 * @param pn: the processing network
 * @return 0 on success, negative error code on failure
 */
int pn_process_inputs(struct proc_net *pn);

/**
 * @brief process any pending output tasks
 * @param pn: the processing network
 * @return 0 on success, negative error code on failure
 */
int pn_process_outputs(struct proc_net *pn);

/**
 * @brief create an output node for the processing network
 * @param pn: the processing network
 * @param op: the operator function for the output node
 * @return 0 on success, negative error code on failure
 */
int pn_create_output_node(struct proc_net *pn, op_func_t op);

/**
 * @brief add a tracker node to the processing network
 * @param pn: the processing network
 * @param pt: the tracker to add as a node
 * @return 0 on success, negative error code on failure
 */
int pn_add_node(struct proc_net *pn, struct proc_tracker *pt);

/**
 * @brief create a new processing network
 * @return pointer to the new network, or NULL on failure
 */
struct proc_net *pn_create(void);

/**
 * @brief destroy a processing network and release its resources
 * @param pn: the processing network to destroy
 */
void pn_destroy(struct proc_net *pn);

#endif /* _DATA_PROC_NET_H_ */
