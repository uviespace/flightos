/**
 * This creates a number processing nodes representing Xentium kernel
 * processing stages. Two special trackers are used for input and output.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>


#include <data_proc_task.h>
#include <data_proc_tracker.h>
#include <data_proc_net.h>


#include <kernel/kernel.h>


void *kzalloc(size_t size);
void kfree(void *ptr);
int printk(const char *fmt, ...);

int pn_prepare_nodes(struct proc_net *pn);
void pn_new_input_task(struct proc_net *pn);

int op_add(unsigned long op_code, struct proc_task *pt);
int op_sub(unsigned long op_code, struct proc_task *pt);
int op_mul(unsigned long op_code, struct proc_task *pt);
int op_output(unsigned long op_code, struct proc_task *pt);


int printk(const char *fmt, ...)
{
	int ret;
	va_list args;

	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);

	return ret;
}

void *kzalloc(size_t size)
{
	return calloc(size, 1);
}

void kfree(void *ptr)
{
	free(ptr);
}




/* special op codes */
#define OP_INPUT   0xFFFFFFFF
#define OP_OUTPUT  0x00000000
/* a random base for op codes for this demo */
#define OP_BASE   0x10000000

#define TASKS	5
#define NODES   (STEPS + 2)	/* one per steps/op code + input + output */

#define NODE_IN  STEPS
#define NODE_OUT (STEPS + 1)







#if 0
void proc_tasks_prepare(void)
{

	int i;
	int j;
	int go;
	int op;

	struct proc_task *pt;
	struct proc_tracker **ptt_nodes;




	/* allocate references to the intermediate nodes */
	ptt_nodes = (struct proc_tracker **)
		kzalloc(NODES * sizeof(struct proc_tracker *));

	BUG_ON(!ptt_nodes);
	
	/* create the intermediate tracker nodes, their ID is an op code*/
	for (i = 0; i < STEPS; i++) {
		ptt_nodes[i] = pt_track_create(OP_BASE + i);
		BUG_ON(!ptt_nodes[i]);

	}

	/* create the input and output nodes */
	
	ptt_nodes[NODE_IN] = pt_track_create(OP_INPUT);
	BUG_ON(!ptt_nodes[NODE_IN]);
	ptt_nodes[NODE_OUT] = pt_track_create(OP_OUTPUT);
	BUG_ON(!ptt_nodes[NODE_OUT]);


	/* create a number of individual tasks and define some processing steps,
	 * then add the to the input stage
	 */

	for (i = 0; i < TASKS; i++) {
		/* create a task holding at most STEPS steps */
		pt = pt_create(NULL, 0, STEPS, 0, i);
		BUG_ON(!pt);
	
		/* activate all steps with different op-codes */
		for (j = 0; j < STEPS; j++)
			BUG_ON(pt_add_step(pt, OP_BASE + j, NULL));

		/* add the task to the input tracker */
		pt_track_put(ptt_nodes[NODE_IN], pt);
	}


	/* now process the tasks, we "schedule" by looping over all the
	 * trackers until none have pending tasks left
	 */

	go = 1;
	while (go) {
		go = 0;
		for (i = 0; i < NODES; i++) {
			while (1) { /* keep processing while the node holds
				       tasks */
				pt = pt_track_get(ptt_nodes[i]);
				if (!pt)
					break;

				printk("Processing task in node %d\n", i);
				go++; /* our indicator */

				switch (i) {
				case NODE_IN:
					/* the input node, just moves the task
					 * into the first processing node based
					 * on the first processing step op code
					 */
					op = pt_get_pend_step_op_code(pt);
					printk("IN: move task to node %d\n",
					       op - OP_BASE);
					/* in our demo, we get the correct node
					 * by calculating it from the OP_BASE
					 */
					pt_track_put(ptt_nodes[op - OP_BASE], pt);
					break;

				case NODE_OUT: /* output node */
					printk("OUT: destroy task\n");
					pt_destroy(pt);
					break;

				default: /* processing node */
					op = pt_get_pend_step_op_code(pt);
					printk("PROC: simulating processing "
					       "operation %d\n", op - OP_BASE);
					/* this step is now complete */
					pt_next_pend_step_done(pt);
				
					op = pt_get_pend_step_op_code(pt);
					if (op) {
						printk("PROC: move task to node"
						       " %d\n",
						       op - OP_BASE);
						pt_track_put(ptt_nodes[op - OP_BASE], pt);
					} else {
						printk("PROC: last step, moving"
						       " to output node");
						pt_track_put(ptt_nodes[NODE_OUT], pt);
					}

					break;
				}
			}

		}
		printk("Scheduling cycle complete\n");
	}
		
	printk("Processing complete\n");




#if 0
	pt_dump_steps_todo(pt);
	pt_dump_steps_done(pt);


	pt_next_pend_step_done(pt);
	pt_next_pend_step_done(pt);
	pt_dump_steps_todo(pt);
	pt_dump_steps_done(pt);


	pt_next_pend_step_done(pt);
	pt_dump_steps_todo(pt);
	pt_dump_steps_done(pt);

	pt_rewind_steps_done(pt);
	
	pt_dump_steps_todo(pt);
#endif



	for (i = 0; i < NODES; i++)
		pt_track_destroy(ptt_nodes[i]);
	
	kfree(ptt_nodes);

}

