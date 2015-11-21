/*
 *  log.h
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


#ifndef VEEPROM_LOG_H
#define VEEPROM_LOG_H

#include "ch.h"
#include "shell.h"
#include "chprintf.h"
extern const ShellConfig shell_cfg;


#define VEEPROM_Log(etype, args...) chprintf(shell_cfg.sc_channel, args);


#endif
