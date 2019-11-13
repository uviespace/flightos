/**
 * @file include/list.h
 * @ingroup linked_list
 *
 * @note This list implementation was shamelessly stolen and modified from the
 *       linux kernel's include/linux/list.h
 *       Its API (if you will) is used in this (currently non-GPL) project as
 *       under the assumption that the inclusion of header files does not
 *       automatically imply derivative work, see
 *       http://lkml.iu.edu//hypermail/linux/kernel/0301.1/0362.html
 *
 *       No explicit copyright or author statement is given in the original
 *       file, so we assume per the Linux COPYING file:
 *
 * @author Linus Torvalds (and others who actually wrote the linux kernel
 *         version)
 * @author Armin Luntzer (armin.luntzer@univie.ac.at) (local modifications or
 *         extensions)
 *
 * @copyright
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @defgroup linked_list Linked Lists
 * @brief a modified doubly-linked list implementation from the linux kernel
 *
 *
 * This is a (slightly modified) doubly-linked list implementation from the
 * Linux kernel. An easy to understand explanation of
 * how it works and is used can be found here
 * http://kernelnewbies.org/FAQ/LinkedLists
 *
 * Be smart
 * -----
 * It is a valid to criticise linked lists as generally performing badly
 * if you traverse their entries at high frequency rather than just
 * manipulating them. This can be especially true (and has been demonstrated
 * numerous times) for implementations like std:list.
 *
 * Please consider the following though: linked lists are not inherently
 * slow because of how they work algorithmically (*the opposite is true*),
 * but rather because how their cache hit (or miss) rate is in
 * configurations where entries are randomly scattered in memory rather
 * than layed out in one big chunk.
 *
 * Be smart. Don't do that. Allocate a pool in a __single chunk__ and enjoy the
 * cache performance. Do not use larger chunks than the page size of your
 * platform if you have MMU support. If you need more than that, you probably
 * need to redesign your program anyways.
 *
 * This does of course not apply as long as you do access your lists only
 * at slow rates (i.e. in the order of several tens of ms) or if performance
 * is not at all critial.
 *
 */

#ifndef LIST_H
#define LIST_H

#ifdef LIST_HEAD
#undef LIST_HEAD
#endif

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
	next->prev = new;
	new->next  = next;
	new->prev  = prev;
	prev->next = new;
}

/**
 * @brief add a new entry
 * @param new: new entry to be added
 * @param head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}


/**
 * @brief get the struct for this entry
 * @param ptr:	the &struct list_head pointer.
 * @param type:	the type of the struct this is embedded in.
 * @param member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member) ({ \
	struct list_head *head__ = (ptr); \
	struct list_head *pos__ = READ_ONCE(head__->next); \
	pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * @brief iterate over list of given type
 * @param pos:	the type * to use as a loop counter.
 * @param head:	the head for your list.
 * @param member:	the name of the list_struct within the struct.
 * @param note	modified to use __typeof__()
 */

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, __typeof__(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = list_entry(pos->member.next, __typeof__(*pos), member))


/**
 * @brief iterate over list of given type safe against removal of list entry
 * @param pos:	the type * to use as a loop counter.
 * @param n:		another type * to use as temporary storage
 * @param head:	the head for your list.
 * @param member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, __typeof__(*pos), member),	\
	     n = list_entry(pos->member.next, __typeof__(*pos), member);\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, __typeof__(*pos), member))


/**
 * @brief iterator wrapper start
 * @param pos:	the type * to use as a loop counter.
 * @param head:	the head for your list.
 * @param member:	the name of the list_struct within the struct.
 * @param type:	the type of struct
 * @param warning	requires list_entry_while() to close statement
 * @param note	this construction is necessary for a truly circular list that is "headless"
 * @param note	and can be iterated from any starting element without additional overhead
 * @param note	compared to the LIST_HEAD/list_for_each_entry approach
 * @param todo	check if this is functional for all targets (confirmed: gcc 4.8.2)
 */

