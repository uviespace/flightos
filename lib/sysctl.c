/**
 * @file   lib/sysctl.c
 * @ingroup sysctl
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @author Linus Torvalds et al.
 * @date   January, 2016
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @defgroup sysctl system control and statistics
 * @brief
 *
 * This system statistics/configuration functionality is tailored/derived
 * from how sysfs/kobjects works in Linux. For obvious reasons, access via a
 * virtual file system is not possible. The file system tree which would usually
 * hold the references to registered objects is not present, hence objects know
 * about their parents AND their children. Sets can be part of another set.
 * Settings and values can be accessed by specifying the path to the object
 * and their name.
 *
 * Trees are built from objects and sets. Objects have attributes that can be
 * read and/or set.
 *
 *
 * Usage notes:
 * This is not supposed to be used from mutliple threads or
 * any other possibly reentrant way. (Needs at least refcounts,
 * atomic locking and strtok_r)
 *
 * Currently, objects and sets are not intended to be freed
 *
 * External object access is not supposed to be fast or  done at a high rates,
 * i.e. more than a few times per second.
 * You can store the reference to the object, which will save some
 * time while searching the tree.
 *
 *
 * @note explicit references to linux source files are not always given
 */

#include <kernel/printk.h>
#include <kernel/kmem.h>
#include <kernel/sysctl.h>
#include <kernel/export.h>
#include <kernel/string.h>

#include <errno.h>

#include <list.h>


/* our standard sets */
struct sysset *sys_set;
struct sysset *driver_set;



static struct sysset *sysset_get(struct sysset *s);


/**
 * @brief get the name of a sysobject
 * @param sobj a reference to a sysobject
 * @return a reference to the name buffer of the sysobject
 */

const char *sysobj_name(const struct sysobj *sobj)
{
        return sobj->name;
}


/**
 * @brief join a sysobject to its sysset
 * @param sobj a reference to a sysobject
 */

static void sobj_sysset_join(struct sysobj *sobj)
{
        if (!sobj->sysset)
                return;

        sysset_get(sobj->sysset);

        list_add_tail(&sobj->entry, &sobj->sysset->list);
}


/**
 * @brief get the reference to a sysobject
 * @param sobj a reference to a sysobject
 */

static struct sysobj *sysobj_get(struct sysobj *sobj)
{
        return sobj;
}


/**
 * @brief set the name of a sysobject
 * @param sobj a reference to a sysobject
 * @param name the name buffer reference to assign
 */

static void sysobj_set_name(struct sysobj *sobj, const char *name)
{
	sobj->name = name;
}


/**
 * @brief initialise a sysobject's lists head
 * @param sobj a reference to a sysobject
 */

static void sysobj_init_internal(struct sysobj *sobj)
{
	if (!sobj)
		return;

	INIT_LIST_HEAD(&sobj->entry);
}


/**
 * @brief initialise a sysobject
 * @param sobj a reference to a sysobject
 */

void sysobj_init(struct sysobj *sobj)
{
        if (!sobj)
                return;	/* invalid pointer */

        sysobj_init_internal(sobj);

        return;
}


/**
 * @brief create a sysobject
 * @return a reference to the new sysobject or NULL on error
 */

struct sysobj *sysobj_create(void)
{
	struct sysobj *sobj;


	sobj = (struct sysobj *) kmalloc(sizeof(*sobj));

	if (!sobj)
		return NULL;

	sysobj_init(sobj);

	return sobj;
}
EXPORT_SYMBOL(sysobj_create);

/**
 * @brief add a sysobject and optionally set its sysset as parent
 * @param sobj a reference to the sysobject
 * @return -1 on error, 0 otherwise
 */

static int32_t sysobj_add_internal(struct sysobj *sobj)
{
	struct sysobj *parent;


	if (!sobj)
                return -1;

	parent = sysobj_get(sobj->parent);

        /* join sysset if set, use it as parent if we do not already have one */
        if (sobj->sysset) {
                if (!parent)
                        parent = sysobj_get(&sobj->sysset->sobj);
                sobj_sysset_join(sobj);
                sobj->parent = parent;
        }

	return 0;
}


/**
 * @brief add a sysobject to a set and/or a parent
 * @param sobj a reference to the sysobject
 * @param parent a reference to the parent sysobject
 * @param sysset a reference to the parent sysset
 * @param name the name of the sysobj
 * @return -1 on error, 0 otherwise
 */

int32_t sysobj_add(struct sysobj *sobj, struct sysobj *parent,
		   struct sysset *sysset, const char *name)
{
        if (!sobj)
                return -1;

	sobj->sysset = sysset;

	sysobj_set_name(sobj, name);

	sobj->parent = parent;

	sysobj_add_internal(sobj);

        return 0;
}


/**
 * @brief create and add a sysobject to a parent
 * @parm name the name of a sysobj
 * @param parent an optional parent to the sysobject
 * @return a reference to the newly created sysobject or NULL on error
 */

struct sysobj *sysobj_create_and_add(const char *name, struct sysobj *parent)
{
        struct sysobj *sobj;


        sobj = sysobj_create();

        if (!sobj)
                return NULL;

        sysobj_add(sobj, parent, NULL, name);

	return sobj;
}


/**
 * @brief list the parameters in a sysobject
 * @param sobj a struct sysobj
 */

void sysobj_list_attr(struct sysobj *sobj)
{
	struct sobj_attribute **sattr;


	if (!sobj)
		return;

	if (!sobj->sattr)
		return;

	sattr = sobj->sattr;

	printk("{");
	while ((*sattr)) {
		printk(" %s", (*sattr)->name);
		sattr++;
	}
	printk(" }");
}


/**
 * @brief call the "show" attribute function of a sysobject
 * @param sobj a struct sysobj
 * @param name the name of the attribute
 * @param buf a buffer to return parameters
 */

