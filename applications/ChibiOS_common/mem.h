/*
 *  mem.h
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


#ifndef VEEPROM_MEM_H
#define VEEPROM_MEM_H


#include "ch.h"
#include "chmempools.h"
#include "rbtree.h"
#include "eeprom.h"

#define DECLARE_POOL(what) extern memory_pool_t what ## _pool; 

DECLARE_POOL(vpage_status);
DECLARE_POOL(veeprom_status);
DECLARE_POOL(vrw_cursor);
DECLARE_POOL(vrw_data);
DECLARE_POOL(vdata);
DECLARE_POOL(rbtree);
DECLARE_POOL(rbnode);

#undef DECLARE_POOL

#define VEEPROM_Alloc(what) chPoolAlloc(&what ## _pool);
#define VEEPROM_Dealloc(what, pointer) chPoolFree(&what ## _pool, pointer);


#endif
