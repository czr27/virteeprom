/*
 *  wrappers.h
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


#ifndef VEEPROM_WRAPPERS_H
#define VEEPROM_WRAPPERS_H


#include "log.h"


#define __WRAPPER(some_code) do{\
    some_code\
} while(0);


/*
 * The log types could be used in case of an implementation of VEEPROM_Log 
 * supports log priorities. 
 */
#define VEEPROM_LOGTYPE_DEBUG 0
#define VEEPROM_LOGTYPE_INFO 1
#define VEEPROM_LOGTYPE_ERROR 2
#define VEEPROM_LOGTYPE_CRASH 3



#ifdef VEEPROM_DEBUG 

    #define VEEPROM_LOG_PREFIX "[%s:%d] "
    #define VEEPROM_LOG_PREFIX_ARGS __FILE__, __LINE__

    #define VEEPROM_LOGDEBUG(format, args...) \
        VEEPROM_Log(VEEPROM_LOGTYPE_DEBUG, VEEPROM_LOG_PREFIX format, VEEPROM_LOG_PREFIX_ARGS, ## args);

    #define VEEPROM_LOGERROR(format, args...) \
        VEEPROM_Log(VEEPROM_LOGTYPE_ERROR, VEEPROM_LOG_PREFIX format, VEEPROM_LOG_PREFIX_ARGS, ## args);

    #define VEEPROM_LOGINFO(format, args...) \
        VEEPROM_Log(VEEPROM_LOGTYPE_INFO, VEEPROM_LOG_PREFIX format, VEEPROM_LOG_PREFIX_ARGS, ## args);

#else

    #define VEEPROM_LOGDEBUG(args...)

    #define VEEPROM_LOGERROR(format, args...) \
        VEEPROM_Log(VEEPROM_LOGTYPE_ERROR, format, ## args);

    #define VEEPROM_LOGINFO(format, args...) \
        VEEPROM_Log(VEEPROM_LOGTYPE_INFO, format, ## args);

#endif


/*
 * If the invariant is false then log error and return error code
 * upward for the following consideration.
 */
#define VEEPROM_THROW(invariant, errnum, actions...) __WRAPPER(\
        if (!(invariant)) {\
            VEEPROM_LOGERROR("%s\n", emsg(errnum));\
            actions;\
            return errnum;\
        }\
)


/*
 * If the invariant is false then log error without compulsory return.
 */
#define VEEPROM_TRACE(invariant, errnum, actions...) __WRAPPER(\
        if (!(invariant)) {\
            VEEPROM_LOGERROR("%s\n", emsg(errnum));\
            actions;\
        }\
)


#endif
