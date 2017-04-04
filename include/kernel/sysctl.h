/**
 * @file include/kernel/sysctl.h
 */

#ifndef _KERNEL_SYSCTL_H_
#define _KERNEL_SYSCTL_H_

#include <kernel/types.h>
#include <list.h>
#include <kernel/kernel.h>

#ifdef offsetof
#undef offsetof
#endif

/* sysfs.h, modified */
#define __ATTR(_name, _show, _store) {                          \
	.name = __stringify(_name),                             \
	.show  = _show,                                         \
	.store = _store,                                        \
}



struct sysobj {
        const char             *name;
        struct list_head        entry;

	struct sysobj          *parent;
	struct sysobj          *child;

        struct sysset          *sysset;

	struct sobj_attribute **sattr;
};

struct sysset {
	struct list_head list;
	struct sysobj sobj;
};


struct sobj_attribute {
	const char *name;
	ssize_t (*show) (struct sysobj *sobj, struct sobj_attribute *sattr, char *buf);
	ssize_t (*store)(struct sysobj *sobj, struct sobj_attribute *sattr, const char *buf, size_t len);
};

extern struct sysset *sys_set;
extern struct sysset *driver_set;


struct sysobj *sysobj_create(void);

int32_t sysobj_add(struct sysobj *sobj, struct sysobj *parent,
		   struct sysset *sysset, const char *name);

struct sysobj *sysobj_create_and_add(const char *name, struct sysobj *parent);

void sysobj_show_attr(struct sysobj *sobj, const char *name, char *buf);
void sysobj_store_attr(struct sysobj *sobj, const char *name, const char *buf, size_t len);

struct sysset *sysset_create_and_add(const char *name,
				     struct sysobj *parent_sobj,
				     struct sysset *parent_sysset);

void sysset_show_tree(struct sysset *sysset);
struct sysobj *sysset_find_obj(struct sysset *sysset, const char *path);



#endif
