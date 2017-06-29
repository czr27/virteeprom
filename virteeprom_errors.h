/*
 *  virteeprom_errors.h
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


/*
 * Errors pertain to VirtEEPROM layer
 */
__errnum_message__(VEEPROM_ERROR_PAGEALLOC, ("veeprom page alloc error"))
__errnum_message__(VEEPROM_ERROR_NOMEM, ("veeprom no memory"))
__errnum_message__(VEEPROM_ERROR_INIT, ("veeprom not initialized"))
__errnum_message__(VEEPROM_ERROR_ID, ("veeprom id error"))
__errnum_message__(VEEPROM_ERROR_LENGTH, ("veeprom length error"))
__errnum_message__(VEEPROM_ERROR_DATA, ("veeprom data error"))
__errnum_message__(VEEPROM_ERROR_CHECKSUM, ("veeprom checksum error"))
__errnum_message__(VEEPROM_ERROR_VIRTNUM, ("veeprom wrong virtnum"))
__errnum_message__(VEEPROM_ERROR_ID_NOTFOUND, ("veeprom id not found"))
__errnum_message__(VEEPROM_ERROR_BUFSIZE, ("veeprom insufficient buffer size"))


/*
 * Flash related errors
 */
__errnum_message__(ERROR_FLASH_EXPIRED, ("warning: flash read/write operations are limited"))
__errnum_message__(ERROR_FLASH_ASSERT, ("flash error"))
__errnum_message__(ERROR_FLASH_WRITE, ("flash write error"))
__errnum_message__(ERROR_FLASH_WRP, ("flash write protects error"))
__errnum_message__(ERROR_FLASH_ERASE, ("flash erase error"))
