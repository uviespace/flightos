/**
 * @file lib/data_proc_net.c
 *
 *
 * This is a data processing network
 *
 */

#include <kernel/printk.h>
#include <kernel/kmem.h>
#include <kernel/kernel.h>

#include <errno.h>

#include <data_proc_net.h>


struct proc_net {
	struct proc_tracker *in;
	struct proc_tracker *out;

	struct list_head nodes;
	
	size_t n;
};	


static int pn_dummy_op(unsigned long op_code, struct proc_task *pt)
{
	return 0;
}

/**
 * @brief locate a tracker by op code
 *
 * @returns tracker or NULL if not found
 * 
 * @note this is not very efficient, especially for large numbers of nodes...
 */

static struct proc_tracker *pn_find_tracker(struct proc_net *pn,
					    unsigned long op_code)
{
	struct proc_tracker *p_elem;


	list_for_each_entry(p_elem, &pn->nodes, node)
		if (p_elem->op_code == op_code)
			return p_elem;


	return NULL;
}



int pn_process_next(struct proc_net *pn)
{
	struct proc_task *t = NULL;
	struct proc_tracker *pt;
	static struct proc_tracker *pt_out;

	unsigned long op;

	
	if (!pt_out)
		pt = list_entry(pn->nodes.next, struct proc_tracker, node);

	/* locate the first tracker that holds at least one task and process it
	 * ideally, this would be sorted so that the most critical tracker
	 * was the first item
	 * XXX implement that sometime...
	 */

	list_for_each_entry(pt, &pn->nodes, node) {

		t = pt_track_get(pt);

		if (t)
			break;
	}

	
	while (1) {
		if (!t)
			break;	/* nothing to do */

		/* current step's op code */
		op = pt_get_pend_step_op_code(t);

		/* XXX maybe eval return code, e.g. for signalling abort of
		 * current task node processing */
		pt->op(op, t);


		pt_next_pend_step_done(t);

		/* next steps's op code */
		op = pt_get_pend_step_op_code(t);

		if (!op) {
			pt_track_put(pn->out, t);
		} else if (pt_out->op_code != op) {
			pt_out = pn_find_tracker(pn, op);

			/* this should not happen */
			if (!pt_out) {
				pr_crit("Error, no such op code, "
					"destroying task\n");

				pt_destroy(t);
				
				/* reset */
				pt_out = list_entry(pn->nodes.next,
						    struct proc_tracker, node);
		
				t = pt_track_get(pt);
				continue;
			}
		}

		/* move to next matching node */	
		pt_track_put(pt_out, t);

		t = pt_track_get(pt);
	}

	/* move processing task to end of queue */
	/* XXX should insert that based on critical level/fill state */
	if (pt)
		list_move_tail(&pt->node, &pn->nodes);


	return 0;
}





/**
 * @brief add a task to the input of the network
 */

void pn_input_task(struct proc_net *pn, struct proc_task *t)
{
	if (!pn)
		return;
	
	if (!t)
		return;

	BUG_ON(pt_track_put_force(pn->in, t));
}


/**
 * @brief assign input tasks to their first processing node
 *
 * @returns 0 or -1 on error (e.g. no processing nodes defined)
 */

int pn_process_inputs(struct proc_net *pn)
{
	unsigned long op;

	struct proc_task *t;
	static struct proc_tracker *pt;


	if (list_empty(&pn->nodes))
		return -1;
	
	if (!pt)
		pt = list_entry(pn->nodes.next, struct proc_tracker, node);
	
	while (1) {
		t = pt_track_get(pn->in);

		if (!t)
			break;

		op = pt_get_pend_step_op_code(t);

		if (pt->op_code != op) {
			pt = pn_find_tracker(pn, op);
			if (!pt) {
				pr_crit("Error, no such op code, "
					"destroying task\n");

				pt_destroy(t);
				
				/* reset */
				pt = list_entry(pn->nodes.next,
						struct proc_tracker, node);
				
				t = pt_track_get(pt);
				continue;
			}
		 }

		BUG_ON(pt_track_put(pt, t));
	}


	return 0;
}

	
/**
 * @brief create an output node of the network
 *
 * @returns 0 on success, -ENOMEM on alloc error
 *
 * @note this destroys the previous output node on success only, otherwise the
 *	 original node is left intact
 */

int pn_create_output_node(struct proc_net *pn,
			  int (*op)(unsigned long op_code, struct proc_task *))

{
	struct proc_tracker *pt;


	pt = pt_track_create(op, PN_OP_NODE_OUT, 1);
	
	if (!pt)
		return -ENOMEM;

	if (pn->out)
		pt_track_destroy(pn->out);

	return 0;
}


/**
 * @brief add a tracker node to a processing network
 *
 * @returns 0 on success, -EINVAL on error
 */

int pn_add_node(struct proc_net *pn, struct proc_tracker *pt)

{
	if (!pn)
		return -EINVAL;

	if (!pt)
		return -EINVAL;

	if (pt->op_code == PN_OP_NODE_IN)
		return -EINVAL;
	
	if (pt->op_code == PN_OP_NODE_OUT)
		return -EINVAL;


	list_add_tail(&pt->node, &pn->nodes);

	pn->n++;

	return 0;
}


/**
 * @brief create a processing network with an input and output node
 *
 * @param n_input_tasks_critical critical level of input task
 * @param n_output_tasks_critical critical level of output task
 *
 * @return processing network or NULL on error
 *
 * @note this creates a default output node that does nothing and just
 *       accumulates tasks
 */

struct proc_net *pn_create(size_t n_in_tasks_crit, size_t n_out_tasks_crit)
{
	struct proc_net *pn;


	if (!n_in_tasks_crit)
		return NULL;
	
	if (!n_out_tasks_crit)
		return NULL;

	pn = (struct proc_net *) kzalloc(sizeof(struct proc_net));

	if (!pn)
		goto error;


	/* the input node just accepts tasks and distributes them to the
	 * appropriate trackers in the network
	 */
	pn->in = pt_track_create(pn_dummy_op, PN_OP_NODE_IN, 1);

	if (!pn->in)
		goto cleanup;

	/* create a default output node that does nothing */
	pn->out = pt_track_create(pn_dummy_op, PN_OP_NODE_OUT, 1);
	if (!pn->out)
		goto cleanup;


	INIT_LIST_HEAD(&pn->nodes);


	return pn;

cleanup:
	pt_track_destroy(pn->in);
	pt_track_destroy(pn->out);

	kfree(pn);

error:
	return NULL;
}


/**
 * @brief destroy a processing network 
 *
 * @param pn a struct proc_net
 *
 *
 * @note the data pointers in the processing tasks are untouched,
 *	 see also pt_destroy() 
 */

void pn_destroy(struct proc_net *pn)
{
	struct proc_tracker *p_elem;
	struct proc_tracker *p_tmp;


	if (!pn)
		return;

	list_for_each_entry_safe(p_elem, p_tmp, &pn->nodes, node) {
		list_del(&p_elem->node);
		pt_track_destroy(p_elem);
	}

	
	pt_track_destroy(pn->in);
	pt_track_destroy(pn->out);

	kfree(pn);
}
