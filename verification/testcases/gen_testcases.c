/*
 *  gen_testcases.c
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "examine.h"
#include "flash_cfg.h"
#include "flash.h"
#include "errnum.h"
#include "errmsg.h"
#include "gen_testcases.h"


#define PAGE_RECEIVING 0xAAAA
#define PAGE_VALID 0x0000
#define PAGE_ERASED 0xFFFF

#define VEEPROM_PAGE_HEADER 4
#define VEEPROM_PAGE_HEADER_2B 2

#define VEEPROM_PAGE_STATUS_BITS 0xc000
#define VEEPROM_PAGE_STATUS_SHIFT 14

#define VEEPROM_COUNTER_BITS 0x3FFF



#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))


static int write_hw(int value, FILE *file) {
    uint16_t hw = value & 0xFFFF;
    return fwrite(&hw, sizeof(hw), 1, file);
}


static int write_id_length(uint16_t id, uint16_t length, FILE *file) {
    int hw_written = 0;
    int written = fwrite(&id, sizeof(uint16_t), 1, file);
    COND_ERROR_RET(written == 1, ERROR_WRT);
    hw_written += 1;

    written = fwrite(&length, sizeof(uint16_t), 1, file);
    COND_ERROR_RET(written == 1, -ERROR_WRT);
    hw_written += 1;

    return hw_written;
}


static int write_data(uint16_t id, uint16_t length, FILE *file) {
    int hw_written = write_id_length(id, length, file);

    uint16_t checksum = id ^ length;
    int i = 0;
    int aligned_length_2b = (length + (length & 1)) >> 1;
    for (; i < aligned_length_2b; i++) {
        int written = fwrite(&i, sizeof(uint16_t), 1, file);
        COND_ERROR_RET(written == 1, -ERROR_WRT);
        hw_written += 1;
        checksum ^= i; 
    }

    int written = fwrite(&checksum, sizeof(uint16_t), 1, file);
    COND_ERROR_RET(written == 1, -ERROR_WRT);
    hw_written += 1;

    return hw_written;
}


static int write_raw(uint16_t num, uint16_t sval, FILE *file) {
    int i = 0;
    int hw_written = 0;
    for (; i < num; i++, sval++) {
        int written = fwrite(&sval, sizeof(uint16_t), 1, file);
        COND_ERROR_RET(written == 1, -ERROR_WRT);
        hw_written += 1;
    }
    return hw_written;
}


static int write_data_checksum(uint16_t id, uint16_t length,
        uint16_t checksum, FILE *file) {
    int hw_written = write_id_length(id, length, file);
    int i = 0;
    int aligned_length_2b = (length + (length & 1)) >> 1;
    for (; i < aligned_length_2b; i++) {
        int written = fwrite(&i, sizeof(uint16_t), 1, file);
        COND_ERROR_RET(written == 1, -ERROR_WRT);
        hw_written += 1;
    }

    int written = fwrite(&checksum, sizeof(uint16_t), 1, file);
    COND_ERROR_RET(written == 1, -ERROR_WRT);
    hw_written += 1;

    return hw_written;
}


int write_header(FILE *file, int status, int num) {
    int written = fwrite(&status, sizeof(uint16_t), 1, file);
    COND_ERROR_RET(written == 1, ERROR_WRT);
    written = fwrite(&num, sizeof(uint16_t), 1, file);
    COND_ERROR_RET(written == 1, ERROR_WRT);
    return 2;
}


static int fill_empty(int num, FILE *file) {
    int total_written = 0;
    uint16_t empty = 0xFFFF;
    int i = 0;
    for (; i < num; i++) {
        int written = fwrite(&empty, sizeof(uint16_t), 1, file);
        COND_ERROR_RET(written == 1, -ERROR_WRT);
        total_written++;
    }
    return total_written;
}


uint16_t calc_checksum(int id, int length) {
    uint16_t checksum = id ^ length;
    int alen = (length + (length & 1)) >> 1;
    uint16_t i = 0;
    for (; i < alen; i++)
        checksum ^= i;
    return checksum;
}


/*
 * Page 2, 4, 99 - receiving , others - erased
 */
