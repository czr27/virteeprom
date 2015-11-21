/*
 *  mem.c
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


#include "mem.h"

#define DEFINE_POOL(what) typedef struct what what; MEMORYPOOL_DECL(what ## _pool, sizeof(what), chCoreAllocI);

DEFINE_POOL(vpage_status);
DEFINE_POOL(veeprom_status);
DEFINE_POOL(vrw_cursor);
DEFINE_POOL(vrw_data);
DEFINE_POOL(vdata);
DEFINE_POOL(rbtree);
DEFINE_POOL(rbnode);

#undef DEFINE_POOL