#endif

#define CRIT_LEVEL	10
#define CRIT_IN		CRIT_LEVEL
#define CRIT_OUT	CRIT_LEVEL

#define OP_ADD		0x1234
#define OP_SUB		0x1235
#define OP_MUL		0x1236

#define STEPS	3



int op_output(unsigned long op_code, struct proc_task *pt)
{
	printk("OUT: op code %d\n", op_code);
	
	return 0;
}


int op_add(unsigned long op_code, struct proc_task *pt)
{
	printk("ADD: op code %d\n", op_code);
	
	return 0;
}

int op_sub(unsigned long op_code, struct proc_task *pt)
{
	printk("SUB: op code %d\n", op_code);
	
	return 0;
}

int op_mul(unsigned long op_code, struct proc_task *pt)
{
	printk("MUL: op code %d\n", op_code);

	return 0;
}


int pn_prepare_nodes(struct proc_net *pn)
{
	struct proc_tracker *pt;

		
	/* create and add processing node trackers for the each operation */

	pt = pt_track_create(op_add, OP_ADD, CRIT_LEVEL);
	BUG_ON(!pt);
	BUG_ON(pn_add_node(pn, pt));

	pt = pt_track_create(op_sub, OP_SUB, CRIT_LEVEL);
	BUG_ON(!pt);
	BUG_ON(pn_add_node(pn, pt));

	pt = pt_track_create(op_mul, OP_MUL, CRIT_LEVEL);
	BUG_ON(!pt);

	BUG_ON(pn_add_node(pn, pt));

	BUG_ON(pn_create_output_node(pn, op_output));

	return 0;
}



void pn_new_input_task(struct proc_net *pn)
{
	struct proc_task *t;

	static int seq;


	t = pt_create(NULL, 0, STEPS, 0, seq++);

	BUG_ON(!t);

	BUG_ON(pt_add_step(t, OP_ADD, NULL));
	BUG_ON(pt_add_step(t, OP_SUB, NULL));
	BUG_ON(pt_add_step(t, OP_MUL, NULL));


	pn_input_task(pn, t);
}

void pn_new_input_task2(struct proc_net *pn)
{
	struct proc_task *t;

	static int seq;


	t = pt_create(NULL, 0, STEPS, 0, seq++);

	BUG_ON(!t);

	BUG_ON(pt_add_step(t, OP_MUL, NULL));
	BUG_ON(pt_add_step(t, OP_SUB, NULL));
	BUG_ON(pt_add_step(t, OP_ADD, NULL));


	pn_input_task(pn, t);
}





int main(int argc, char **argv)
{

	struct proc_net *pn;


	pn = pn_create(CRIT_IN, CRIT_OUT);

	BUG_ON(!pn);

	pn_prepare_nodes(pn);


	pn_new_input_task(pn);
	pn_new_input_task2(pn);


	pn_process_inputs(pn);	
	
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);



	return 0;
}

