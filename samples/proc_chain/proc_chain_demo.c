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


#define CRIT_LEVEL	10

#define OP_ADD		0x1234
#define OP_SUB		0x1235
#define OP_MUL		0x1236

#define STEPS	3



void *kzalloc(size_t size);
void kfree(void *ptr);
int printk(const char *fmt, ...);

int pn_prepare_nodes(struct proc_net *pn);
void pn_new_input_task(struct proc_net *pn, size_t n);

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

int op_output(unsigned long op_code, struct proc_task *t)
{
	ssize_t i;
	ssize_t n;

	unsigned int *p = NULL;


	n = pt_get_nmemb(t);
	printk("OUT: op code %d, %d items\n", op_code, n);

	if (!n)
		goto exit;


	p = (unsigned int *) pt_get_data(t);
	if (!p)
		goto exit;



	for (i = 0; i < n; i++) {
		printk("\t%d\n", p[i]);
	}

exit:
	kfree(p);	/* clean up our data buffer */

	pt_destroy(t);

	return PN_TASK_SUCCESS;
}


int op_add(unsigned long op_code, struct proc_task *t)
{

	ssize_t i;
	ssize_t n;

	unsigned int *p;


	n = pt_get_nmemb(t);

	if (!n)
		return PN_TASK_SUCCESS;


	p = (unsigned int *) pt_get_data(t);

	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;

	printk("ADD: op code %d, %d items\n", op_code, n);

	for (i = 0; i < n; i++) {
		p[i] += 10;
	}


	return PN_TASK_SUCCESS;
}

int op_sub(unsigned long op_code, struct proc_task *t)
{

	ssize_t i;
	ssize_t n;

	unsigned int *p;


	n = pt_get_nmemb(t);

	if (!n)
		return PN_TASK_SUCCESS;


	p = (unsigned int *) pt_get_data(t);

	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;

	printk("SUB: op code %d, %d items\n", op_code, n);

	for (i = 0; i < n; i++) {
		p[i] -= 2;
	}


	return PN_TASK_SUCCESS;
}

int op_mul(unsigned long op_code, struct proc_task *t)
{

	ssize_t i;
	ssize_t n;

	unsigned int *p;


	n = pt_get_nmemb(t);

	if (!n)
		return PN_TASK_SUCCESS;


	p = (unsigned int *) pt_get_data(t);

	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;

	printk("MUL: op code %d, %d items\n", op_code, n);

	for (i = 0; i < n; i++) {
		p[i] *= 3;
	}


	return PN_TASK_SUCCESS;
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



void pn_new_input_task(struct proc_net *pn, size_t n)
{
	struct proc_task *t;

	static int seq;

	int i;
	unsigned int *data;


	t = pt_create(NULL, 0, STEPS, 0, seq++);

	BUG_ON(!t);

	BUG_ON(pt_add_step(t, OP_ADD, NULL));
	BUG_ON(pt_add_step(t, OP_SUB, NULL));
	BUG_ON(pt_add_step(t, OP_MUL, NULL));



	data = kzalloc(sizeof(unsigned int) * n);

	for (i = 0; i < n; i++)
		data[i] = i;

	pt_set_data(t, data, n);


	pn_input_task(pn, t);
}




int main(int argc, char **argv)
{

	struct proc_net *pn;


	pn = pn_create();

	BUG_ON(!pn);

	pn_prepare_nodes(pn);


	pn_new_input_task(pn, 5);
	pn_new_input_task(pn, 0);
	pn_new_input_task(pn, 3);

	pn_process_inputs(pn);

	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);
	pn_process_next(pn);

	pn_process_outputs(pn);


	return 0;
}

