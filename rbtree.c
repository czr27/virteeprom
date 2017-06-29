/*
 *  rbtree_t.c
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


#include "rbtree.h"
#include <stdlib.h>
#include "mem.h"
#include "errdef.h"
#include <wrappers.h>


static rbtree_t *rb_left_rotate(rbtree_t *tree, rbnode_t *x) {
    rbnode_t *y = x->right;
    x->right = y->left;

    if (y->left != tree->nullnode)
        y->left->parent = x;

    y->parent = x->parent;

    if(x->parent == tree->nullnode)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left = x;
    x->parent = y;

    return tree;
}


static rbtree_t *rb_right_rotate(rbtree_t *tree, rbnode_t *x) {
    rbnode_t *y = x->left;
    x->left = y->right;

    if (y->right != tree->nullnode)
        y->right->parent = x;

    y->parent = x->parent;

    if(x->parent == tree->nullnode)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->right = x;
    x->parent = y;

    return tree;
}


static rbtree_t *rb_insert_repair(rbtree_t *tree, rbnode_t *node) {
    rbnode_t *y = NULL;
    while (node->parent->color == RB_RED) {
        if (node->parent == node->parent->parent->left) {
            y = node->parent->parent->right;
            if (y->color == RB_RED) {
                node->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                node = node->parent->parent;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    tree = rb_left_rotate(tree, node);
                }
                node->parent->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                tree = rb_right_rotate(tree, node->parent->parent);
            }
        } else {
            y = node->parent->parent->left;
            if (y->color == RB_RED) {
                node->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                node = node->parent->parent;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    tree = rb_right_rotate(tree, node);
                }
                node->parent->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                tree = rb_left_rotate(tree, node->parent->parent);
            }

        }
    }
    tree->root->color = RB_BLACK;
    return tree;

}


inline int rb_is_nullnode(rbtree_t *tree, rbnode_t *node) {
    return (node == 0 || node == tree->nullnode);
}


rbnode_t *rb_min_node(rbtree_t *tree, rbnode_t *node) {
    while (node != tree->nullnode && node->left != tree->nullnode)
        node = node->left;
    return node;
}


rbnode_t *rb_max_node(rbtree_t *tree, rbnode_t *node) {
    while (node != tree->nullnode && node->right != tree->nullnode)
        node = node->right;
    return node;
}


rbnode_t *rb_next_node(rbtree_t *tree, rbnode_t *node) {
    if (tree == NULL || node == NULL || node == tree->nullnode)
        return node;

    rbnode_t *nullnode = tree->nullnode;

    if (node->right != nullnode)
        return rb_min_node(tree, node->right);

    rbnode_t *p = node->parent;
    while (p != nullnode && p->right == node) {
        node = p;
        p = p->parent;
    }
    return p;
}


rbnode_t *rb_prev_node(rbtree_t *tree, rbnode_t *node) {
    if (tree == NULL || node == NULL || node == tree->nullnode)
        return node;

    rbnode_t *nullnode = tree->nullnode;
    if (node->left != nullnode)
        return rb_max_node(tree, node->left);

    rbnode_t *p = node->parent;
    while (p != nullnode && p->left == node) {
        node = p;
        p = p->parent;
    }
    return p;
}


rbtree_t *rb_insert_node(rbtree_t *tree, rbnode_t *node) {
    rbnode_t *y = tree->nullnode;
    rbnode_t *x = tree->root;

    while (x != tree->nullnode) {
        y = x;
        int ret = (*(tree->comparator))(node->data, x->data);
        if (ret == -1)
            x = x->left;
        else
            x = x->right;
    }

    node->parent = y;

    if (y == tree->nullnode) {
        tree->root = node;
    } else {
        int ret = (*(tree->comparator))(node->data, y->data);
        if (ret == -1)
            y->left = node;
        else
            y->right = node;
    }

    node->left = tree->nullnode;
    node->right = tree->nullnode;
    node->color = RB_RED;

    return rb_insert_repair(tree, node);
}


static rbtree_t *rb_replace(rbtree_t *tree, rbnode_t *n, rbnode_t *replacer) {
    if (n->parent == tree->nullnode)
        tree->root = replacer;
    else if (n == n->parent->left)
        n->parent->left = replacer;
    else
        n->parent->right = replacer;
    if (replacer != tree->nullnode)
        replacer->parent = n->parent;
    return tree;
}


static rbtree_t *rb_delete_repair(rbtree_t *tree, rbnode_t *x) {
    rbnode_t *w = NULL;
    while (x != tree->nullnode && x != tree->root && x->color == RB_BLACK) {
        if (x == x->parent->left) {
            w = x->parent->right;
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                tree = rb_left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == RB_BLACK && w->right->color == RB_BLACK) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->right == RB_BLACK) {
                    w->left->color = RB_BLACK;
                    w->color = RB_RED;
                    tree = rb_right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->right->color = RB_BLACK;
                tree = rb_left_rotate(tree, x->parent);
                x = tree->root;
            }
        } else {
            w = x->parent->left;
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                tree = rb_right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == RB_BLACK && w->left->color == RB_BLACK) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->left == RB_BLACK) {
                    w->right->color = RB_BLACK;
                    w->color = RB_RED;
                    tree = rb_left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->left->color = RB_BLACK;
                tree = rb_right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = RB_BLACK;
    return tree;
}


rbtree_t *rb_delete_node(rbtree_t *tree, rbnode_t *z) {
    if (tree == NULL || z == NULL || z == tree->nullnode)
        return tree;

    rbnode_t *y = z;
    int y_original_color = y->color;
    rbnode_t *x = NULL;

    if (z->left == tree->nullnode) {
        x = z->right;
        tree = rb_replace(tree, z, z->right);
    } else if (z->right == tree->nullnode) {
        x = z->left;
        tree = rb_replace(tree, z, z->left);
    } else {
        y = rb_min_node(tree, z->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z)
            x->parent = y;
        else {
            tree = rb_replace(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        tree = rb_replace(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    if (y_original_color == RB_BLACK)
        tree = rb_delete_repair(tree, x);
    return tree;
}


rbnode_t *rb_search_node(rbtree_t *tree, void *data) {
    rbnode_t *p = tree->root;
    int (*cmp)(void*, void*) = tree->comparator;
    while (p != tree->nullnode) {
        int ret = (*cmp)(data, p->data);
        if (ret == 0)
            return p;
        else if (ret < 0)
            p = p->left;
        else
            p = p->right;
    }
    return p;
}


rbtree_t *rb_create_tree(int (*cmp_func)(void*, void*)) {
    rbtree_t *tree = VEEPROM_Alloc(rbtree_t);

    rbnode_t *node = rb_create_node();
    node->color = RB_BLACK;
    node->data = NULL;
    node->parent = NULL;
    node->left = NULL;
    node->right = NULL;
    tree->nullnode = node;
    tree->comparator = cmp_func;
    tree->root = node;
    return tree;
}


int rb_release_tree(rbtree_t *tree) {
    THROW (tree != NULL, ERROR_NULLPTR);

    rbnode_t *node = rb_min_node(tree, tree->root);
    while (!rb_is_nullnode(tree, node)) {
        tree = rb_delete_node(tree, node);
        VEEPROM_Dealloc(rbnode_t, node);
        node = rb_min_node(tree, tree->root);
    }

    VEEPROM_Dealloc(rbnode_t, tree->nullnode);
    VEEPROM_Dealloc(rbtree_t, tree);

    return OK;
}


rbnode_t *rb_create_node() {
    return VEEPROM_Alloc(rbnode_t);
}


int rb_release_node(rbtree_t *tree, rbnode_t *node) {
    THROW (node != NULL, ERROR_NULLPTR);
    THROW (node != tree->nullnode, ERROR_DCNSTY);

    VEEPROM_Dealloc(rbnode_t, node);

    return OK;
}
