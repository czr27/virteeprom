/*
 *  flash.h
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


#ifndef VEEPROM_FLASH_H
#define VEEPROM_FLASH_H

#include "types.h"

int flash_write_short(uint16_t data, uint16_t *p);

int flash_write_int(uint32_t data, uint32_t *p);

int flash_zero_short(uint16_t *p);

int flash_erase_page(uint16_t *p);


#endif