#define list_entry_do(pos, head, member, type)		\
	{						\
	pos = list_entry((head), type, member);		\
	do						\
	{						\


/**
 * @brief list iterator wrapper stop
 * @param pos:	the type * to use as a loop counter.
 * @param head:	the head for your list.
 * @param member:	the name of the list_struct within the struct.
 * @param type:	the type of struct
 * @param warning	requires list_entry_do() to open statement
 * @param note	see list_entry_while()
 */

#define list_entry_while(pos, head, member, type)		\
	}							\
	while (pos = list_entry(pos->member.next, type, member),\
	       &pos->member != head); 				\
	}

/**
 * @brief the list entry do-while equivalent fo a code block
 * @param pos:	the type * to use as a loop counter.
 * @param head:	the head for your list.
 * @param member:	the name of the list_struct within the struct.
 * @param type:	the type of struct
 * @param _CODE_:	a code segment
 * @param note	see list_entry_while(), list_entry_do()
 */

#define list_entry_do_while(pos, head, member, type, _CODE_) \
	list_entry_do(pos,head,member,type)		     \
	{						     \
		_CODE_;					     \
	} list_entry_while(pos,head,member,type)


/**
 * @brief reverse iterate over list of given type
 * @param pos:	the type * to use as a loop counter.
 * @param head:	the head for your list.
 * @param member:	the name of the list_struct within the struct.
 * (slightly modified in case there is no typeof() functionality in target compiler)
 */

#define list_for_each_entry_rev(pos, head, member)			\
	for (pos = list_entry((head)->prev, __typeof__(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = list_entry(pos->member.prev, __typeof(*pos), member))


/*
 * @brief delete a list entry by making the prev/next entries
 *        point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}


/**
 * @brief deletes entry from list.
 * @param entry: the element to delete from the list.
 * @note list_empty on entry does not return true after this,
 *       the entry is in an undefined state.
 */

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
}


/**
 * @brief deletes entry from list.
 * @param entry: the element to delete from the list.
 * @brief list_empty() on entry does not return true after this, the entry is
 *        in an undefined state.
 */

static inline void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

/**
 * @brief deletes entry from list and reinitialize it.
 * @param entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}

/**
 * @brief delete from one list and add as another's head
 * @param list the entry to move
 * @param head the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del_entry(list);
	list_add(list, head);
}


/**
 * @brief add a new entry
 * @param new: new entry to be added
 * @param head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}


/**
 * @brief replace old entry by new one
 * @param old : the element to be replaced
 * @param new : the new element to insert
 *
 * If @param old was empty, it will be overwritten.
 */

static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}


/**
 * @brief replace entry1 with entry2 and re-add entry1 at entry2's position
 * @param entry1: the location to place entry2
 * @param entry2: the location to place entry1
 */
static inline void list_swap(struct list_head *entry1,
			     struct list_head *entry2)
{
	struct list_head *pos = entry2->prev;

	list_del(entry2);
	list_replace(entry1, entry2);
	if (pos == entry1)
		pos = entry2;
	list_add(entry1, pos);
}

/**
 * @brief tests whether a list is empty
 * @param head: the list to test.
 */

static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

/**
 * @brief tests whether there is at least one element in the list
 * @param head: the list to test.
 */

static inline int list_filled(struct list_head *head)
{
	return head->next != head;
}


/**
 * @brief delete from one list and add as another's tail
 * @param list: the entry to move
 * @param head: the head that will follow our entry
 */

static inline void list_move_tail(struct list_head *list,
				  struct list_head *head)
{
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
}


/**
 * @brief rotate the list to the left
 * @param head: the head of the list
 */

static inline void list_rotate_left(struct list_head *head)
{
        struct list_head *first;

        if (!list_empty(head)) {
                first = head->next;
                list_move_tail(first, head);
        }
}


#endif
