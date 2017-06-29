/*
 *  rbtree_t.h
 *
 *  This file is a part of VirtEEPROM, emulation of EEPROM (Electrically
 *  Erasable Programmable Read-only Memory).
 *
 *  (C) 2016  Nina Evseenko <anvoebugz@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef VEEPROM_RBTREE_H
#define VEEPROM_RBTREE_H

#include "types.h"

#define RB_RED 1
#define RB_BLACK 0


struct rbnode_t {
    struct rbnode_t *parent;
    struct rbnode_t *left;
    struct rbnode_t *right;
    char color;
    void *data;
};
typedef struct rbnode_t rbnode_t;


struct rbtree_t {
    rbnode_t *root;
    rbnode_t *nullnode;
    int (*comparator)(void *data1, void *data2);
};
typedef struct rbtree_t rbtree_t;


int rb_is_nullnode(rbtree_t *tree, rbnode_t *node);

rbnode_t *rb_min_node(rbtree_t *tree, rbnode_t *node);

rbnode_t *rb_max_node(rbtree_t *tree, rbnode_t *node);

rbnode_t* rb_next_node(rbtree_t *tree, rbnode_t *node);

rbnode_t* rb_prev_node(rbtree_t *tree, rbnode_t *node);

rbtree_t *rb_create_tree(int (*cmp_func)(void*, void*));

extern rbnode_t *rb_create_node();

int rb_release_tree(rbtree_t *tree);

rbtree_t *rb_insert_node(rbtree_t *tree, rbnode_t *node);

rbtree_t *rb_delete_node(rbtree_t *tree, rbnode_t *node);

rbnode_t *rb_search_node(rbtree_t *tree, void *data);

rbtree_t *rb_release_nodes(rbtree_t *tree);

int rb_release_node(rbtree_t *tree, rbnode_t *node);

#endif