int gen_verify_2(const char *filename) {
    FILE *file = fopen(filename, "w+");
    uint16_t e = 0xFFFF;
    uint16_t r = 0xAAAA;
    int i = 0;
    int written = 0;
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        for (i = 0; i < FLASH_PAGE_SIZE_2B; i++) {
            if (i == 0 && (page == 2 || page == 4 || page == 99))
                written = fwrite(&r, sizeof(uint16_t), 1, file);
            else if (i == 1 && (page == 2 || page == 4 || page == 99))
                written = fwrite(&page, sizeof(uint16_t), 1, file);
            else
                written = fwrite(&e, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - receiving , others - erased
 */
int gen_verify_3(const char *filename) {
    FILE *file = fopen(filename, "w+");
    uint16_t e = 0xFFFF;
    uint16_t r = 0xAAAA;
    int i = 0;
    int written = 0;
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        for (i = 0; i < FLASH_PAGE_SIZE_2B; i++) {
            if (i == 0 && (page == 0 || page == 1 || page == 99))
                written = fwrite(&r, sizeof(uint16_t), 1, file);
            else if (i == 1 && (page == 0 || page == 1 || page == 99))
                written = fwrite(&page, sizeof(uint16_t), 1, file);
            else
                written = fwrite(&e, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 2 is VALID with number=123, no data
 * Pages 0, 1, 99 - recevieving, no data
 * Others - erased
 */
int gen_verify_4(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 2) {
            int hw_written = write_header(file, PAGE_VALID, 123);
            int ret = OK;
            COND_ERROR_RET((ret=fill_empty(510, file)) > 0, ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 2 is VALID with number=0, no data
 * Pages 0, 1, 99 - recevieving, no data
 * Others - erased
 */
int gen_verify_5(const char *filename) {
    FILE *file = fopen(filename, "w+");
    uint16_t e = 0xFFFF;
    uint16_t r = 0xAAAA;
    uint16_t v = 0x0000;
    int i = 0;
    int written = 0;
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 2) {
            written = fwrite(&v, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
            uint16_t virtnum = 0;
            written = fwrite(&virtnum, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
            written = fill_empty(510, file);
            COND_ERROR_RET(written == 510, ERROR_WRT);
        } else {
            for (i = 0; i < FLASH_PAGE_SIZE_2B; i++) {
                if (i == 0 && (page == 0 || page == 1 || page == 99))
                    written = fwrite(&r, sizeof(uint16_t), 1, file);
                else
                    written = fwrite(&e, sizeof(uint16_t), 1, file);
                COND_ERROR_RET(written == 1, ERROR_WRT);
            }
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 2 - VALID - number=0
 * Page 3 - VALID - number=3
 * Pages 0, 1, 99 - RECEIVING (should be deleted)
 * Others - ERASED
 */
int gen_verify_6(const char *filename) {
    FILE *file = fopen(filename, "w+");
    uint16_t e = 0xFFFF;
    uint16_t r = 0xAAAA;
    int i = 0;
    int written = 0;
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 2) {
            int page_status = 0x0;
            written = fwrite(&page_status, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
            int page_number = 0x0000;
            written = fwrite(&page_number, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
            written = fill_empty(510, file);
            COND_ERROR_RET(written == 510, ERROR_WRT);
        } else if (page == 3) {
            int page_status = 0x0;
            written = fwrite(&page_status, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
            int page_number = 0x0003;
            written = fwrite(&page_number, sizeof(uint16_t), 1, file);
            COND_ERROR_RET(written == 1, ERROR_WRT);
            written = fill_empty(510, file);
            COND_ERROR_RET(written == 510, ERROR_WRT);
        } else {
            for (i = 0; i < FLASH_PAGE_SIZE_2B; i++) {
                if (i == 0 && (page == 0 || page == 1 || page == 99))
                    written = fwrite(&r, sizeof(uint16_t), 1, file);
                else
                    written = fwrite(&e, sizeof(uint16_t), 1, file);
                COND_ERROR_RET(written == 1, ERROR_WRT);
            }
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0 - RECEIVING - number=3
 * Page 99 - RECEIVING - number=4
 * Page 1 - RECEIVING - number=5
 * Page 20 - VALID - number=0
 * Page 127 - VALID - number=1
 * Others - erased, no data
 */
int gen_verify_7(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    int ret = 0;
    // v - 1 v - 2 r - 3 r - 4 r - 5
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 0) {
            int hw_written = write_header(file, PAGE_RECEIVING, 3);
            COND_ERROR_RET((ret=fill_empty(510, file)) > 0, ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        } else if (page == 99) {
            int hw_written = write_header(file, PAGE_RECEIVING, 4);
            COND_ERROR_RET((ret=fill_empty(510, file)) > 0, ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        } else if (page == 1) {
            int hw_written = write_header(file, PAGE_RECEIVING, 5);
            COND_ERROR_RET((ret=fill_empty(510, file)) > 0, ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        } else if (page == 20) {
            int hw_written = write_header(file, PAGE_VALID, 0);
            COND_ERROR_RET((ret=fill_empty(510, file)) > 0, ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        } else if (page == 127) {
            int hw_written = write_header(file, PAGE_VALID, 1);
            COND_ERROR_RET((ret=fill_empty(510, file)) > 0, ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        } else {
            int hw_written = 0;
            COND_ERROR_RET((ret=fill_empty(512, file)) > 0,    
                    ERROR_WRT);
            hw_written += ret;
            COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 77 - VALID - number=0
 * Page 44 - VALID - number=1
 * No data
 */
int gen_verify_8(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 44) {
            int w = write_header(file, PAGE_VALID, 1);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 77 - VALID - number=0
 * Page 44 - VALID - number=0
 * No data
 * EXPECTED: DEFRAGMENTATION FAILED
 */
int gen_verify_9(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 77 - VALID - number=0
 * Page 44 - VALID - number=0, two jags
 * No data
 * EXPECTED: all erased
 */
int gen_verify_10(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(10, file);
            uint16_t jag = 0x0000;
            w += fwrite(&jag, sizeof(jag), 1, file);
            w += fill_empty(10, file);
            w += fwrite(&jag, sizeof(jag), 1, file);
            w += fill_empty(488, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, no data
 * Page 77 - VALID - number=0, two jags, no data
 * EXPECTED: all erased
 */
int gen_verify_11(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(10, file);
            uint16_t jag = 0x0000;
            w += fwrite(&jag, sizeof(jag), 1, file);
            w += fill_empty(10, file);
            w += fwrite(&jag, sizeof(jag), 1, file);
            w += fill_empty(488, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, with data:
 *          top, length=0, correct checksum
 * Page 77 - VALID - number=0, 4 jags
 */
int gen_verify_12(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_data(243, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            w += fill_empty(507, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, with data:
 *          middle, length=0, correct checksum
 * Page 77 - VALID - number=0, 4 jags
 */
int gen_verify_13(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(20, file);
            int ret = write_data(243, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            w += fill_empty(487, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, with data:
 *          bottom, length=0, correct checksum
 * Page 77 - VALID - number=0, 4 jags
 * EXPECTED: gc -> Page 100 VALID with data from 44
 */
int gen_verify_14(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(507, file);
            int ret = write_data(243, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, with data:
 *          top, length=0, wrong checksum
 * Page 77 - VALID - number=0, 4 jags
 */
int gen_verify_15(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_id_length(243, 0, file);
            COND_ERROR_RET(ret == 2, -ret);
            w += ret;
            w += write_hw(777, file);
            w += fill_empty(507, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, with data:
 *          middle, length=0, wrong checksum
 * Page 77 - VALID - number=0, 4 jags
 */
int gen_verify_16(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(20, file);
            int ret = write_id_length(243, 0, file);
            COND_ERROR_RET(ret == 2, -ret);
            w += ret;
            w += write_hw(123, file);
            w += fill_empty(487, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, with data:
 *          bottom, length=0, wrong checksum
 * Page 77 - VALID - number=0, 4 jags
 */
int gen_verify_17(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(507, file);
            int ret = write_id_length(243, 0, file);
            COND_ERROR_RET(ret == 2, -ret);
            w += ret;
            w += write_hw(123, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0,1,99 - RECEIVING
 * Page 44 - VALID - number=0, top, invalid length (0xFFFF)
 * Page 77 - VALID - number=0
 */
int gen_verify_18(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_id_length(243, 0xFFFF, file);
            COND_ERROR_RET(ret == 2, -ret);
            w += ret;
            w += fill_empty(508, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, middle, invalid length (0xFFFF)
 * Page 77 - VALID - number=0
 */
int gen_verify_19(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(20, file);
            int ret = write_id_length(243, 0xFFFF, file);
            COND_ERROR_RET(ret == 2, -ret);
            w += ret;
            w += fill_empty(488, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - number=0, bottom, invalid length (0xFFFF)
 * Page 77 - VALID - number=0
 */
int gen_verify_20(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(508, file);
            int ret = write_id_length(243, 0xFFFF, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 77) {
            int w = write_header(file, PAGE_VALID, 0);
            int i = 0;
            for (; i < 4; i++) {
                w += fill_empty(10, file);
                uint16_t jag = 0x0000;
                w += fwrite(&jag, sizeof(jag), 1, file);
            }
            w += fill_empty(466, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - data at the top (id, length=1, correct checksum)
 */
int gen_verify_21(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_data(243, 1, file);
            COND_ERROR_RET(ret == 4, -ret);
            w += ret;
            w += fill_empty(506, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - data at the middle (id, length=1, correct checksum)
 */
int gen_verify_22(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(20, file);
            int ret = write_data(243, 1, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            w += fill_empty(486, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - data at the bottom (id, length=1, correct checksum)
 * EXPECTED: gc -> page 100 with data from 44
 */

int gen_verify_23(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(506, file);
            int ret = write_data(243, 1, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - data at the top (id, length=1, wrong checksum)
 */
int gen_verify_24(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_data_checksum(243, 1, 123, file);
            COND_ERROR_RET(ret == 4, -ret);
            w += ret;
            w += fill_empty(506, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - data at the middle (id, length=1, wrong checksum)
 */
int gen_verify_25(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(20, file);
            int ret = write_data_checksum(243, 1, 123, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            w += fill_empty(486, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 44 - VALID - data at the bottom (id, length=1, wrong checksum)
 */
int gen_verify_26(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 44) {
            int w = write_header(file, PAGE_VALID, 0);
            w += fill_empty(506, file);
            int ret = write_data_checksum(243, 1, 123, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 43 - VALID - number=0;
 *      data: length=1014, correct checksum
 */
int gen_verify_27(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 43) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_data(243, 1014, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Pages 0, 1, 99 - RECEIVING
 * Page 43 - VALID - number=0;
 *      data: length=1014, wrong checksum
 */
int gen_verify_28(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 43) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_data_checksum(243, 1014, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 0 || page == 1 || page == 99) {
            int w = write_header(file, PAGE_RECEIVING, page);
            w += fill_empty(510, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * 100 (1018) -> 32 (1022) -> 1 (29) + aligned(0)
 * length = 2069 -> aligned +1
 * correct checksum
 */
int gen_verify_29(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 100) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_id_length(123, 2069, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            ret = write_raw(508, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 32) {
            int w = write_header(file, PAGE_VALID, 1);
            int ret = write_raw(510, 508, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 1) {
            int w = write_header(file, PAGE_VALID, 2);
            int ret = write_raw(17, 1018, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            int checksum = calc_checksum(123, 2069);
            w += write_hw(checksum, file);
            w += fill_empty(492, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * 100 (1018) -> 32 (1022) -> 1 (29) + aligned(0)
 * length = 2069 -> aligned +1
 * wrong checksum
 */
int gen_verify_30(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 100) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_id_length(123, 2069, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            ret = write_raw(508, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 32) {
            int w = write_header(file, PAGE_VALID, 1);
            int ret = write_raw(510, 508, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 1) {
            int w = write_header(file, PAGE_VALID, 2);
            int ret = write_raw(17, 1018, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            w += write_hw(555, file);
            w += fill_empty(492, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


/*
 * Page 24 VALID num=0, id=123, id=456, id=1, id=12
 * Page 12 VALID num=1, id=12777
 * Page 14 VALID num=2, id=888
 * Page 1  VALID num=3, id=888 (continue)
 */
int gen_verify_31(const char *filename) {
    FILE *file = fopen(filename, "w+");
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        if (page == 24) {
            int w = write_header(file, PAGE_VALID, 0);
            int ret = write_data(123, 5, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            ret = write_data(456, 3, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            ret = write_data(1, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            ret = write_data(12, 2, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            w += fill_empty(492, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 12) {
            int w = write_header(file, PAGE_VALID, 1);
            int ret = write_data(12777, 1014, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 14) {
            int w = write_header(file, PAGE_VALID, 2);
            int ret = write_id_length(888, 1019, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            ret = write_raw(508, 0, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else if (page == 1) {
            int w = write_header(file, PAGE_VALID, 3);
            int ret = write_raw(2, 508, file);
            COND_ERROR_RET(ret > 0, -ret);
            w += ret;
            uint16_t checksum = calc_checksum(888, 1019);
            w += fwrite(&checksum, sizeof(uint16_t), 1, file);
            w += fill_empty(507, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        } else {
            int w = fill_empty(512, file);
            COND_ERROR_RET(w == 512, ERROR_WRT);
        }
    }
    fclose(file);
    return OK;
}


int gen_clear(const char *filename) {
    FILE *file = fopen(filename, "w+");
    COND_ERROR_RET(file != NULL, ERROR_SYSTEM);
    int ret = OK;
    int total_written = 0;
    int page = 0;
    for (; page < FLASH_PAGE_COUNT; page++) {
        int hw_written = 0;
        COND_ERROR_RET((ret=fill_empty(512, file)) > 0,
                ERROR_WRT);
        hw_written += ret;
        COND_ERROR_RET(hw_written == 512, ERROR_WRT);
        total_written += hw_written;
    }
    COND_ERROR_RET(total_written == 512*128,
            ERROR_WRT);

    COND_ERROR_RET((ret=fclose(file)) == 0, ERROR_SYSTEM);
    return OK;
}
