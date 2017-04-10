/**
 * @file include/data_proc_task.h
 */

#ifndef _DATA_PROC_TASK_H_
#define _DATA_PROC_TASK_H_

#include <kernel/types.h>
#include <list.h>


struct proc_step {

	unsigned long op_code;	/* the operation to perform on this item */
	void *op_info;		/* arbitrary additional data used by
				 * the processing of this item
				 */
	struct list_head node;
};

struct proc_task {

	void *data;
	size_t size;		/* size of the buffer in bytes */
	size_t nmemb;		/* elements in the buffer */

	struct proc_step *pool;

	struct list_head todo;
	struct list_head done;
	struct list_head free;

	unsigned long type;
	unsigned long seq;
	struct list_head node;	/* to be used for external tracking */
};


void pt_dump_steps_todo(struct proc_task *t);
void pt_dump_steps_done(struct proc_task *t);

void pt_rewind_steps_done(struct proc_task *t);
int pt_del_last_step_done(struct proc_task *t);

int pt_del_pend_step(struct proc_task *t);

void pt_del_all_pending(struct proc_task *t);

int pt_next_pend_step_done(struct proc_task *t);

unsigned long pt_get_pend_step_op_code(struct proc_task *t);

void *pt_get_pend_step_op_info(struct proc_task *t);

int pt_add_step(struct proc_task *t,
		       unsigned long op_code, void *op_info);


size_t pt_get_nmemb(struct proc_task *t);
size_t pt_get_size(struct proc_task *t);
void *pt_get_data(struct proc_task *t);
unsigned long pt_get_type(struct proc_task *t);
unsigned long pt_get_seq(struct proc_task *t);

void pt_set_nmemb(struct proc_task *t, size_t nmemb);
void pt_set_size(struct proc_task *t, size_t size);
void pt_set_data(struct proc_task *t, void *data, size_t size);

void pt_set_type(struct proc_task *t, unsigned long type);
void pt_set_seq(struct proc_task *t, unsigned long seq);


struct proc_task *pt_create(void *data, size_t size, size_t steps,
			    unsigned long type, unsigned long seq);
void pt_destroy(struct proc_task *t);



#endif /* _DATA_PROC_TASK_H_ */