void sysobj_show_attr(struct sysobj *sobj, const char *name, char *buf)
{
	struct sobj_attribute **sattr;


	if (!name)
		return;

	if (!sobj)
		return;

	if (!sobj->sattr)
		return;


	sattr = sobj->sattr;

	while ((*sattr)) {

		if (!strcmp((*sattr)->name, name))
			(*sattr)->show(sobj, (*sattr), buf);

		sattr++;
		continue;

	}
}


/**
 * @brief call the "store" attribute function of a sysobject
 * @param sobj a struct sysobj
 * @param name the name of the attribute
 * @param buf a buffer holding parameters
 * @param len the lenght of the buffer
 */

void sysobj_store_attr(struct sysobj *sobj, const char *name, const char *buf, size_t len)
{
	struct sobj_attribute **sattr;

	if (!name)
		return;

	if (!sobj)
		return;

	if (!sobj->sattr)
		return;


	sattr = sobj->sattr;

	while ((*sattr)) {

		if (!strcmp((*sattr)->name, name)) {
			(*sattr)->store(sobj, (*sattr), buf, len);
		}

		sattr++;
		continue;

	}
}


/**
 * @brief get the sysset container of a sysobject
 * @param s a struct sysset
 * @return reference to the sysset or NULL
 */

__extension__
static struct sysset *to_sysset(struct sysobj *sobj)
{
        return sobj ? container_of(sobj, struct sysset, sobj) : NULL;
}


/**
 * @brief get the sysset container of a sysset's sysobject
 * @param s a struct sysset
 * @return reference to the sysset or NULL
 */

static struct sysset *sysset_get(struct sysset *s)
{
        return s ? to_sysset(sysobj_get(&s->sobj)) : NULL;
}


/**
 * @brief initialise a sysset
 * @param s a struct sysset
 */

static void sysset_init(struct sysset *s)
{
        sysobj_init_internal(&s->sobj);
        INIT_LIST_HEAD(&s->list);
}


/**
 * @brief register a sysset
 * @param s a struct sysset
 * @return -1 on error, 0 otherwise
 */

static int32_t sysset_register(struct sysset *s)
{
        if (!s)
                return -1;

        sysset_init(s);

	return sysobj_add_internal(&s->sobj);
}


/**
 * @brief create a sysset
 * @param name a string holding the name of the set
 * @param parent_sobj a struct sysobj or NULL if no sysobj parent
 * @param parent_sysset a struct sysysset or NULL if no sysysset parent
 * @return a reference to the new sysset
 */

struct sysset *sysset_create(const char *name,
			     struct sysobj *parent_sobj,
			     struct sysset *parent_sysset)
{
        struct sysset *sysset;

        sysset = kcalloc(1, sizeof(*sysset));

        if (!sysset)
                return NULL;


        sysobj_set_name(&sysset->sobj, name);

        sysset->sobj.parent = parent_sobj;
        sysset->sobj.child  = &sysset->sobj;

        sysset->sobj.sysset = parent_sysset;

        return sysset;
}


/**
 * @brief create and add a sysset
 * @param name a string holding the name of the set
 * @param parent_sobj a struct sysobj or NULL if no sysobj parent
 * @param parent_sysset a struct sysysset or NULL if no sysysset parent
 * @return a reference to the new sysset
 */

struct sysset *sysset_create_and_add(const char *name,
				     struct sysobj *parent_sobj,
				     struct sysset *parent_sysset)
{
        struct sysset *sysset;


        sysset = sysset_create(name, parent_sobj, parent_sysset);

        if (!sysset)
                return NULL;

        sysset_register(sysset);

        return sysset;
}


/**
 * @brief find the reference to an object in a sysset by its path
 * @param sysset a struct sysset
 * @param path a string describing a path
 * @return a reference to the sysobj found
 */

__extension__
struct sysobj *sysset_find_obj(struct sysset *sysset, const char *path)
{
	char str[256]; /* XXX */
	char *token;

        struct sysobj *s;
        struct sysobj *ret = NULL;



	memcpy(str, path, strlen(path) + 1);

	token = str;

	strsep(&token, "/");

	/* root node */
	if(strcmp(sysobj_name(&sysset->sobj), token))
		return ret;

	while (1) {

		strsep(&token, "/");

		if (!token)
			return ret;

		list_for_each_entry(s, &sysset->list, entry) {

			if (!sysobj_name(s))
				return ret;

			if (strcmp(sysobj_name(s), token))
				continue;

			if (!s->child)
				return s;

			sysset = container_of(s->child, struct sysset, sobj);

			break;
		}
	}

        return ret;
}


/**
 * @brief show a sysset tree
 * @param sysset a struct sysset
 */

__extension__
void sysset_show_tree(struct sysset *sysset)
{
	int32_t i;

	static int32_t rec;

        struct sysobj *k;


	rec++;

	for(i = 0; i < rec; i++)
		printk("    ");

	printk("|__ %s\n", sysobj_name(&sysset->sobj));

        list_for_each_entry(k, &sysset->list, entry) {

		if (!k->child) {

			for(i = 0; i < rec+1; i++)
				printk("    ");
			printk("|__ %s ", sysobj_name(k));

			sysobj_list_attr(k);

			printk("\n");

		} else {
			sysset_show_tree(container_of(k->child, struct sysset, sobj));
		}

	}
	rec--;
}


/**
 * @brief initalises sysctl
 */

int32_t sysctl_init(void)
{
	if (sys_set)
		return -1;

	sys_set = sysset_create_and_add("sys", NULL, NULL);

	if (!sys_set)
		return -1;

	driver_set = sysset_create_and_add("driver", NULL, sys_set);

	if (!driver_set)
		return -1;

	return 0;
}
