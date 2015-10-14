/*
 *  examine.h
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


#ifndef TOOLBOX_EXAMINE_H
#define TOOLBOX_EXAMINE_H

#ifndef CHIBIOS_ON
#include <stdio.h>
#else
#include "ch.h"
#include "shell.h"
#include "chprintf.h"
extern const ShellConfig shell_cfg;
#endif

#define __WRAPPER(some_code) do{\
    some_code\
} while(0)


#ifndef CHIBIOS_ON
#define COND_ERROR_RET(cond, ret)  __WRAPPER(\
    if (!(cond)) {\
        fprintf(stderr, "ERROR %s:%d %s (%d)\n", __FILE__, __LINE__, emsg(ret), ret);\
        return ret;\
    }\
)
#else
#define COND_ERROR_RET(cond, ret)  __WRAPPER(\
    if (!(cond)) {\
        chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s (%d)\r\n", __FILE__, __LINE__, emsg(ret), ret);\
        return ret;\
    }\
)
#endif


#ifndef CHIBIOS_ON
#define COND_ERROR_RET2(cond, text, ret)  __WRAPPER(\
    if (!(cond)) {\
        fprintf(stderr, "ERROR %s:%d %s\n", __FILE__, __LINE__, text);\
        return ret;\
    }\
)
#else
#define COND_ERROR_RET2(cond, text, ret)  __WRAPPER(\
    if (!(cond)) {\
        chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s\r\n", __FILE__, __LINE__, text);\
        return ret;\
    }\
)
#endif


#ifndef CHIBIOS_ON
#define COND_ERROR(cond, text)  __WRAPPER(\
    if (!(cond)) {\
        fprintf(stderr, "ERROR %s:%d %s\n", __FILE__, __LINE__, text);\
    }\
)
#else
#define COND_ERROR(cond, text)  __WRAPPER(\
    if (!(cond)) {\
        chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s\r\n", __FILE__, __LINE__, text);\
    }\
)
#endif


#ifndef CHIBIOS_ON
#define COND_ERROR2(cond, ret)  __WRAPPER(\
    if (!(cond)) {\
        fprintf(stderr, "ERROR %s:%d %s (%d)\n", __FILE__, __LINE__, emsg(ret), ret);\
    }\
)
#else
#define COND_ERROR2(cond, text)  __WRAPPER(\
    if (!(cond)) {\
        chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s (%d)\r\n", __FILE__, __LINE__, emsg(ret), ret);\
    }\
)
#endif


#ifndef CHIBIOS_ON
#define ERROR_RET(ret)  __WRAPPER(\
    fprintf(stderr, "ERROR %s:%d %s (%d)\n", __FILE__, __LINE__, emsg(ret), ret);\
        return ret;\
)
#else
#define ERROR_RET(ret)  __WRAPPER(\
    chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s (%d)\r\n", __FILE__, __LINE__, emsg(ret), ret);\
        return ret;\
)
#endif


#ifndef CHIBIOS_ON
#define ERROR_TRET(text, ret)  __WRAPPER(\
    fprintf(stderr, "ERROR %s:%d %s (%d)\n", __FILE__, __LINE__, text, ret);\
        return ret;\
)
#else
#define ERROR_TRET(text, ret)  __WRAPPER(\
    chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s (%d)\r\n", __FILE__, __LINE__, text, ret);\
        return ret;\
)
#endif


#ifndef CHIBIOS_ON
#define ERROR(text)  __WRAPPER(\
    fprintf(stderr, "ERROR %s:%d %s\n", __FILE__, __LINE__, text);\
)
#else
#define ERROR(text)  __WRAPPER(\
    chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s\r\n", __FILE__, __LINE__, text);\
)
#endif


#ifndef CHIBIOS_ON 
#define DEBUG_MSG(fmt, val)  __WRAPPER(\
    fprintf(stderr, "DEBUG %s:%d ", __FILE__, __LINE__);\
    fprintf(stderr, fmt, val);\
    fprintf(stderr, "\n");\
)
#else
#define DEBUG_MSG(fmt, val)  __WRAPPER(\
    chprintf(shell_cfg.sc_channel, "DEBUG %s:%d ", __FILE__, __LINE__);\
    chprintf(shell_cfg.sc_channel, fmt, (val));\
    chprintf(shell_cfg.sc_channel, "\r\n");\
)
#endif


#ifndef CHIBIOS_ON
#define COND_ERROR_RET_F(cond, ret, f)  __WRAPPER(\
    if (!(cond)) {\
        fprintf(stderr, "ERROR %s:%d %s (%d)\n", __FILE__, __LINE__, emsg(ret), ret);\
        f;\
        return ret;\
    }\
)
#else
#define COND_ERROR_RET_F(cond, ret, f)  __WRAPPER(\
    if (!(cond)) {\
        chprintf(shell_cfg.sc_channel, "ERROR %s:%d %s (%d)\r\n", __FILE__, __LINE__, emsg(ret), ret);\
        f;\
        return ret;\
    }\
)
#endif



#endif
