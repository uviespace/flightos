/**
 * @file include/data_proc_tracker.h
 */

#ifndef _DATA_PROC_TRACKER_H_
#define _DATA_PROC_TRACKER_H_


struct proc_tracker;

int pt_track_get_usage(struct proc_tracker *pt);

int pt_track_put(struct proc_tracker *pt, struct proc_task *t);

struct proc_task pt_track_get(struct proc_tracker *pt);

void pt_track_sort_seq(struct proc_tracker *pt)

struct proc_tracker *pt_track_create(size_t nmemb);

int pt_track_expand(struct proc_tracker *pt, size_t nmemb);

void pt_track_destroy(struct proc_tracker *pt);


#endif /* _DATA_PROC_TRACKER_H_ */
