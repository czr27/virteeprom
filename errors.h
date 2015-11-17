/*
 *  errors.h
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


__errno_message__(OK, ("no error"))
__errno_message__(ERROR_UNKNOWN, ("unknown error"))
__errno_message__(ERROR_NULLPTR, ("null pointer"))
__errno_message__(ERROR_UNKNOWNSTATUS, ("uknown status"))
__errno_message__(ERROR_INVORDER, ("invalid order"))
__errno_message__(ERROR_DCNSTY, ("data consistency error"))
__errno_message__(ERROR_FLASHWRT, ("flash write error"))
__errno_message__(ERROR_PAGEALLOC, ("page alloc error"))
__errno_message__(ERROR_NOMEM, ("no memory"))
__errno_message__(ERROR_PARAM, ("bad parameter"))
__errno_message__(ERROR_VDLT, ("deletion error"))
__errno_message__(ERROR_NOTFOUND, ("not found"))
__errno_message__(ERROR_VALUE, ("wrong value"))
__errno_message__(ERROR_DFG, ("defragmentation failed"))
__errno_message__(ERROR_WRT, ("writing failed"))
__errno_message__(ERROR_SYSTEM, ("system error"))
__errno_message__(ERROR_OBNDS, ("out of bounds"))
__errno_message__(ERROR_EFAIL, ("expected fail"))
__errno_message__(ERROR_FLEXP, ("flash expired"))
__errno_message__(ERROR_FLASHERASE, ("flash erase error"))
__errno_message__(ERROR_FLASHASSERT, ("flash assert"))
