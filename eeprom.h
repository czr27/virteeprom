/*
 *  eeprom.h
 *
 *  This file is a part of VirtEEPROM, emulation of EEPROM (Electrically
 *  Erasable Programmable Read-only Memory).
 *
 *  (C) 2017  Nina Evseenko <anvoebugz@gmail.com>
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

/*
 * VEEPROM header locates at the top of a page.
 * First chunk  - page status
 * Second chunk - virtual number
 */
#define VEEPROM_HEADER_CHUNKS    2
#define VEEPROM_HEADER_SIZE     (VEEPROM_HEADER_CHUNKS * sizeof(flash_chunk_t))


/*
 * It's supposed that every flash page divisible by chunks equal size.
 */
#define FLASH_PAGE_CHUNKS             (FLASH_PAGE_SIZE / sizeof(flash_chunk_t))


/*
 * Define the number of chunks depending on the platform.
 */
#define TO_CHUNKS_16(v)  ((v + (v & 1)) >> 1)
#define TO_CHUNKS_32(v)  (v / 4 + (((v & 0x03) >> 1) | (v & 0x01)))
#define TO_CHUNKS_64(v)  (v / 8 + (((v & 0x07) >> 2) | ((v & 0x02) >> 1) | (v & 0x01) ))


#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define VEEPROM_BUSY_PAGE_FLAG -1

#ifndef VEEPROM_DEBUG
#define VEEPROM_MODULE(t) static t
#else
#define VEEPROM_MODULE(t) t
#endif


typedef struct {
    int16_t busy_map[FLASH_PAGE_COUNT];
    int busy_pages;
    flash_chunk_t *flash_start;
    int16_t next_alloc;
    int flags;
} veeprom_status_t;


typedef struct {
    flash_chunk_t *p_start_page;
    flash_chunk_t *p_current;
    flash_chunk_t checksum;
    int index;
} veeprom_cursor_t;


typedef struct {
    flash_chunk_t id;
    uint8_t *buf;
    int buf_size;
    int length;
    flash_chunk_t *checksum;
} veeprom_read_t;


enum {
    VEEPROM_NOTINITIALIZED = 0x00,
    VEEPROM_INITIALIZED = 0x01
};


/* API functions */

int veeprom_init(flash_chunk_t *flash_start);

int veeprom_deinit();

int veeprom_write(flash_chunk_t id, uint8_t *data, flash_chunk_t length);

int veeprom_read(veeprom_read_t *read_buf);

int veeprom_delete(flash_chunk_t id);

flash_chunk_t* veeprom_find(flash_chunk_t id);

#ifdef CHIBIOS_ON
int veeprom_clean();
#endif


/* For debug and testing purposes */
veeprom_status_t* veeprom_get_status();
flash_chunk_t**   veeprom_get_pages();
int               veeprom_get_pages_size();
flash_chunk_t**   veeprom_get_ids();
int               veeprom_get_ids_size();
veeprom_cursor_t* veeprom_get_cursor();

#endif
