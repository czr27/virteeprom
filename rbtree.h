/*
 *  rbtree.h
 *
 *  This file is a part of VirtEEPROM, emulation of EEPROM (Electrically
 *  Erasable Programmable Read-only Memory).
 *
 *  (C) 2015  Nina Evseenko <anvoebugz@gmail.com>
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


struct rbnode {
    struct rbnode *parent;
    struct rbnode *left;
    struct rbnode *right;
    char color;
    void *data;
};
typedef struct rbnode rbnode;


struct rbtree {
    struct rbnode *root;
    struct rbnode *nullnode;
    int (*comparator)(void *data1, void *data2);
};
typedef struct rbtree rbtree;


int rb_is_nullnode(rbtree *tree, rbnode *node);

rbnode *rb_min_node(rbtree *tree, rbnode *node);

rbnode *rb_max_node(rbtree *tree, rbnode *node);

rbnode* rb_next_node(rbtree *tree, rbnode *node);

rbnode* rb_prev_node(rbtree *tree, rbnode *node);

rbtree *rb_create_tree(int (*cmp_func)(void*, void*));

extern rbnode *rb_create_node();

int rb_release_tree(rbtree *tree);

rbtree *rb_insert_node(rbtree *tree, rbnode *node);

rbtree *rb_delete_node(rbtree *tree, rbnode *node);

rbnode *rb_search_node(rbtree *tree, void *data);

rbtree* rb_release_nodes(rbtree *tree);

void rb_release_node(rbnode *node);

#endif
