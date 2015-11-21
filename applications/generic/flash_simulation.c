/*
 *  flash_simulation.c
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


#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "wrappers.h"
#include "flash_cfg.h"
#include "flash.h"
#include "errnum.h"
#include "errmsg.h"


int flash_write_short(uint16_t data, uint16_t *p) {
    *p = data;  
    return 0;
}


int flash_write_int(uint32_t data, uint32_t *p) {
    *p = data;  
    return 0;
}


int flash_zero_short(uint16_t *p) {
    *p = 0;
    return OK;
}


int flash_erase_page(uint16_t *p) {
    return !((void*)p == memset((void*)p, 0xFF, FLASH_PAGE_SIZE));
}


void* flash_init(int fd) {
    VEEPROM_TRACE(fd > 2, ERROR_PARAM, return NULL;);
    int length = FLASH_PAGE_SIZE * FLASH_PAGE_COUNT;
    return mmap(NULL, length, PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
}


int flash_uninit(void *p) {
    int ret = OK;
    VEEPROM_THROW((ret=munmap(p, FLASH_PAGE_SIZE * FLASH_PAGE_COUNT)) == 0,
            ERROR_SYSTEM);
    return OK;
}
