/*
 *  flash.h
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


#ifndef VEEPROM_FLASH_H
#define VEEPROM_FLASH_H

#include "types.h"

int init_blocks();

int flash_invert(flash_chunk_t *p);

int flash_write_chunk(flash_chunk_t data, flash_chunk_t *addr);

int flash_erase_page(flash_chunk_t *p);


#endif
