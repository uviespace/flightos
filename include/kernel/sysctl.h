/**
 * @file include/kernel/sysctl.h
 */

#ifndef SYSCTL_H
#define SYSCTL_H

#include <sys/types.h>
#include <list.h>

#ifdef offsetof
#undef offsetof
#endif
/* linux/stddef.h */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/* linux/kernel.h */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */
__extension__
#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)

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


int32_t sysctl_init(void);


#endif
