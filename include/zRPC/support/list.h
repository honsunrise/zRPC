//
// Created by zhsyourai on 12/7/16.
//

#ifndef ZRPC_LIST_H
#define ZRPC_LIST_H

#include <stddef.h>
#include "useful.h"

typedef struct zRPC_list_head {
    struct zRPC_list_head *prev;
    struct zRPC_list_head *next;
} zRPC_list_head;

static inline void zRPC_list_init(struct zRPC_list_head *list) {
    list->prev = list;
    list->next = list;
}

static inline void __list_add(struct zRPC_list_head *new_node,
                              struct zRPC_list_head *prev, struct zRPC_list_head *next) {
    prev->next = new_node;
    new_node->prev = prev;
    new_node->next = next;
    next->prev = new_node;
}

static inline void zRPC_list_add(struct zRPC_list_head *new_node, struct zRPC_list_head *head) {
    __list_add(new_node, head, head->next);
}

static inline void zRPC_list_add_tail(struct zRPC_list_head *new_node, struct zRPC_list_head *head) {
    __list_add(new_node, head->prev, head);
}

static inline void __list_del(struct zRPC_list_head *prev, struct zRPC_list_head *next) {
    prev->next = next;
    next->prev = prev;
}

static inline void zRPC_list_del(struct zRPC_list_head *entry) {
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

static inline void zRPC_list_move(struct zRPC_list_head *list, struct zRPC_list_head *head) {
    __list_del(list->prev, list->next);
    zRPC_list_add(list, head);
}

static inline void zRPC_list_move_tail(struct zRPC_list_head *list,
                                       struct zRPC_list_head *head) {
    __list_del(list->prev, list->next);
    zRPC_list_add_tail(list, head);
}

#define zRPC_list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define zRPC_list_first_entry(head, type, member) \
    zRPC_list_entry((head)->next, type, member)

#define zRPC_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#endif //ZRPC_LIST_H