/*
 *  eeprom.h
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

#ifndef VEEPROM_EEPROM_H
#define VEEPROM_EEPROM_H

#include "flash_cfg.h"
#include "flash.h"
#include "types.h"
#include "rbtree.h"

#define VEEPROM_PAGE_HEADER 4

#define VEEPROM_PAGE_HEADER_2B 2

#define PAGE_ERASED 0xFFFF
#define PAGE_RECEIVING 0xAAAA
#define PAGE_VALID 0x0000

#define VRW_REPLAY 0x200
#define VRW_PAGE_FINISHED 0x100

#define VEEPROM_MAX_VIRTNUM 0xfffe

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define FLASH_RESOURCE 0xFFFF

struct vpage_status {
    int16_t counter;
    int16_t physnum;
    int16_t fragments;
    int16_t free_space;
};
typedef struct vpage_status vpage_status;


struct veeprom_status {
    struct rbtree *page_order;
    int16_t busy_map[FLASH_PAGE_COUNT];
    int busy_pages;
    struct rbtree *ids;
    uint16_t *flash_start;
    int16_t next_alloc;
};
typedef struct veeprom_status veeprom_status;


enum {
    VRW_CLEAN,
    VRW_ID_FINISHED,
    VRW_LENGTH_FINISHED,
    VRW_DATA_FINISHED,
    VRW_CHECKSUM_FINISHED,
    VRW_OK,
    VRW_FAILED,
};


struct vrw_data {
    uint16_t id;
    uint16_t length;
    uint16_t *p_start_data;
    uint16_t *p_end_data;
    uint16_t checksum;
};
typedef struct vrw_data vrw_data;


struct vrw_cursor {
    veeprom_status *vstatus;
    rbnode *virtpage;
    vrw_data *data;
    uint16_t *p_cur;
    uint16_t *p_start_page;
    uint16_t *p_end_page;
    uint16_t cur_checksum;
    uint16_t flags;
    uint16_t rw_ops;
    uint16_t aligned_length_2b;
} __attribute__((packed, aligned(4)));
typedef struct vrw_cursor vrw_cursor;


struct vdata {
    uint16_t *p;
};
typedef struct vdata vdata;


int veeprom_init(veeprom_status *s);

veeprom_status *veeprom_create_status();

int veeprom_status_init(veeprom_status *s, uint16_t *flash_start);

int veeprom_status_release(veeprom_status *s);

int veeprom_write(uint16_t id, uint8_t *data, int length,
        veeprom_status *vstatus);

vdata* veeprom_read(uint16_t id, veeprom_status *vstatus);

int veeprom_delete(uint16_t id, veeprom_status *vstatus);

int veeprom_clean(veeprom_status *vstatus);

#endif
