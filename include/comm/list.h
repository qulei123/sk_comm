/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Double Linked Lists
 *
 * Copyright (C) 2018 Laurent Pinchart
 *
 * This file comes from the omap3-isp-live project
 * (git://git.ideasonboard.org/omap3-isp-live.git)
 *
 * Copyright (C) 2010-2011 Ideas on board SPRL
 *
 * Contact: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 */

#ifndef __LIST_H
#define __LIST_H

#include <stddef.h>

struct list_entry {
    struct list_entry *prev;
    struct list_entry *next;
};

static inline void list_init(struct list_entry *list)
{
    list->next = list;
    list->prev = list;
}

static inline int list_empty(struct list_entry *list)
{
    return list->next == list;
}

/* entry加入队列尾部, entry -> head -> .. -> entry */
static inline void list_append(struct list_entry *entry, struct list_entry *head)
{
    entry->next = head;
    entry->prev = head->prev;
    head->prev->next = entry;   /* 倒数第二个node，指向最后一个node(new) */
    head->prev = entry;         /* head->prev  --->  new */
}

/* entry加入队列头部, head -> entry */
static inline void list_prepend(struct list_entry *entry, struct list_entry *head)
{
    entry->next = head->next;
    entry->prev = head;
    head->next->prev = entry;
    head->next = entry;        /* head->next  --->  new */
}

/* entry加入到after节点后面，after -> entry */
static inline void list_insert_after(struct list_entry *entry, struct list_entry *after)
{
    list_prepend(entry, after);
}

/* entry加入到before节点前面，entry -> before */
static inline void list_insert_before(struct list_entry *entry, struct list_entry *before)
{
    list_append(entry, before);
}

static inline void list_remove(struct list_entry *entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
}

#define list_entry(entry, type, member) \
    (type *)((char *)(entry) - offsetof(type, member))

#define list_first_entry(list, type, member) \
    list_entry((list)->next, type, member)

#define list_last_entry(list, type, member) \
	list_entry((list)->prev, type, member)

#define list_only_one(list) \
    ((((list)->next != (list)) && ((list)->next == (list)->prev)) ? 1 : 0)

#define list_for_each(entry, list) \
    for (entry = (list)->next; entry != (list); entry = entry->next)

#define list_for_each_entry(entry, list, member) \
    for (entry = list_entry((list)->next, typeof(*entry), member); \
         &entry->member != (list); \
         entry = list_entry(entry->member.next, typeof(*entry), member))

#define list_for_each_safe(entry, __next, list) \
    for (entry = (list)->next, __next = entry->next; entry != (list); \
         entry = __next, __next = entry->next)

#define list_for_each_entry_safe(entry, __next, list, member) \
    for (entry = list_entry((list)->next, typeof(*entry), member), \
         __next = list_entry(entry->member.next, typeof(*entry), member); \
         &entry->member != (list); \
         entry = __next, __next = list_entry(entry->member.next, typeof(*entry), member))

#endif /* __LIST_H */

