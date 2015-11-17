/*
 *  eeprom.c
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


#include <stdlib.h>
#include <string.h>
#include "rbtree.h"
#include "eeprom.h"
#include "wrappers.h"
#include "errnum.h"
#include "errmsg.h"
#include "mem.h"


static int VEEPROM_PAGE_COUNT = 0;

static int iter_data(vrw_cursor *cursor);
static void init_cursor(vrw_cursor *cursor, veeprom_status *vstatus);
static int _veeprom_write(uint16_t id, uint8_t *data, int length,
        veeprom_status *vstatus);
static int check_cursor(vrw_cursor *cursor);
static void reset_cursor(vrw_cursor *cursor);
static int set_receiving(int physnum, rbnode **f, veeprom_status *vstatus);


static inline void set_page_finished(vrw_cursor *cursor) {
    cursor->flags |= 0x100;
}


static inline void unset_page_finished(vrw_cursor *cursor) {
    cursor->flags &= 0x2ff;
}


static inline int check_page_finished(vrw_cursor *cursor) {
    return cursor->flags & 0x100;
}


static inline int get_data_status(vrw_cursor *cursor) {
    return cursor->flags & 0xff;
}


static inline void set_data_status(vrw_cursor *cursor, int flag) {
    cursor->flags = (cursor->flags & 0xff00) | flag;
}


static inline int get_page_status(uint16_t *page) {
    return *page;
}


static vpage_status *get_pstatus(veeprom_status *vstatus, uint16_t counter) {
    rbnode * node = rb_search_node(vstatus->page_order, (void*)&counter);
    if (node != NULL || node->data != NULL) {
        return (vpage_status*)node->data;
    }
    return NULL;
}


static inline void set_replay(vrw_cursor *cursor) {
    cursor->flags |= VRW_REPLAY;
}


static inline int is_replay(vrw_cursor *cursor) {
    return (VRW_REPLAY & cursor->flags);
}


static uint16_t
*get_page(vpage_status *pstatus, struct veeprom_status *vstatus) {
    VEEPROM_TRACE(pstatus != NULL && vstatus != NULL, ERROR_NULLPTR);
    return vstatus->flash_start + pstatus->physnum * FLASH_PAGE_SIZE_2B;
}


static inline void move_forward(vrw_cursor *cursor) {
    cursor->p_cur++;
}


static inline void move_backward(vrw_cursor *cursor) {
    cursor->p_cur--;
}


static inline vrw_cursor *create_cursor() {
    vrw_cursor *cursor = VEEPROM_Alloc(vrw_cursor);
    memset(cursor, 0, sizeof(vrw_cursor));
    return cursor;
}


static inline vrw_data *create_cursor_data() {
    return VEEPROM_Alloc(vrw_data);
}


static inline void release_cursor_data(vrw_data *data) {
    if (data == NULL)
        return;
    VEEPROM_Dealloc(vrw_data, data);
}


static inline void release_cursor(vrw_cursor *cursor) {
    if (cursor == NULL)
        return;
    release_cursor_data(cursor->data);
    VEEPROM_Dealloc(vrw_cursor, cursor);
}


vpage_status *create_pstatus() {
    return VEEPROM_Alloc(vpage_status);
}


void release_pstatus(vpage_status *pstatus) {
    if (pstatus == NULL)
        return;
    VEEPROM_Dealloc(vpage_status, pstatus);
}


vdata *create_vdata() {
    return VEEPROM_Alloc(vdata);
}


void release_vdata(vdata *data) {
    if (data != NULL) {
        VEEPROM_Dealloc(vdata, data);
    }
}


static inline void
copy_location(vrw_cursor *src, vrw_cursor *dst) {
    dst->p_cur = src->p_cur;
    dst->p_start_page = src->p_start_page;
    dst->p_end_page = src->p_end_page;
    dst->virtpage = src->virtpage;
}


static inline int
set_next_alloc(veeprom_status *vstatus) {
    int16_t i = vstatus->next_alloc + 1;
    for (; i < VEEPROM_PAGE_COUNT; i++) {
        if (vstatus->busy_map[i] != -1) {
            vstatus->next_alloc = i;
            return OK;
        }
    }

    i = 0;
    for (; i < vstatus->next_alloc; i++) {
        if (vstatus->busy_map[i] != -1) {
            vstatus->next_alloc = i;
            return OK;
        }
    }

    vstatus->next_alloc = -1;
    return OK;
}


static inline void
order_erased_page(vpage_status *pstatus, veeprom_status *vstatus) {
    rbnode *node = rb_search_node(vstatus->page_order, (void*)&(pstatus->counter));
    if (!rb_is_nullnode(vstatus->page_order, node)) {
        rb_delete_node(vstatus->page_order, node);
        release_pstatus(pstatus);
        rb_release_node(node);
    }
    vstatus->busy_map[pstatus->physnum] = pstatus->physnum;
    vstatus->busy_pages--;
}


static int
erase_page(uint16_t *page, vpage_status *pstatus, veeprom_status *vstatus) {
    if (pstatus != NULL)
        order_erased_page(pstatus, vstatus);
    return flash_erase_page(page);
}


static int
remove_page(uint16_t *page, rbnode *node, vpage_status *pstatus,
        veeprom_status *vstatus) {
    vstatus->busy_map[pstatus->physnum] = pstatus->physnum;
    if (vstatus->next_alloc == -1)
        vstatus->next_alloc = pstatus->physnum;
    vstatus->busy_pages--;
    rb_delete_node(vstatus->page_order, node);
    release_pstatus(pstatus);
    rb_release_node(node);
    return flash_erase_page(page);
}


static int move_cursor(rbnode *node, vrw_cursor *cursor) {
    VEEPROM_THROW(cursor->vstatus != NULL && node != NULL, ERROR_NULLPTR);

    cursor->virtpage = node;
    uint16_t *page = get_page((vpage_status*)node->data, cursor->vstatus);
    cursor->p_start_page = page;
    cursor->p_end_page = page + FLASH_PAGE_SIZE_2B;
    page += VEEPROM_PAGE_HEADER_2B;
    cursor->p_cur = page;
    unset_page_finished(cursor);
    return OK;
}


static int end_of_page(vrw_cursor *cursor) {
    if (check_page_finished(cursor))
        return 1;
    else if (cursor->p_cur < cursor->p_end_page)
        return 0;
    set_page_finished(cursor);
    return 1;
}


static int
move_valid_data(rbnode *node, veeprom_status *vstatus) {
    VEEPROM_THROW(vstatus != NULL, ERROR_NULLPTR);
    vrw_cursor *rcursor = create_cursor();
    init_cursor(rcursor, vstatus);
    int ret = move_cursor(node, rcursor);
    VEEPROM_THROW(ret == OK, ret, release_cursor(rcursor));
    set_replay(rcursor);

    for (; !end_of_page(rcursor); move_forward(rcursor)) {
        VEEPROM_THROW((ret=iter_data(rcursor)) == OK, ret,
                release_cursor(rcursor));
        switch (get_data_status(rcursor)) {
        case VRW_OK:
            VEEPROM_THROW((ret=_veeprom_write(rcursor->data->id,
                    (uint8_t*)(rcursor->data->p_start_data + 2),
                    *(rcursor->data->p_start_data + 1),
                    vstatus)) == OK, ret,
                    release_cursor(rcursor));
            reset_cursor(rcursor);
            continue;
        case VRW_FAILED:
            release_cursor(rcursor);
            VEEPROM_THROW(0, ERROR_DCNSTY);
        default:
            VEEPROM_THROW(check_page_finished(rcursor),
                    ERROR_DCNSTY, release_cursor(rcursor));
            break;
        }
    }

    release_cursor(rcursor);
    return OK;
}


static inline int
has_no_data(vpage_status *pstatus) {
    return pstatus->fragments + pstatus->free_space ==
            FLASH_PAGE_SIZE - VEEPROM_PAGE_HEADER;
}


static inline int
is_fragmented(vpage_status *pstatus) {
    VEEPROM_THROW(pstatus != NULL, ERROR_NULLPTR);
    int half = (FLASH_PAGE_SIZE - VEEPROM_PAGE_HEADER) / 2;
    if (pstatus->fragments < half)
        return 0;
    return 1;
}


static int gc(veeprom_status *vstatus) {
    VEEPROM_THROW(vstatus != NULL, ERROR_NULLPTR);
    rbnode *node = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    while (!rb_is_nullnode(vstatus->page_order, node)) {
        VEEPROM_THROW(node->data != NULL, ERROR_NULLPTR);
        vpage_status *pstatus = (vpage_status*)node->data;
        if (has_no_data(pstatus)) {
            ;
        } else if (is_fragmented(pstatus) &&
                vstatus->busy_pages < VEEPROM_PAGE_COUNT) {
            int ret = move_valid_data(node, vstatus);
            VEEPROM_THROW(ret == OK, ret);
        } else {
            node = rb_next_node(vstatus->page_order, node);
            continue;
        }

        uint16_t *page = get_page(pstatus, vstatus);
        rbnode *next = rb_next_node(vstatus->page_order, node);
        int ret = remove_page(page, node, pstatus, vstatus);
        VEEPROM_THROW(ret == OK, ret);
        node = next;
    }
    return OK;
}


static int
erase_data(vrw_cursor *cursor) {
    check_cursor(cursor);
    vrw_cursor *prev_cursor = create_cursor();
    VEEPROM_THROW(prev_cursor != NULL, ERROR_NULLPTR);
    copy_location(cursor, prev_cursor);

    int ret = OK;
    uint16_t erased = 0;
    while (erased++ < cursor->rw_ops) {
        if (cursor->p_cur == cursor->p_start_page + 1) {
            rbtree *page_order = cursor->vstatus->page_order;
            rbnode *n = rb_prev_node(page_order, cursor->virtpage);
            VEEPROM_THROW(!rb_is_nullnode(page_order, n),
                    ERROR_NULLPTR, release_cursor(prev_cursor));
            ret = move_cursor(n, cursor);
            VEEPROM_THROW(ret == OK, ret, release_cursor(prev_cursor));
            cursor->p_cur = cursor->p_start_page + FLASH_PAGE_SIZE_2B - 1;
        }

        if ((ret = flash_zero_short(cursor->p_cur)) != OK) {
            VEEPROM_LOGDEBUG("flash_zero_short ret=%d\n", ret);
            copy_location(prev_cursor, cursor);
            release_cursor(prev_cursor);
            return ret;
        }

        ((vpage_status*)(cursor->virtpage->data))->fragments += 2;
        move_backward(cursor);
    }
    copy_location(prev_cursor, cursor);
    release_cursor(prev_cursor);

    return OK;
}


static void reset_cursor(vrw_cursor *cursor) {
    set_data_status(cursor, VRW_CLEAN);
    cursor->rw_ops = 0;
    cursor->cur_checksum = 0;
    if (cursor->data != NULL) {
        cursor->data->id = 0;
        cursor->data->length = 0;
        cursor->data->p_start_data = NULL;
        cursor->data->p_end_data = NULL;
        cursor->data->checksum = 0;
    }
}


static inline int check_cursor_status(vrw_cursor *cursor) {
    if (is_replay(cursor))
        return get_data_status(cursor) == VRW_OK;
    return get_data_status(cursor) == VRW_FAILED;
}


static int check_cursor(vrw_cursor *cursor) {
    VEEPROM_THROW(cursor != NULL && cursor->vstatus != NULL &&
            cursor->virtpage != NULL &&
            cursor->data != NULL &&
            cursor->p_cur != NULL &&
            cursor->data->p_start_data != NULL &&
            cursor->data->p_end_data != NULL, ERROR_NULLPTR);
    VEEPROM_THROW(cursor->p_start_page <= cursor->p_cur &&
            cursor->p_end_page >= cursor->p_cur &&
            check_cursor_status(cursor) == 1, ERROR_DCNSTY);
    return OK;
}


static int estimate_free_space(uint16_t *p_page) {
    uint16_t *p = p_page;
    uint16_t *p_end = p + FLASH_PAGE_SIZE_2B;
    int free_space = 0;

    p += VEEPROM_PAGE_HEADER_2B;
    for (; p < p_end; p++) {
        if (*p == 0xFFFF)
            free_space++;
    }
    return free_space;
}


static int
order_valid_page(int physnum, uint16_t *page, veeprom_status *vstatus) {
    int ret = OK;
    uint16_t counter = *(page + 1);
    VEEPROM_THROW(counter < VEEPROM_MAX_VIRTNUM, ERROR_OBNDS);

    vpage_status *pstatus_prev = get_pstatus(vstatus, counter);
    if (pstatus_prev != NULL) {
        uint16_t *page_prev = vstatus->flash_start +
                pstatus_prev->physnum * FLASH_PAGE_SIZE_2B;
        int free_space_prev = estimate_free_space(page_prev);
        int free_space = estimate_free_space(page);

        if (free_space_prev > free_space) {
            ret = erase_page(page, NULL, vstatus);
            VEEPROM_THROW(ret == OK, ret);
            vstatus->busy_map[physnum] = physnum;
            return ret;
        } else if (free_space_prev < free_space) {
            VEEPROM_THROW((ret=erase_page(page_prev, NULL, vstatus)) == OK, ret);
            vstatus->busy_map[pstatus_prev->physnum] = pstatus_prev->physnum;
        } else {
            VEEPROM_THROW(0, ERROR_DFG);
        }
    } else {
        int busy_pages = vstatus->busy_pages;
        VEEPROM_THROW(++busy_pages < VEEPROM_PAGE_COUNT, ERROR_NOMEM);
        pstatus_prev = create_pstatus();
        pstatus_prev->counter = counter;
        rbnode *node = rb_create_node();
        node->data = (void*)pstatus_prev;
        rb_insert_node(vstatus->page_order, node);
        vstatus->busy_pages = busy_pages;
    }

    pstatus_prev->physnum = physnum;
    pstatus_prev->fragments = 0;
    pstatus_prev->free_space = 0;
    return OK;
}


static void init_cursor(vrw_cursor *cursor, veeprom_status *vstatus) {
    cursor->vstatus = vstatus;
    cursor->virtpage = NULL;
    cursor->data = create_cursor_data();
    cursor->data->id = 0;
    cursor->data->length = 0xFFFF;
    cursor->data->p_start_data = NULL;
    cursor->data->p_end_data = NULL;
    cursor->data->checksum = 0;
    cursor->cur_checksum = 0;
    cursor->flags = VRW_CLEAN;
    cursor->rw_ops = 0;
    cursor->aligned_length_2b = 0;
}


static void copy_vdata(vrw_cursor *cursor, rbnode *node) {
    vdata *p = (vdata*)(node->data);
    p->p = cursor->data->p_start_data;
}


static int locate(vrw_cursor *cursor) {
    veeprom_status *vstatus = cursor->vstatus;
    unsigned long delta = (unsigned long)((void*)(cursor->p_cur)) -
            (unsigned long)((void*)(vstatus->flash_start));
    int physnum = delta / FLASH_PAGE_SIZE;
    cursor->p_start_page = vstatus->flash_start +
            physnum * FLASH_PAGE_SIZE_2B;
    cursor->p_end_page = cursor->p_start_page +
            FLASH_PAGE_SIZE_2B;

    int16_t status = get_page_status(cursor->p_start_page);
    VEEPROM_THROW(status == PAGE_VALID, ERROR_DCNSTY);
    int16_t counter = *(cursor->p_start_page + 1);
    rbnode *v = rb_search_node(vstatus->page_order, (void*)&counter);
    VEEPROM_THROW(v != NULL && v->data != NULL, ERROR_NULLPTR);
    cursor->virtpage = v;

    return OK;
}


static int remove_data(vrw_cursor *rcursor, uint16_t id) {
    int ret = OK;
    VEEPROM_THROW((ret=locate(rcursor)) == OK, ret);
    set_replay(rcursor);

    rbtree *page_order = rcursor->vstatus->page_order;
    while (!rb_is_nullnode(page_order, rcursor->virtpage)) {
        VEEPROM_THROW((ret=iter_data(rcursor)) == OK, ret);
        switch (get_data_status(rcursor)) {
        case VRW_OK:
            VEEPROM_THROW(rcursor->data->id == id, ERROR_DCNSTY);
            VEEPROM_THROW((ret=erase_data(rcursor)) == OK, ret);
            return OK;
        case VRW_FAILED:
            VEEPROM_THROW(0, ERROR_DCNSTY);
        default:
            VEEPROM_THROW(check_page_finished(rcursor), ERROR_DCNSTY);
            break;
        }
        rcursor->virtpage = rb_next_node(page_order, rcursor->virtpage);
        ret = move_cursor(rcursor->virtpage, rcursor);
        VEEPROM_THROW(ret == OK, ret);
    }

    VEEPROM_THROW(0, ERROR_UNKNOWN);
}


static int add_data(vrw_cursor *cursor) {
    rbnode *node = NULL;
    rbtree *ids = cursor->vstatus->ids;
    int ret = OK;
    vdata data;
    data.p = &(cursor->data->id);
    if ((node = rb_search_node(ids, &data)) != ids->nullnode) {
        vrw_cursor *rcursor = create_cursor();
        init_cursor(rcursor, cursor->vstatus);
        rcursor->p_cur = ((vdata*)node->data)->p;
        ret = remove_data(rcursor, cursor->data->id);
        if (ret != OK) {
            release_cursor(rcursor);
            VEEPROM_THROW(0, ret);
        }
        copy_vdata(cursor, node);
        release_cursor(rcursor);
    } else {
        node = rb_create_node();
        node->data = create_vdata();
        copy_vdata(cursor, node);
        ids = rb_insert_node(ids, node);
    }
    return OK;
}


static int init_page(vrw_cursor *cursor) {
    do {
        int ret = iter_data(cursor);
        VEEPROM_THROW(ret == OK, ret);

        switch (get_data_status(cursor)) {
        case VRW_OK:
            VEEPROM_THROW((ret=add_data(cursor)) == OK, ret);
            reset_cursor(cursor);
            move_forward(cursor);
            continue;
        case VRW_FAILED:
            VEEPROM_THROW((ret=erase_data(cursor)) == OK, ret);
            reset_cursor(cursor);
            move_forward(cursor);
            continue;
        case VRW_CLEAN:
            VEEPROM_THROW(check_page_finished(cursor), ERROR_DCNSTY);
            continue;
        default:
            if (check_page_finished(cursor))
                return OK;
            VEEPROM_THROW(0, ERROR_UNKNOWNSTATUS);
        }
    } while (!check_page_finished(cursor));
    return OK;
}


static int init_data(veeprom_status *vstatus) {
    if (vstatus->busy_pages < 0)
        VEEPROM_THROW(0, ERROR_DCNSTY);
    else if (vstatus->busy_pages == 0)
        return OK;

    vrw_cursor *cursor = create_cursor();
    VEEPROM_THROW(cursor != NULL, ERROR_NULLPTR);
    init_cursor(cursor, vstatus);

    rbtree *page_order = vstatus->page_order;
    rbnode *node = rb_min_node(page_order, page_order->root);
    for (; !rb_is_nullnode(page_order, node);
            node=rb_next_node(page_order, node)) {
        int ret = move_cursor(node, cursor);
        if (ret != OK) {
            release_cursor(cursor);
            VEEPROM_THROW(0, ret);
        }
        ret = init_page(cursor);
        if (ret != OK) {
            release_cursor(cursor);
            VEEPROM_THROW(0, ret);
        }
    }

    release_cursor(cursor);
    return OK;
}


static int order_pages(veeprom_status *vstatus) {
    uint16_t *p = vstatus->flash_start;
    uint16_t *flash_end = p + FLASH_PAGE_SIZE * VEEPROM_PAGE_COUNT;
    int ret = 0;
    int physnum = 0;
    for (; physnum < VEEPROM_PAGE_COUNT && p < flash_end;
            physnum++, p += FLASH_PAGE_SIZE_2B) {
        int s = get_page_status(p);
        switch (s) {
        case PAGE_VALID:
            VEEPROM_THROW((ret=order_valid_page(physnum, p, vstatus)) == OK,
                    ret);
            vstatus->next_alloc = physnum;
            break;
        case PAGE_RECEIVING:
            ret = erase_page(p, NULL, vstatus);
            VEEPROM_THROW(ret == OK, ret);
            vstatus->busy_map[physnum] = physnum;
            vstatus->next_alloc = physnum;
            break;
        case PAGE_ERASED:
            vstatus->busy_map[physnum] = physnum;
            break;
        default:
            VEEPROM_THROW(0, ERROR_UNKNOWNSTATUS);
            break;
        }
    }
    set_next_alloc(vstatus);
    return OK;
}


static void set_veeprom_page_count() {
#ifdef CHIBIOS_ON
    extern uint8_t _veeprom_start[];
    extern uint8_t _veeprom_end[];
    VEEPROM_LOGDEBUG("veeprom_start=0x%x\n", _veeprom_start);
    VEEPROM_LOGDEBUG("veeprom_end=0x%x\n", _veeprom_end);
    VEEPROM_PAGE_COUNT = (_veeprom_end - _veeprom_start)/FLASH_PAGE_SIZE;
    VEEPROM_LOGDEBUG("veeprom_page_count=%d\n", VEEPROM_PAGE_COUNT);
#else
    VEEPROM_PAGE_COUNT = FLASH_PAGE_COUNT;
#endif
}


static int check_order(veeprom_status *vstatus) {
    int i = 0;
    rbnode *node = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    for (; i < vstatus->busy_pages; i++) {
        VEEPROM_THROW(!rb_is_nullnode(vstatus->page_order, node), ERROR_INVORDER);
        node = rb_next_node(vstatus->page_order, node);
    }
    return OK;
}


int veeprom_init(veeprom_status *s) {
    VEEPROM_THROW(s != NULL, ERROR_NULLPTR);

    int ret = 0;
    if (((ret=order_pages(s)) != OK) || ((ret=check_order(s)) != OK) ||
            ((ret=init_data(s)) != OK) || (ret=gc(s))) {
        return ret;
    }
    return OK;
}


int cmp_vdata(void *pdata1, void *pdata2) {
    VEEPROM_THROW((pdata1 != NULL || pdata2 != NULL), ERROR_NULLPTR);

    uint16_t id1 = *(((vdata*)pdata1)->p);
    uint16_t id2 = *(((vdata*)pdata2)->p);

    if (id1 == id2)
        return 0;
    else if (id1 < id2)
        return -1;
    return 1;
}


int cmp_uint16(void *pdata1, void *pdata2) {
    VEEPROM_THROW((pdata1 != NULL || pdata2 != NULL), ERROR_NULLPTR);

    uint16_t v1 = *((uint16_t*)pdata1);
    uint16_t v2 = *((uint16_t*)pdata2);

    if (v1 == v2)
        return 0;
    else if (v1 < v2)
        return -1;
    return 1;
}


int veeprom_status_init(veeprom_status *vstatus, uint16_t *flash_start) {
    set_veeprom_page_count();
    memset(vstatus->busy_map, 0xFF, (sizeof(vstatus->busy_map)));
    vstatus->flash_start = flash_start;
    vstatus->ids = rb_create_tree(&cmp_vdata);
    vstatus->page_order = rb_create_tree(&cmp_uint16);
    vstatus->busy_pages = 0;
    vstatus->next_alloc = -1;
    return OK;
}


int veeprom_status_release(veeprom_status *vstatus) {
    if (vstatus == NULL)
        return OK;

    rbnode *node = rb_min_node(vstatus->ids, vstatus->ids->root);
    while (!rb_is_nullnode(vstatus->ids, node)) {
        release_vdata(node->data);
        node = rb_next_node(vstatus->ids, node);
    }

    rb_release_tree(vstatus->ids);

    node = rb_min_node(vstatus->page_order, vstatus->page_order->root);
    while (!rb_is_nullnode(vstatus->page_order, node)) {
        release_pstatus(node->data);
        node = rb_next_node(vstatus->page_order, node);
    }

    rb_release_tree(vstatus->page_order);

    VEEPROM_Dealloc(veeprom_status, vstatus);

    vstatus = NULL;
    return OK;
}


veeprom_status *veeprom_create_status() {
    return VEEPROM_Alloc(veeprom_status);
}


int veeprom_clean(veeprom_status *vstatus) {
    int ret = OK;
    int i = 0;
    uint16_t *page = vstatus->flash_start;
    for (; i < VEEPROM_PAGE_COUNT; i++) {
        VEEPROM_THROW((ret=flash_erase_page(page)) == OK, ret);
        page += FLASH_PAGE_SIZE_2B;
    }
    return OK;
}


static int
write_2b(uint16_t data, vrw_cursor *cursor) {
    if (cursor->p_cur >= cursor->p_end_page) {
        rbtree *page_order = cursor->vstatus->page_order;
        rbnode *n = rb_next_node(page_order, cursor->virtpage);
        VEEPROM_THROW(!rb_is_nullnode(page_order, n), ERROR_NULLPTR);
        int ret = move_cursor(n, cursor);
        VEEPROM_THROW(ret == OK, ret);
        int s = get_page_status(cursor->p_start_page);
        VEEPROM_THROW(s == PAGE_RECEIVING, ERROR_PAGEALLOC);
    }
    int ret = OK;
    VEEPROM_THROW((ret=flash_write_short(data, cursor->p_cur)) == OK,
            ret);
    cursor->rw_ops++;
    ((vpage_status*)(cursor->virtpage->data))->free_space -= 2;
    cursor->data->p_end_data = cursor->p_cur;
    cursor->cur_checksum ^= data;
    return OK;
}


static int alloc_pages(int size, vrw_cursor *cursor) {
    int count = size / FLASH_PAGE_SIZE_2B;
    if (size % FLASH_PAGE_SIZE_2B)
        count++;

    veeprom_status *vstatus = cursor->vstatus;
    int free_pages = VEEPROM_PAGE_COUNT - vstatus->busy_pages;
    VEEPROM_THROW(count <= free_pages, ERROR_NOMEM);

    rbnode *first = NULL;
    int pageno = 0;
    for (; pageno < count; pageno++) {
        int physnum = vstatus->next_alloc;
        VEEPROM_THROW(physnum != -1, ERROR_NOMEM);
        int ret = OK;
        if (first != NULL)
            ret = set_receiving(physnum, NULL, vstatus);
        else
            ret = set_receiving(physnum, &first, vstatus);
        VEEPROM_THROW(ret == OK, ret);
        ret = set_next_alloc(vstatus);
        VEEPROM_THROW(ret == OK, ret);
    }

    return move_cursor(first, cursor);
}


static int alloc_space(vrw_cursor *cursor) {
    int size = cursor->aligned_length_2b + 3;
    veeprom_status *vstatus = cursor->vstatus;
    if (size <= FLASH_PAGE_SIZE_2B - VEEPROM_PAGE_HEADER_2B) {

        rbtree *page_order = vstatus->page_order;
        rbnode *node = rb_min_node(page_order, page_order->root);
        for (; !rb_is_nullnode(page_order, node);
                node=rb_next_node(page_order, node)) {
            VEEPROM_THROW(node->data != NULL, ERROR_NULLPTR);
            vpage_status *pstatus = (vpage_status*)node->data;
            if (!is_fragmented(pstatus) && pstatus->free_space >= size*2) {
                int ret = move_cursor(node, cursor);
                VEEPROM_THROW(ret == OK, ret);
                vpage_status *p = (vpage_status*)cursor->virtpage->data;
                VEEPROM_THROW(p != NULL, ERROR_NULLPTR);
                cursor->p_cur = cursor->p_start_page + FLASH_PAGE_SIZE_2B -
                        p->free_space/2;
                return OK;
            }
        }
    }

    return alloc_pages(size, cursor);
}


static int set_receiving(int physnum, rbnode **f, veeprom_status *vstatus) {
    VEEPROM_THROW(vstatus->busy_pages + 1 <= VEEPROM_PAGE_COUNT, ERROR_NOMEM);

    uint16_t *p = vstatus->flash_start + physnum * FLASH_PAGE_SIZE_2B;
    int ret = flash_write_short(PAGE_RECEIVING, p);
    VEEPROM_THROW(ret == OK, ret);

    rbtree *page_order = vstatus->page_order;

    uint16_t counter = 0;
    {
        rbnode *node = rb_max_node(page_order, page_order->root);
        if (rb_is_nullnode(page_order, node)) {
            ret = flash_write_short(counter, p + 1);
        } else {
            VEEPROM_THROW(node != NULL, ERROR_NULLPTR);
            counter = ((vpage_status*)node->data)->counter + 1;
            VEEPROM_THROW(counter < FLASH_RESOURCE, ERROR_FLEXP);
            ret = flash_write_short(counter, p + 1);
        }
    }
    VEEPROM_THROW(ret == OK, ret);

    vstatus->busy_map[physnum] = -1;
    vpage_status *pstatus = create_pstatus();
    pstatus->counter = counter;
    pstatus->physnum = physnum;
    pstatus->free_space = FLASH_PAGE_SIZE - VEEPROM_PAGE_HEADER;
    pstatus->fragments = 0;
    rbnode *node = rb_create_node();
    VEEPROM_THROW(node != NULL, ERROR_NULLPTR);
    node->data = (void*)pstatus;
    rb_insert_node(page_order, node);
    vstatus->busy_pages++;

    if (f != NULL)
        *f = node;

    return OK;
}


static int
set_valid(uint16_t *page, vpage_status *pstatus, veeprom_status *vstatus) {
    VEEPROM_THROW(pstatus != NULL, ERROR_NULLPTR);
    int ret = flash_write_short(PAGE_VALID, page);
    if (ret != OK) {
        erase_page(page, pstatus, vstatus);
        VEEPROM_THROW(0, ret);
    }
    return OK;
}


static int receiving_to_valid(veeprom_status *vstatus) {
    int ret = OK;
    rbtree *page_order = vstatus->page_order;
    rbnode *node = rb_max_node(page_order, page_order->root);
    for (; !rb_is_nullnode(page_order, node);
            node=rb_prev_node(page_order, node)) {
        VEEPROM_THROW(node->data != NULL, ERROR_NULLPTR);
        vpage_status *pstatus = (vpage_status*)node->data;
        uint16_t *p = get_page(pstatus, vstatus);
        VEEPROM_THROW(p != NULL, ERROR_NULLPTR);
        int s = get_page_status(p);
        switch (s) {
        case PAGE_VALID:
            break;
        case PAGE_RECEIVING:
            ret = set_valid(p, pstatus, vstatus);
            VEEPROM_THROW(ret == OK, ret);
            continue;
        default:
            VEEPROM_THROW(0, ERROR_UNKNOWNSTATUS);
        }
    }
    return ret;
}


static int erase_receiving(veeprom_status *vstatus) {
    rbtree *page_order = vstatus->page_order;
    rbnode *node = rb_max_node(page_order, page_order->root);
    while (!rb_is_nullnode(page_order, node)) {
        VEEPROM_THROW(node->data != NULL, ERROR_NULLPTR);
        vpage_status *pstatus = (vpage_status*)node->data;
        uint16_t *p = get_page(pstatus, vstatus);
        VEEPROM_THROW(p != NULL, ERROR_NULLPTR);
        int s = get_page_status(p);
        switch (s) {
        case PAGE_VALID:
            return OK;
        case PAGE_RECEIVING:
        {
            rbnode *prev = rb_prev_node(page_order, node);
            int ret = remove_page(p, node, pstatus, vstatus);
            VEEPROM_THROW(ret == OK, ret);
            node = prev;
        }
        continue;
        default:
            VEEPROM_THROW(0, ERROR_UNKNOWNSTATUS);
        }
    }
    VEEPROM_THROW(0, ERROR_DCNSTY);
}


static inline int check_cursor_cur(vrw_cursor *cursor) {
    uint16_t *start = cursor->vstatus->flash_start;
    uint16_t *end = start + VEEPROM_PAGE_COUNT * FLASH_PAGE_SIZE;
    VEEPROM_THROW(cursor->p_cur >= start && cursor->p_cur < end,
            ERROR_PAGEALLOC);
    return OK;
}


static int
write_data(uint16_t id, uint8_t *data, uint16_t length,
        vrw_cursor *cursor) {

    int ret = alloc_space(cursor);
    VEEPROM_THROW(ret == OK, ret);
    ret = check_cursor_cur(cursor);
    VEEPROM_THROW(ret == OK, ret);

    cursor->cur_checksum = 0;
    VEEPROM_THROW(cursor->data != NULL, ERROR_NULLPTR);
    cursor->data->id = id;
    cursor->data->length = length;
    VEEPROM_THROW((ret=write_2b(id, cursor)) == OK, ret);
    cursor->data->p_start_data = cursor->p_cur;
    move_forward(cursor);

    VEEPROM_THROW((ret=write_2b(length, cursor)) == OK, ret);
    move_forward(cursor);
    {
        int i = 0;
        for (; i < cursor->aligned_length_2b - 1; i++) {
            uint16_t d = (*(data + 2*i) << 8) | (*(data + 2*i + 1));
            VEEPROM_THROW((ret=write_2b(d, cursor)) == OK, ret);
            move_forward(cursor);
        }
    }

    if (cursor->aligned_length_2b > 0) {
        uint16_t d;
        if ((length >> 1) < cursor->aligned_length_2b)
            d = (*(data + length - 1));
        else
            d = (*(data + length - 2)) | (*(data + length - 1) << 8); //LE
            VEEPROM_THROW((ret=write_2b(d, cursor)) == OK, ret);
            move_forward(cursor);
    }

    VEEPROM_THROW((ret=write_2b(cursor->cur_checksum, cursor)) == OK, ret);

    return OK;
}


static
int _veeprom_write(uint16_t id, uint8_t *data, int length,
        veeprom_status *vstatus) {

    vrw_cursor *cursor = create_cursor(vstatus);
    init_cursor(cursor, vstatus);
    cursor->aligned_length_2b = (length + (length & 1)) >> 1;

    int ret = write_data(id, data, length, cursor);
    if (ret != OK) {
        erase_receiving(vstatus);
        release_cursor(cursor);
        VEEPROM_THROW(0, ret);
    }

    ret = receiving_to_valid(vstatus);
    VEEPROM_THROW(ret == OK, ret, release_cursor(cursor));

    ret = add_data(cursor);
    release_cursor(cursor);

    VEEPROM_THROW(ret == OK, ret);
    return OK;
}


int veeprom_write(uint16_t id, uint8_t *data, int length,
        veeprom_status *vstatus) {

    VEEPROM_THROW(data != NULL && vstatus != NULL, ERROR_NULLPTR);
    VEEPROM_THROW(id > 0 && id < 0xFFFF && length < 0xFFFF && length >=0,
            ERROR_PARAM);

    int ret = _veeprom_write(id, data, length, vstatus);
    VEEPROM_THROW(ret == OK, ret);
    VEEPROM_THROW((ret=gc(vstatus)) == OK, ret);
    return OK;
}


static void iter_id(vrw_cursor *cursor) {
    uint16_t value = *(cursor->p_cur);
    vpage_status *pstatus = (vpage_status*)cursor->virtpage->data;
    switch (value) {
    case 0x0000:
        if (!is_replay(cursor)) {
            pstatus->fragments += pstatus->free_space;
            pstatus->fragments += 2;
            pstatus->free_space = 0;
        }
        break;
    case 0xFFFF:
        if (!is_replay(cursor))
            pstatus->free_space += 2;
        break;
    default:
        if (!is_replay(cursor)) {
            pstatus->fragments += pstatus->free_space;
            pstatus->free_space = 0;
        }
        cursor->data->p_start_data = cursor->p_cur;
        cursor->data->id = value;
        set_data_status(cursor, VRW_ID_FINISHED);
        cursor->rw_ops++;
        break;
    }
}


static void iter_length(vrw_cursor *cursor) {
    uint16_t value = *(cursor->p_cur);
    switch (value) {
    case 0x00:
        set_data_status(cursor, VRW_LENGTH_FINISHED);
        cursor->data->length = 0;
        cursor->aligned_length_2b = 0;
        cursor->rw_ops++;
        break;
    case 0xFFFF:
        set_data_status(cursor, VRW_FAILED);
        cursor->data->p_end_data = cursor->p_cur;
        cursor->aligned_length_2b = 0xFFFF;
        cursor->data->length = 0xFFFF;
        move_backward(cursor);
        break;
    default:
        cursor->aligned_length_2b = (value + (value & 1)) >> 1;
        cursor->data->length = value;
        set_data_status(cursor, VRW_LENGTH_FINISHED);
        cursor->rw_ops++;
        break;
    }
}


static void iter_data_chunks(vrw_cursor *cursor) {
    if (cursor->rw_ops == 2)
        cursor->cur_checksum =
                (cursor->data->id) ^ (cursor->data->length);

    if (cursor->rw_ops - 2 < cursor->aligned_length_2b) {
        cursor->cur_checksum ^= *(cursor->p_cur);
        cursor->rw_ops++;
    } else {
        set_data_status(cursor, VRW_DATA_FINISHED);
    }
}


static void iter_checksum(vrw_cursor *cursor) {
    uint16_t value = *(cursor->p_cur);
    if (cursor->cur_checksum == value) {
        set_data_status(cursor, VRW_OK);
        cursor->data->checksum = value;
        cursor->data->p_end_data = cursor->p_cur;
    } else {
        set_data_status(cursor, VRW_FAILED);
        cursor->data->p_end_data = cursor->p_cur;
    }
    cursor->rw_ops++;
}


static int iter_data(vrw_cursor *cursor) {
    for (; !end_of_page(cursor); move_forward(cursor)) {
        switch (get_data_status(cursor)) {
        case VRW_CLEAN:
            iter_id(cursor);
            break;
        case VRW_ID_FINISHED:
            iter_length(cursor);
            break;
        case VRW_LENGTH_FINISHED:
            iter_data_chunks(cursor);
            if (get_data_status(cursor) == VRW_DATA_FINISHED)
                iter_checksum(cursor);
            break;
        case VRW_FAILED:
            return OK;
        case VRW_OK:
            return OK;
        default:
            VEEPROM_THROW(0, ERROR_UNKNOWNSTATUS);
        }
        int data_status = get_data_status(cursor);
        if (data_status == VRW_FAILED || data_status == VRW_OK)
            return OK;
    }

    return OK;
}


vdata* veeprom_read(uint16_t id, veeprom_status *vstatus) {
    if (vstatus == NULL)
        return NULL;

    rbnode *node = NULL;
    rbtree *ids = vstatus->ids;
    {
        vdata data;
        data.p = &id;
        node = rb_search_node(ids, &data);
        if (node == NULL)
            return NULL;
    }

    if (node == ids->nullnode) {
        VEEPROM_LOGDEBUG("not found id=%d\n", id);
        return NULL;
    }

    return (vdata*)node->data;
}


int veeprom_delete(uint16_t id, veeprom_status *vstatus) {
    VEEPROM_THROW(vstatus != NULL, ERROR_NULLPTR);

    rbnode *node = NULL;
    rbtree *ids = vstatus->ids;
    {
        vdata data;
        data.p = &id;
        node = rb_search_node(ids, &data);
        VEEPROM_THROW(node != NULL, ERROR_NULLPTR);
    }

    if (node == ids->nullnode) {
        VEEPROM_LOGDEBUG("not found id=%d\n", id);
        return OK;
    }

    vrw_cursor *rcursor = create_cursor();
    init_cursor(rcursor, vstatus);
    rcursor->p_cur = ((vdata*)node->data)->p;
    int ret = remove_data(rcursor, id);
    if (ret != OK) {
        release_cursor(rcursor);
        VEEPROM_THROW(0, ret);
    }

    ids = rb_delete_node(ids, node);
    release_vdata(node->data);
    rb_release_node(node);
    release_cursor(rcursor);

    VEEPROM_THROW((ret=gc(vstatus)) == OK, ret);
    return OK;
}
