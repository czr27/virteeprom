/*
 *  examine.c
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


#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "eeprom.h"
#include "examine.h"
#include "errnum.h"
#include "errmsg.h"
#include "gen_testcases.h"
#include <fcntl.h>

void* flash_init(int fd);
int flash_uninit();


struct alloc_res {
    void *mapped_mem;
    veeprom_status *vstatus;
    int fd;
};
typedef struct alloc_res alloc_res;


struct vdata_adapter {
    uint16_t *p;
    uint16_t id;
};
typedef struct vdata_adapter vdata_adapter;


void print_int(rbnode *node) {
    if (node == NULL || node->data == NULL)
        fprintf(stderr, "NULL");
    else
        fprintf(stderr, "%d", *(int*)node->data);
}


static void
print_node(rbtree *tree, rbnode* node, int level, int mode,
        void (*print_data)(rbnode*)) {
    int i = 0;
    for (; i < level; i++)
        fprintf(stderr, "            ");

    const char *color;
    if (node->color == RB_BLACK)
        color = "black";
    else
        color = "red";

    if (mode == -1) {
        (*print_data)(node);
        fprintf(stderr, "(%s)\n", color);
    } else if (mode == 1) {
        fprintf(stderr, "/========= ");
        (*print_data)(node);
        fprintf(stderr, " (%s)(L)\n", color);
    } else {
        fprintf(stderr, "\\========= ");
        (*print_data)(node);
        fprintf(stderr, " (%s)(R)\n", color);
    }
}

void print_tree(rbtree *tree, rbnode *node, void (*print_data)(rbnode*)) {
    if (tree == 0 || node == 0)
        return;

    rbnode *cur = node;
    rbnode *prev = tree->nullnode;
    int level = 0;
    int mode = -1;

    while (cur != tree->nullnode) {
        if (prev != tree->nullnode) {
            if (prev == cur->left && cur->right != tree->nullnode) {
                cur = cur->right;
                prev = tree->nullnode;
                ++level;
                mode = 0;
            } else {
                prev = cur;
                --level;
                cur = cur->parent;
            }
            continue;
        }

        print_node(tree, cur, level, mode, print_data);

        if (cur->left != tree->nullnode) {
            mode = 1;
            cur = cur->left;
            prev = tree->nullnode;
            ++level;
        } else if (cur->right != tree->nullnode) {
            mode = 0;
            cur = cur->right;
            prev = tree->nullnode;
            ++level;
        } else {
            prev = cur;
            cur = cur->parent;
            --level;
        }
    }
    fprintf(stderr, "++++++++++++++++++++++++++++++++++++\n");
}


int cmp_int(void *pdata1, void *pdata2) {
    COND_ERROR_RET((pdata1 != NULL || pdata2 != NULL), ERROR_NULLPTR);

    int v1 = *((int*)pdata1);
    int v2 = *((int*)pdata2);

    if (v1 == v2)
        return 0;
    else if (v1 < v2)
        return -1;
    return 1;
}


int verify_rbtree_1(alloc_res *a) {
    rbtree *tree = rb_create_tree(&cmp_int);

    int i = 0;
    for (; i < 6; i++) {
        int *data = malloc(sizeof(int));
        *data = i;
        rbnode *node = rb_create_node();
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        node->data = data;
        tree = rb_insert_node(tree, node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    i = 0;
    for (; i < 2; i++) {
        rbnode *node = rb_search_node(tree, &i);
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        free(node->data);
        tree = rb_delete_node(tree, node);
        free(node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    i = 6;
    for (; i < 9; i++) {
        int *data = malloc(sizeof(int));
        *data = i;
        rbnode *node = rb_create_node();
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        node->data = data;
        tree = rb_insert_node(tree, node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    i = 3;
    for (; i < 5; i++) {
        rbnode *node = rb_search_node(tree, &i);
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        free(node->data);
        tree = rb_delete_node(tree, node);
        free(node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    i = 9;
    for (; i < 12; i++) {
        int *data = malloc(sizeof(int));
        *data = i;
        rbnode *node = rb_create_node();
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        node->data = data;
        tree = rb_insert_node(tree, node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    i = 6;
    for (; i < 8; i++) {
        rbnode *node = rb_search_node(tree, &i);
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        free(node->data);
        tree = rb_delete_node(tree, node);
        free(node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }


    i = 12;
    for (; i < 15; i++) {
        int *data = malloc(sizeof(int));
        *data = i;
        rbnode *node = rb_create_node();
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        node->data = data;
        tree = rb_insert_node(tree, node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    i = 9;
    for (; i < 10; i++) {
        rbnode *node = rb_search_node(tree, &i);
        COND_ERROR_RET(node != NULL, ERROR_NULLPTR);
        free(node->data);
        tree = rb_delete_node(tree, node);
        free(node);
        COND_ERROR_RET(tree != NULL, ERROR_NULLPTR);
        print_tree(tree, tree->root, &print_int);
    }

    rbnode *node = rb_min_node(tree, tree->root);
    while (!rb_is_nullnode(tree, node)) {
        free(node->data);
        node = rb_next_node(tree, node);
    }

    rb_release_tree(tree);
    return OK;
}


int create_vstatus(alloc_res *a) {
    COND_ERROR_RET(a->mapped_mem != NULL, ERROR_NULLPTR);
    veeprom_status *vstatus = veeprom_create_status();
    COND_ERROR_RET(vstatus != NULL, ERROR_NULLPTR);
    a->vstatus = vstatus;
    int ret = veeprom_status_init(vstatus, a->mapped_mem);
    COND_ERROR_RET(ret == OK, ret);
    COND_ERROR_RET(vstatus->page_order != NULL, ERROR_NULLPTR);
    COND_ERROR_RET(vstatus->ids != NULL, ERROR_NULLPTR);
    COND_ERROR_RET(vstatus->page_order != NULL, ERROR_NULLPTR);
    COND_ERROR_RET(vstatus->flash_start == a->mapped_mem,
            ERROR_VALUE);

    int i = 0;
    for (i = 0; i < FLASH_PAGE_COUNT; i++) {
         COND_ERROR_RET(vstatus->busy_map[i] == -1,
                ERROR_DCNSTY);
    }

    return OK;
}

int init_flash(alloc_res *a) {
    int ret = OK;
    a->fd = open("./testcases/tmp_testcase", O_RDWR); 
    COND_ERROR_RET(a->fd != -1, ERROR_SYSTEM);
    a->mapped_mem = flash_init(a->fd);
    COND_ERROR_RET(a->mapped_mem != NULL, ERROR_NULLPTR);
    COND_ERROR_RET((ret = create_vstatus(a)) == OK, ret);
    ret = veeprom_init(a->vstatus);
    COND_ERROR_RET(ret == OK, ret);
    return OK;
}

int verify_clear(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);

    veeprom_status *vstatus = a->vstatus;
    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i,
                ERROR_DCNSTY);
    }
    COND_ERROR_RET(vstatus->ids->root == vstatus->ids->nullnode, ERROR_VALUE);
    COND_ERROR_RET(vstatus->page_order->root == vstatus->page_order->nullnode, ERROR_VALUE);
    COND_ERROR_RET(vstatus->flash_start == a->mapped_mem,
            ERROR_VALUE);
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_VALUE);
    COND_ERROR_RET(vstatus->next_alloc == 0, ERROR_VALUE);

    return OK;
}


int verify_2(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    
    veeprom_status *vstatus = a->vstatus;
    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i,
                ERROR_DCNSTY);
    }
    COND_ERROR_RET(vstatus->ids->root == vstatus->ids->nullnode,
            ret);
    COND_ERROR_RET(vstatus->page_order->root == vstatus->page_order->nullnode,
            ERROR_VALUE);
    COND_ERROR_RET(vstatus->flash_start == a->mapped_mem,
            ERROR_VALUE);
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_VALUE);
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_3(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);

    veeprom_status *vstatus = a->vstatus;
    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i,
                ERROR_DCNSTY);
    }
    COND_ERROR_RET(vstatus->ids->root == vstatus->ids->nullnode, ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->page_order->root == vstatus->page_order->nullnode, ERROR_VALUE);
    COND_ERROR_RET(vstatus->flash_start == a->mapped_mem, ERROR_VALUE);
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_VALUE);
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_4(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);

    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_5(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_6(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_7(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 0, ERROR_VALUE);

    return OK;
}


int verify_8(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_9(alloc_res *a) {
    int ret = OK;
    a->fd = open("./testcases/tmp_testcase", O_RDWR); 
    COND_ERROR_RET(a->fd != -1, ERROR_SYSTEM);
    a->mapped_mem = flash_init(a->fd);
    COND_ERROR_RET(a->mapped_mem != NULL, ERROR_NULLPTR);
    COND_ERROR_RET((ret = create_vstatus(a)) == OK, ret);
    ret = veeprom_init(a->vstatus);
    COND_ERROR_RET(ret == ERROR_DFG, ret);
    return OK;
}


int verify_10(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}



int verify_11(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_12(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 1,
            ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 44)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 44, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 1014, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+1) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+2) == 243,
            ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_13(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 1,
            ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 44)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 44, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 40, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 974, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+1) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+2) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_14(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 1, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 100)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 1, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 100, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 1014, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+1) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+2) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->next_alloc == 101, ERROR_VALUE);

    return OK;
}


int verify_15(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_16(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_17(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root == tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data == NULL,
            ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);

    return OK;
}


int verify_18(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_19(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_20(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root == tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data == NULL,
            ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_21(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 1,
            ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 44)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 44, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 1012, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);

    uint16_t *page = vstatus->flash_start +
        FLASH_PAGE_SIZE_2B * 44;
    COND_ERROR_RET(*page == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 1) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 2) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 3) == 1,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 4) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 5) == 242,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 6) == 0xFFFF,
            ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_22(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 1,
            ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 44)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 44, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 40, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 972, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);

    COND_ERROR_RET(*(((vdata*)tree->root->data)->p) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+1) == 1,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+2) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+3) == 242,
            ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_23(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 1, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 100)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 1, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 100, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 1012, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+1) == 1,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+2) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+3) == 242,
            ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->next_alloc == 101, ERROR_VALUE);

    return OK;
}


int verify_24(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;
    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_25(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_26(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root == tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data == NULL,
            ERROR_DCNSTY);

    uint16_t *page = vstatus->flash_start +
        FLASH_PAGE_SIZE_2B * 44;
    COND_ERROR_RET(*page == 0xFFFF, ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 1) == 0xFFFF, ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 508) == 0xFFFF, ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 509) == 0xFFFF, ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 510) == 0xFFFF, ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 511) == 0xFFFF, ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_27(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 1, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 43)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 43, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 0, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);

    uint16_t *page = vstatus->flash_start +
        FLASH_PAGE_SIZE_2B * 43;
    COND_ERROR_RET(*page == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 1) == 0,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 2) == 243,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 3) == 1014,
            ERROR_DCNSTY);

    i = 0;
    for (; i < 507; i++) {
        uint16_t expected = i;
        uint16_t actual = *(page + 4 + i);
        COND_ERROR_RET(expected == actual, ERROR_DCNSTY);
    }

    COND_ERROR_RET(*(page + 511) == 0x02FE, ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_28(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root == tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data == NULL,
            ERROR_DCNSTY);

    uint16_t *page = vstatus->flash_start +
        FLASH_PAGE_SIZE_2B * 43;
    COND_ERROR_RET(*page == 0xFFFF,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 1) == 0xFFFF,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(page + 2) == 0xFFFF,
            ERROR_DCNSTY);

    i = 0;
    for (; i < 508; i++) {
        COND_ERROR_RET(*(page + 3 + i) == 0xFFFF, ERROR_DCNSTY);
    }

    COND_ERROR_RET(*(page + 511) == 0xFFFF, ERROR_DCNSTY);
    COND_ERROR_RET(vstatus->next_alloc == 100, ERROR_VALUE);
    return OK;
}


int verify_29(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 3, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 100 || i == 32 || i == 1)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 100, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 0, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 1, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 32, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 0, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 2, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 1, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 984, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    rbtree *tree = vstatus->ids;
    COND_ERROR_RET(tree->root != tree->nullnode,
            ERROR_DCNSTY);
    COND_ERROR_RET(tree->root->data != NULL,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p) == 123,
            ERROR_DCNSTY);
    COND_ERROR_RET(*(((vdata*)tree->root->data)->p+1) == 2069,
            ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 101, ERROR_VALUE);
    return OK;
}


int verify_30(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 0, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    COND_ERROR_RET(vstatus->page_order->root ==
            vstatus->page_order->nullnode, ERROR_VALUE);

    COND_ERROR_RET(vstatus->next_alloc == 101, ERROR_VALUE);
    return OK;
}


int verify_31(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;

    COND_ERROR_RET(vstatus->busy_pages == 4, ERROR_DCNSTY);

    int i = 0;
    for (; i < FLASH_PAGE_COUNT; i++) {
        if (i == 24 || i == 12 || i == 14 || i == 1)
            COND_ERROR_RET(vstatus->busy_map[i] == -1, ERROR_DCNSTY);
        else
            COND_ERROR_RET(vstatus->busy_map[i] == i, ERROR_DCNSTY);
    }

    rbnode *n = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    vpage_status *pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 24, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 984, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 1, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 12, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 0, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 2, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 14, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 0, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(!rb_is_nullnode(vstatus->page_order, n), ERROR_NULLPTR);
    COND_ERROR_RET(n->data != NULL, ERROR_NULLPTR);
    pstatus = (vpage_status*)n->data;
    COND_ERROR_RET(pstatus->counter == 3, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->physnum == 1, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->fragments == 0, ERROR_DCNSTY);
    COND_ERROR_RET(pstatus->free_space == 1014, ERROR_DCNSTY);

    n = rb_next_node(vstatus->page_order, n);
    COND_ERROR_RET(rb_is_nullnode(vstatus->page_order, n), ERROR_DCNSTY);

    vdata_adapter data;
    data.p = &data.id;
    data.id = 12345;
    rbnode *node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node == vstatus->ids->nullnode,
            ERROR_DCNSTY);

    data.id = 123;
    node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node != vstatus->ids->nullnode,
            ERROR_DCNSTY);

    data.id = 456;
    node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node != vstatus->ids->nullnode,
            ERROR_DCNSTY);

    data.id = 1;
    node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node != vstatus->ids->nullnode,
            ERROR_DCNSTY);

    data.id = 12;
    node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node != vstatus->ids->nullnode,
            ERROR_DCNSTY);

    data.id = 12777;
    node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node != vstatus->ids->nullnode,
            ERROR_DCNSTY);

    data.id = 888;
    node = rb_search_node(vstatus->ids, &data);
    COND_ERROR_RET(node != vstatus->ids->nullnode,
            ERROR_DCNSTY);

    COND_ERROR_RET(vstatus->next_alloc == 25, ERROR_VALUE);
    return OK;
}


int verify_32(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    int id1 = 123;
    int id2 = 456;
    int id3 = 789;
    int length = 255;
    uint8_t *data = malloc(length);
    int i = 0;
    for (; i < length; i++)
        *(data + i) = i;

    i = 0;
    for (; i < 43690; i++) {
        ret = veeprom_write(id1, data, length, vstatus);
        COND_ERROR_RET_F(ret == OK, ret, free(data));
        ret = veeprom_write(id2, data, length, vstatus);
        COND_ERROR_RET_F(ret == OK, ret, free(data));
        ret = veeprom_write(id3, data, length, vstatus);
        COND_ERROR_RET_F(ret == OK, ret, free(data));
    }

    ret = veeprom_write(id1, data, length, vstatus);
    COND_ERROR_RET_F(ret == OK, ret, free(data));
    ret = veeprom_write(id2, data, length, vstatus);
    COND_ERROR_RET_F(ret == ERROR_FLEXP, ERROR_EFAIL, free(data));

    free(data);
    return OK;
}


int verify_33(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    int id1 = 123;
    int length = 2069;
    uint8_t *data = malloc(length);
    int i = 0;
    for (; i < length; i++) {
        if (i % 2)
            *(data + i) = 'a';
        else
            *(data + i) = 'b';
    }
    i = 0;
    for (; i < 123; i++) {
        ret = veeprom_write(id1, data, length, vstatus);
        COND_ERROR_RET_F(ret == OK, ret, free(data));
    }
    ret = veeprom_write(id1, data, length, vstatus);
    COND_ERROR_RET_F(ret == ERROR_NOMEM, ret, free(data));
    free(data);

    return OK;
}


int verify_34(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;

    int length = 1;
    uint8_t data[] = { "q" };

    int i = 1;
    for (; i < 16257; i++) {
        ret = veeprom_write(i, &data[0], length, vstatus);
        COND_ERROR_RET(ret == OK, ret);
    }
    ret = veeprom_write(i, &data[0], length, vstatus);
    COND_ERROR_RET(ret == ERROR_NOMEM, ret);
    i = 1;
    for (; i < 16257; i++) {
        ret = veeprom_delete(i, vstatus);
        COND_ERROR_RET(ret == OK, ret);
    }


    return OK;
}


int verify_35(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
 
    veeprom_status *vstatus = a->vstatus;

    int id = 123;
    int length = 0xFFFF - 1;
    uint8_t *data = malloc(length);
    memset((void*)data, 3, length);
    ret = veeprom_write(id, &data[0], length, vstatus);
    COND_ERROR_RET(ret == OK, ret);
    ret = veeprom_write(id, &data[0], length, vstatus);
    COND_ERROR_RET(ret == ERROR_NOMEM, ret);
    free(data);

    return OK;
}


int verify_36(alloc_res *a) {
    int ret = init_flash(a);
    COND_ERROR_RET(ret == OK, ret);
    veeprom_status *vstatus = a->vstatus;

    int id = 1;
    int length = 6;
    uint8_t *data = malloc(length);

    for (; id < 86; id++) {
        memset((void*)data, id, length);
        ret = veeprom_write(id, &data[0], length, vstatus);
        COND_ERROR_RET(ret == OK, ret);
    }
    free(data);

    return OK;
}


int unlink_testcase(const char *filename) {
    int ret = 0;
    if ((ret = unlink("testcases/tmp_testcase")) != 0) {
        ERROR_TRET("unlink testcase failed", ret);
    }
    return OK;
}


struct verification_suite {
    const char *descr;
    int (*p_verify)(alloc_res*);
    int (*p_gen_testcase)(const char*);
};


static struct verification_suite VERIFICATION_SUITE[] = {
    { "verify_rbtree_1", &verify_rbtree_1, NULL },
    { "verify_clear", &verify_clear, &gen_clear },
    { "verify_2", &verify_2, &gen_verify_2 },
    { "verify_3", &verify_3, &gen_verify_3 },
    { "verify_4", &verify_4, &gen_verify_4 },
    { "verify_5", &verify_5, &gen_verify_5 },
    { "verify_6", &verify_6, &gen_verify_6 },
    { "verify_7", &verify_7, &gen_verify_7 },
    { "verify_8", &verify_8, &gen_verify_8 },
    { "verify_9", &verify_9, &gen_verify_9 },
    { "verify_10", &verify_10, &gen_verify_10 },
    { "verify_11", &verify_11, &gen_verify_11 },
    { "verify_12", &verify_12, &gen_verify_12 },
    { "verify_13", &verify_13, &gen_verify_13 },
    { "verify_14", &verify_14, &gen_verify_14 },
    { "verify_15", &verify_15, &gen_verify_15 },
    { "verify_16", &verify_16, &gen_verify_16 },
    { "verify_17", &verify_17, &gen_verify_17 },
    { "verify_18", &verify_18, &gen_verify_18 },
    { "verify_19", &verify_19, &gen_verify_19 },
    { "verify_20", &verify_20, &gen_verify_20 },
    { "verify_21", &verify_21, &gen_verify_21 },
    { "verify_22", &verify_22, &gen_verify_22 },
    { "verify_23", &verify_23, &gen_verify_23 },
    { "verify_24", &verify_24, &gen_verify_24 },
    { "verify_25", &verify_25, &gen_verify_25 },
    { "verify_26", &verify_26, &gen_verify_26 },
    { "verify_27", &verify_27, &gen_verify_27 },
    { "verify_28", &verify_28, &gen_verify_28 },
    { "verify_29", &verify_29, &gen_verify_29 },
    { "verify_30", &verify_30, &gen_verify_30 },
    { "verify_31", &verify_31, &gen_verify_31 },
    { "verify_32", &verify_32, &gen_clear },
    { "verify_33", &verify_33, &gen_clear },
    { "verify_34", &verify_34, &gen_clear },
    { "verify_35", &verify_35, &gen_clear },
    { "verify_36", &verify_36, &gen_clear },
};


void reset(alloc_res *a) {
    a->mapped_mem = NULL;
    a->vstatus = NULL;
    a->fd = -1;
}

int main() {
    int failed = 0;
    int passed = 0;
    int ret = OK;

    alloc_res *a = malloc(sizeof(alloc_res));
    int size = ARRAY_SIZE(VERIFICATION_SUITE);
    int i = 0;
    const char *tmp = "./testcases/tmp_testcase";
    for (; i < size; i++) {
        reset(a);
        int size = ARRAY_SIZE(VERIFICATION_SUITE);
        fprintf(stderr, "Running %s\n", VERIFICATION_SUITE[i].descr);
        if (VERIFICATION_SUITE[i].p_gen_testcase != NULL) {
            ret = VERIFICATION_SUITE[i].p_gen_testcase(tmp);
            if (ret != OK) {
                ERROR(emsg(ret));
                fprintf(stderr, "testcase generation failed\n\n");
                continue;
            }
        }
        if (VERIFICATION_SUITE[i].p_verify == NULL) {
            fprintf(stderr, "verify function not found");
            continue;
        }
        ret = VERIFICATION_SUITE[i].p_verify(a);

        if (ret == OK) {
            passed++;
            fprintf(stderr, "PASSED\n");
        } else {
            failed++;
            fprintf(stderr, "FAILED\n");
        }
        
        COND_ERROR((ret=veeprom_status_release(a->vstatus)) == OK, emsg(ret));

        if (a->mapped_mem != NULL) 
            COND_ERROR((ret = flash_uninit(a->mapped_mem)) == OK, emsg(ret));

        if (a->fd != -1)
            COND_ERROR((ret = close(a->fd)) == 0, emsg(ERROR_SYSTEM));
 
        if (VERIFICATION_SUITE[i].p_gen_testcase != NULL)
            COND_ERROR((ret=unlink_testcase(tmp)) == OK, emsg(ret));

        fprintf(stderr, "\n");
    }

    free(a);

    fprintf(stderr, "_________________________\n");
    fprintf(stderr, "PASSED: %d FAILED: %d\n", passed, failed);
    return 0;
}
