/**
 * @file include/kernel/sysctl.h
 *
 * @brief kernel sysctl / object attribute interface
 *
 * Provides a small tree of sysobjects with attributes that can be shown or
 * stored, mirroring a simplified sysfs-like model.
 */

#ifndef _KERNEL_SYSCTL_H_
#define _KERNEL_SYSCTL_H_

#include <kernel/types.h>
#include <list.h>
#include <kernel/kernel.h>

#ifdef offsetof
#undef offsetof
#endif

/**
 * @brief define a static sobj_attribute initialiser
 *
 * @param _name the attribute name
 * @param _show the show callback
 * @param _store the store callback
 */
#define __ATTR(_name, _show, _store) {                          \
	.name = __stringify(_name),                             \
	.show  = _show,                                         \
	.store = _store,                                        \
}

#define SYSCTL_MAX_PATH_LEN 256	/*!< maximum length of a sysctl path */

/**
 * @brief a sysctl object
 *
 * A named node in the sysctl tree, holding its attributes and links to its
 * parent, child and owning sysset.
 */
struct sysobj {
        const char             *name;
        struct list_head        entry;

	struct sysobj          *parent;
	struct sysobj          *child;

        struct sysset          *sysset;

	struct sobj_attribute **sattr;
};

/**
 * @brief a set of sysobjects sharing a common list head
 */
struct sysset {
	struct list_head list;
	struct sysobj sobj;
};


/**
 * @brief sysobj attribute structure for showing/storing values
 */
struct sobj_attribute {
	const char *name;	/*!< attribute name */
	ssize_t (*show) (struct sysobj *sobj, struct sobj_attribute *sattr, char *buf);	/*!< read callback */
	ssize_t (*store)(struct sysobj *sobj, struct sobj_attribute *sattr, const char *buf, size_t len); /*!< write callback */
};


/**
 * @brief create a new sysobj
 * @return pointer to the newly created sysobj, or NULL on failure
 */
struct sysobj *sysobj_create(void);

/**
 * @brief initialize a sysobj to default values
 * @param sobj: the sysobj to initialize
 */
void sysobj_init(struct sysobj *sobj);

/**
 * @brief add a sysobj to the hierarchy
 * @param sobj: the sysobj to add
 * @param parent: the parent sysobj (or NULL for root)
 * @param sysset: the sysset to associate with
 * @param name: name for this sysobj
 * @return 0 on success, negative error code on failure
 */
int32_t sysobj_add(struct sysobj *sobj, struct sysobj *parent,
		   struct sysset *sysset, const char *name);

/**
 * @brief create a sysobj and add it to the hierarchy
 * @param name: name for the new sysobj
 * @param parent: the parent sysobj
 * @return pointer to the new sysobj, or NULL on failure
 */
struct sysobj *sysobj_create_and_add(const char *name, struct sysobj *parent);

/**
 * @brief display an attribute of a sysobj into a buffer
 * @param sobj: the sysobj to read from
 * @param name: attribute name to read
 * @param buf: destination buffer for the attribute value
 */
void sysobj_show_attr(struct sysobj *sobj, const char *name, char *buf);

/**
 * @brief store a value into a sysobj attribute
 * @param sobj: the sysobj to write to
 * @param name: attribute name to write
 * @param buf: source buffer containing the value
 * @param len: length of the value in bytes
 */
void sysobj_store_attr(struct sysobj *sobj, const char *name, const char *buf, size_t len);

/**
 * @brief create a sysset and add it to the hierarchy
 * @param name: name for the new sysset
 * @param parent_sobj: the parent sysobj
 * @param parent_sysset: the parent sysset (or NULL)
 * @return pointer to the new sysset, or NULL on failure
 */
struct sysset *sysset_create_and_add(const char *name,
				     struct sysobj *parent_sobj,
				     struct sysset *parent_sysset);

/**
 * @brief display the sysset tree hierarchy
 * @param sysset: the sysset to display
 */
void sysset_show_tree(struct sysset *sysset);

/**
 * @brief find a sysobj by path within a sysset
 * @param sysset: the sysset to search
 * @param path: path string to the desired sysobj
 * @return pointer to the found sysobj, or NULL if not found
 */
struct sysobj *sysset_find_obj(struct sysset *sysset, const char *path);

/**
 * @brief get the sysset that contains a sysobj
 * @param sobj: the sysobj to look up
 * @return pointer to the owning sysset
 */
struct sysset *sysset_of_obj(struct sysobj *sobj);

/**
 * @brief find a sysset by path starting from a given sysset
 * @param sysset: the starting sysset
 * @param path: path string to the desired sysset
 * @return pointer to the found sysset, or NULL if not found
 */
struct sysset *sysset_from_path(struct sysset *sysset, const char *path);

/**
 * @brief get the root sysctl sysset
 * @return pointer to the root sysset
 */
struct sysset *sysctl_root(void);


#endif
