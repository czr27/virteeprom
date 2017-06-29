/*
 *  eeprom.c
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


#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "eeprom.h"
#include "wrappers.h"
#include "errdef.h"
#include "mem.h"

static int VEEPROM_PAGE_COUNT;

static veeprom_status_t m_status;
static veeprom_cursor_t m_cursor;

static flash_chunk_t* m_veeprom_ids[FLASH_PAGE_COUNT];
static int            m_veeprom_ids_size;

static flash_chunk_t* m_veeprom_pages[FLASH_PAGE_COUNT];
static int            m_veeprom_pages_size;



#define VEEPROM_SET_PHYSNUM(p, physnum) \
    unsigned long delta = (unsigned long)((void*)(p)) - (unsigned long)((void*)(m_status.flash_start));\
    physnum = delta / FLASH_PAGE_SIZE;

#define VEEPROM_IS_INIT() (m_status.flags & VEEPROM_INITIALIZED)

#define VEEPROM_PAGE_STATUS(page) (*page)


VEEPROM_MODULE(int)
veeprom_init_cursor() {
    memset(&m_cursor, 0, sizeof(veeprom_cursor_t));
    m_cursor.index = -1;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_iterate_cursor() {
    if ((m_cursor.p_current - m_cursor.p_start_page + 1) >= FLASH_PAGE_CHUNKS) {
        THROW (m_cursor.index + 1 < m_veeprom_pages_size, ERROR_DCNSTY);
        THROW (m_veeprom_pages[m_cursor.index + 1] != NULL, ERROR_NULLPTR);
        m_cursor.index++;
        flash_chunk_t id = *(m_cursor.p_start_page + VEEPROM_HEADER_CHUNKS);
        m_cursor.p_start_page = m_veeprom_pages[m_cursor.index] - 1;
        THROW (VEEPROM_PAGE_STATUS(m_cursor.p_start_page) == PAGE_RECEIVING, ERROR_DCNSTY);
        m_cursor.p_current = m_veeprom_pages[m_cursor.index];
        THROW (*m_cursor.p_current > 0 && *m_cursor.p_current < VEEPROM_MAX_VIRTNUM, VEEPROM_ERROR_VIRTNUM);
        m_cursor.p_current++;
        RIFER (flash_write_chunk(id, m_cursor.p_current));
    }
    m_cursor.p_current++;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_calculate_pages(flash_chunk_t length) {
    /* 2 additional chunks: length and checksum */
    int chunks = TO_CHUNKS(length) + 2;
    /* Each page has header, page with data has id.
     * id is not part of a page header. The first page has additional length field,
     * the last page has checksum. */
    int free_page_space = FLASH_PAGE_CHUNKS - VEEPROM_HEADER_CHUNKS - 1;
    int pages = chunks / free_page_space;
    if (chunks % free_page_space)
        pages++;
    return pages;
}


VEEPROM_MODULE(int)
veeprom_heapify(flash_chunk_t* a[FLASH_PAGE_COUNT], int size, int i) {
    while (i < size) {
        int l = 2*i + 1;
        int r = 2*i + 2;

        int largest = i;

        if (l < size)
            if (*a[largest] < *a[l])
                largest = l;

        if (r < size)
            if (*a[largest] < *a[r])
                largest = r;

        if (i == largest)
            break;

        flash_chunk_t *tmp = a[i];
        a[i] = a[largest];
        a[largest] = tmp;
        i = largest;
    }
    return OK;
}


VEEPROM_MODULE(int)
veeprom_buildheap(flash_chunk_t* a[FLASH_PAGE_COUNT], int size) {
    for (int j = size/2; j >= 0; j--)
        veeprom_heapify(a, size, j);

    return OK;
}


VEEPROM_MODULE(int)
veeprom_heapsort(flash_chunk_t* a[FLASH_PAGE_COUNT], int size) {
    THROW (a != NULL, ERROR_NULLPTR);
    THROW (size <= FLASH_PAGE_COUNT, ERROR_OBNDS);

    veeprom_buildheap(a, size);
    for (int i = size - 1; i > 0; i--) {
        flash_chunk_t *tmp = a[0];
        a[0] = a[i];
        a[i] = tmp;
        veeprom_heapify(a, --size, 0);
    }
    return OK;
}


VEEPROM_MODULE(int)
veeprom_sortedinsert(flash_chunk_t* a[FLASH_PAGE_COUNT], int *size, flash_chunk_t *p) {
    THROW (size != NULL && a != NULL && p != NULL, ERROR_NULLPTR);
    THROW (*size < FLASH_PAGE_COUNT, VEEPROM_ERROR_NOMEM);

    int i = *size - 1;
    for (; i >= 0; i--)
        if (*a[i] <= *p)
            break;

    (*size)++;
    for (int j = *size - 1; j > i+1; j--)
        a[j] = a[j-1];

    a[i+1] = p;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_vectorpush(flash_chunk_t* a[FLASH_PAGE_COUNT], int *size, flash_chunk_t *p) {
    THROW (size != NULL && a != NULL && p != NULL, ERROR_NULLPTR);
    THROW (*size < FLASH_PAGE_COUNT, VEEPROM_ERROR_NOMEM);

    (*size)++;
    a[*size-1] = p;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_sortedrm(flash_chunk_t* a[FLASH_PAGE_COUNT], int *size, int index) {
    THROW (size != NULL && a != NULL, ERROR_NULLPTR);
    THROW (*size <= FLASH_PAGE_COUNT && *size > 0, ERROR_PARAM);
    THROW (index > -1 && index < FLASH_PAGE_COUNT, ERROR_PARAM);

    for (int i = index+1; i < *size; i++) {
        a[i-1] = a[i];
    }
    a[*size - 1] = 0;
    (*size)--;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_binsearch(flash_chunk_t* a[FLASH_PAGE_COUNT], int size, flash_chunk_t val) {
    int l = 0;
    int r = size - 1;
    while (l <= r) {
        int m = (l + r) >> 1;
        if (*a[m] == val)
            return m;
        if (*a[m] < val)
            l = m + 1;
        else
            r = m - 1;
    }
    return -1;
}


VEEPROM_MODULE(int)
veeprom_set_next_alloc() {
    for (int16_t i = m_status.next_alloc + 1; i < VEEPROM_PAGE_COUNT; i++)
    {
        if (m_status.busy_map[i] != -1) {
            m_status.next_alloc = i;
            return OK;
        }
    }

    for (int i = 0; i < m_status.next_alloc; i++) {
        if (m_status.busy_map[i] != -1) {
            m_status.next_alloc = i;
            return OK;
        }
    }

    m_status.next_alloc = -1;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_rm_dereg_page(flash_chunk_t *page, int index) {
    THROW (page != NULL, ERROR_NULLPTR);
    flash_chunk_t physnum = 0;
    VEEPROM_SET_PHYSNUM(page, physnum);
    VEEPROM_LOGDEBUG("rm page physnum=%" VEEPROM_FLASH_CHUNK_FMT
            " virtnum=%" VEEPROM_FLASH_CHUNK_FMT, physnum, *(page+1));

    m_status.busy_map[physnum] = physnum;
    if (m_status.next_alloc == -1)
        m_status.next_alloc = physnum;
    m_status.busy_pages--;

    RIFER (veeprom_sortedrm(m_veeprom_pages, &m_veeprom_pages_size, index));
    return flash_erase_page(page);
}


VEEPROM_MODULE(int)
veeprom_rm_data_dereg_pages(flash_chunk_t *p) {
    THROW (p != NULL, ERROR_NULLPTR);

    THROW (*(p+1) < VEEPROM_MAX_LENGTH, VEEPROM_ERROR_LENGTH);

    int pages = veeprom_calculate_pages(*(p+1));

    int index = veeprom_binsearch(m_veeprom_pages, m_veeprom_pages_size, *(p-1));

    while (pages-- > 0) {
        THROW (index != -1, ERROR_DCNSTY);
        THROW (m_veeprom_pages[index] != NULL, ERROR_NULLPTR);

        flash_chunk_t *page = m_veeprom_pages[index] - 1;
        THROW (VEEPROM_PAGE_STATUS(page) == PAGE_VALID, ERROR_DCNSTY);

        RIFER (veeprom_rm_dereg_page(page, index));
    }

    return OK;
}


VEEPROM_MODULE(int)
veeprom_reg_id_rm_prev(flash_chunk_t *addr) {
    int index = veeprom_binsearch(m_veeprom_ids, m_veeprom_ids_size, *addr);
    if (index != -1) {
        flash_chunk_t *addr_prev = m_veeprom_ids[index];
        THROW (addr_prev != NULL, ERROR_NULLPTR);
        THROW (*addr_prev == *(flash_chunk_t*)addr, ERROR_DCNSTY);
        RIFER (veeprom_rm_data_dereg_pages(addr_prev));
        m_veeprom_ids[index] = addr;
    } else {
        return veeprom_sortedinsert(m_veeprom_ids, &m_veeprom_ids_size, addr);
    }
    return OK;
}


VEEPROM_MODULE(int)
veeprom_resolve_collision() {
    if (m_veeprom_ids_size < 2)
        return OK;

    THROW (m_veeprom_ids[0] != NULL, ERROR_NULLPTR);
    flash_chunk_t prev_id = *m_veeprom_ids[0];

    for (int i=1; i < m_veeprom_ids_size; i++) {
        THROW (m_veeprom_ids[i] != NULL, ERROR_NULLPTR);

        flash_chunk_t id = *m_veeprom_ids[i];

        if (prev_id != id) {
            prev_id = id;
            continue;
        }

        /* The case of collision */
        flash_chunk_t prev_virtnum = *(m_veeprom_ids[i-1] - 1);
        flash_chunk_t virtnum      = *(m_veeprom_ids[i] - 1);

        THROW (prev_virtnum != virtnum, ERROR_DCNSTY);

        /* The most latest data with the same id have larger virtnum */
        if (virtnum > prev_virtnum) {
            /* There is only one case of collision: when data were added,
             * corresponding pages were set to VALID, old data were not removed */
            RIFER (veeprom_rm_data_dereg_pages(m_veeprom_ids[i-1]));
            RIFER (veeprom_sortedrm(m_veeprom_ids, &m_veeprom_ids_size, i-1));
            return OK;
        } else {
            RIFER (veeprom_rm_data_dereg_pages(m_veeprom_ids[i]));
            RIFER (veeprom_sortedrm(m_veeprom_ids, &m_veeprom_ids_size, i-1));
            return OK;
        }
    }

    return OK;
}


VEEPROM_MODULE(int)
veeprom_init_data() {

    if (m_status.busy_pages < 0) {
        THROW (0, ERROR_DCNSTY);
    } else if (m_status.busy_pages == 0) {
        return OK;
    }

    int index = -1;
    flash_chunk_t *p_id = NULL;
    int pages = 0;

    for (int i=0; i < m_veeprom_pages_size; i++) {
        THROW (m_veeprom_pages[i] != NULL, ERROR_NULLPTR);
        flash_chunk_t *p = m_veeprom_pages[i] + 1;

        THROW (*p > 0 && *p < VEEPROM_MAX_ID, VEEPROM_ERROR_ID);

        if (p_id == NULL) {
            /*TODO: simplify code with conception of m_cursor/iterator */
            index = i;
            p_id = p;
            THROW (*(p+1) < VEEPROM_MAX_LENGTH, VEEPROM_ERROR_LENGTH);
            pages = veeprom_calculate_pages(*(p+1));
            pages--;
            continue;
        }

        if (*p_id != *p) {
            if (pages > 0) {
                /* The case when data were erasing but not completely */

                VEEPROM_LOGDEBUG("previos erasing failed id=%" VEEPROM_FLASH_CHUNK_FMT, *p_id);

                THROW (index > 0 && index < FLASH_PAGE_COUNT &&
                        index < i && m_veeprom_pages[index] != NULL, ERROR_DCNSTY);

                for (int j = 0; j < i - index + 1; j++) {
                    RIFER (veeprom_rm_dereg_page(p_id - VEEPROM_HEADER_CHUNKS, index));
                    THROW (m_veeprom_pages[index] != NULL, ERROR_DCNSTY);
                    p_id = m_veeprom_pages[index] + 1;
                }

                THROW (*(p+1) < VEEPROM_MAX_LENGTH, VEEPROM_ERROR_LENGTH);
                pages = veeprom_calculate_pages(*(p+1));
                pages--;
                p_id = p;
                index = i;
                continue;

            } else if (pages == 0) {
                RIFER (veeprom_vectorpush(m_veeprom_ids, &m_veeprom_ids_size, p_id));

                THROW (*(p+1) < VEEPROM_MAX_LENGTH, VEEPROM_ERROR_LENGTH);
                pages = veeprom_calculate_pages(*(p+1));
                pages--;
                p_id = p;
                index = i;

            } else {
                /* pages cannot be negative */
                THROW (0, ERROR_DCNSTY);
            }

        } else {
            THROW (pages > 0, ERROR_DCNSTY);
            pages--;
        }
    }

    THROW (pages >= 0, ERROR_DCNSTY);

    if (p_id != NULL) {
        THROW (index != -1, ERROR_DCNSTY);

        if (pages > 0) {
            /* Remove up to the last node. The case of incompleted writing.  */
            for (int j = 0; j < m_veeprom_pages_size - index; j--) {
                RIFER (veeprom_rm_dereg_page(p_id - VEEPROM_HEADER_CHUNKS, index));
                THROW (m_veeprom_pages[index] != NULL, ERROR_NULLPTR);
                p_id = m_veeprom_pages[index] + 1;
            }

        } else {
            RIFER (veeprom_vectorpush(m_veeprom_ids, &m_veeprom_ids_size, p_id));
        }

    } else {
        THROW (pages == 0 && index == -1, ERROR_DCNSTY);
    }

    int ret = veeprom_heapsort(m_veeprom_ids, m_veeprom_ids_size);
    THROW (ret == OK, ret);
    return veeprom_resolve_collision();
}


VEEPROM_MODULE(int)
veeprom_order_pages() {
    flash_chunk_t *p = m_status.flash_start;
    flash_chunk_t *flash_end = p + FLASH_PAGE_SIZE * VEEPROM_PAGE_COUNT;
    int ret = 0;
    for (int physnum = 0; physnum < VEEPROM_PAGE_COUNT && p < flash_end; physnum++, p += FLASH_PAGE_CHUNKS) {
        flash_chunk_t s = VEEPROM_PAGE_STATUS(p);
        switch (s) {
        case PAGE_VALID:
            {
                THROW (*(p+1) > 0 && *(p+1) < VEEPROM_MAX_VIRTNUM, VEEPROM_ERROR_VIRTNUM);
                RIFER (veeprom_vectorpush(m_veeprom_pages, &m_veeprom_pages_size, p+1));
                m_status.next_alloc = physnum;
                m_status.busy_pages++;
                m_status.busy_map[physnum] = VEEPROM_BUSY_PAGE_FLAG;
            }
            break;
        case PAGE_RECEIVING:
            THROW ((ret = flash_erase_page(p)) == OK, ret);
            m_status.busy_map[physnum] = physnum;
            m_status.next_alloc = physnum;
            break;
        case PAGE_ERASED:
            m_status.busy_map[physnum] = physnum;
            break;
        default:
            THROW (0, ERROR_UNKNOWNSTATUS);
            break;
        }
    }
    veeprom_set_next_alloc();
    RIFER (veeprom_heapsort(m_veeprom_pages, m_veeprom_pages_size));
    return OK;
}


VEEPROM_MODULE(int)
veeprom_check_order() {
    if (m_veeprom_pages_size < 2) {
        for (int i = 0; i < m_veeprom_pages_size; i++)
            THROW (m_veeprom_pages[i] != NULL, ERROR_NULLPTR);
        return OK;
    }

    for (int i=1; i < m_veeprom_pages_size; i++) {
        THROW (m_veeprom_pages[i-1] != NULL && m_veeprom_pages[i] != NULL, ERROR_NULLPTR);
        THROW (*m_veeprom_pages[i-1] < *m_veeprom_pages[i], VEEPROM_ERROR_VIRTNUM);
    }
    return OK;
}


VEEPROM_MODULE(int)
veeprom_write_chunk(flash_chunk_t data) {
    RIFER (veeprom_iterate_cursor());
    RIFER (flash_write_chunk(data, m_cursor.p_current));
    m_cursor.checksum ^= data;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_set_receiving(int physnum) {
    THROW (m_status.busy_pages + 1 <= VEEPROM_PAGE_COUNT, VEEPROM_ERROR_NOMEM);

    flash_chunk_t *p = m_status.flash_start + physnum * FLASH_PAGE_CHUNKS;
    int ret = flash_write_chunk(PAGE_RECEIVING, p);
    THROW (ret == OK, ret);

    flash_chunk_t virtnum = 1;
    if (m_veeprom_pages_size > 0) {
        flash_chunk_t *virtnum_last = m_veeprom_pages[m_veeprom_pages_size - 1];
        THROW (virtnum_last != NULL, ERROR_NULLPTR);
        THROW (*virtnum_last + 1 < VEEPROM_MAX_VIRTNUM, ERROR_FLASH_EXPIRED);
        virtnum = *virtnum_last + 1;
    }

    RIFER (flash_write_chunk(virtnum, p + 1));

    m_status.busy_map[physnum] = -1;
    /* this insertion doesn't damage sorted order of virtnums */
    RIFER (veeprom_vectorpush(m_veeprom_pages, &m_veeprom_pages_size, p+1));

    m_status.busy_pages++;

    return OK;
}



VEEPROM_MODULE(int)
veeprom_alloc_pages_set_cursor(int count) {
    int free_pages = VEEPROM_PAGE_COUNT - m_status.busy_pages;
    THROW (count <= free_pages, VEEPROM_ERROR_NOMEM);

    /*
     * index start page for writing data
     */
    int index = -1;
    for (int pageno = 0; pageno < count; pageno++) {
        int physnum = m_status.next_alloc;
        THROW (physnum != -1, VEEPROM_ERROR_NOMEM);

        RIFER (veeprom_set_receiving(physnum));

        if (index == -1)
            index = m_veeprom_pages_size - 1;

        RIFER (veeprom_set_next_alloc());
    }

    THROW (index != -1, ERROR_DCNSTY);
    THROW (m_veeprom_pages[index] != NULL, ERROR_NULLPTR);

    m_cursor.p_start_page = m_veeprom_pages[index] - 1;
    m_cursor.p_current = m_cursor.p_start_page + 1;
    m_cursor.index = index;
    return OK;
}


VEEPROM_MODULE(int)
veeprom_receiving_to_valid() {
    if (m_veeprom_pages_size == 0)
        return OK;

    int i = m_veeprom_pages_size - 1;
    for (; i >= 0; i--) {
        THROW (m_veeprom_pages[i] != NULL, ERROR_NULLPTR);
        flash_chunk_t *p = m_veeprom_pages[i] - 1;
        switch (VEEPROM_PAGE_STATUS(p)) {
            case PAGE_RECEIVING:
                RIFER (flash_write_chunk(PAGE_VALID, p));
                continue;
            case PAGE_VALID:
                return OK;
            default:
                THROW (0, ERROR_DCNSTY);
        }
    }

    if (i == -1)
        return OK;

    VEEPROM_LOGDEBUG("receiving to valid inconsistency");
    return ERROR_DCNSTY;
}


VEEPROM_MODULE(int)
veeprom_erase_receiving() {
    if (m_veeprom_pages_size == 0)
        return OK;

    while (m_veeprom_pages_size > 0 &&
          *(m_veeprom_pages[m_veeprom_pages_size - 1] - 1) == PAGE_RECEIVING)
    {
        int index = m_veeprom_pages_size - 1;
        RIFER (veeprom_rm_dereg_page(m_veeprom_pages[index], index));
    }
    return OK;
}


VEEPROM_MODULE(int)
veeprom_write_data(uint8_t *data, flash_chunk_t length) {
    int s = 0;
    flash_chunk_t acc = 0;
    int ret = OK;

    for (unsigned int i = 0; i < length; i++) {
        flash_chunk_t c = *(data + i);
        if (s == sizeof(flash_chunk_t)) {
            THROW ((ret=veeprom_write_chunk(acc)) == OK, ret);
            acc = 0;
            s = 0;
        }
        acc |= (c << (s*8));
        s++;
    }

    if (s > 0) {
        THROW ((ret=veeprom_write_chunk(acc)) == OK, ret);
    }

    return OK;
}




/* API functions */


int veeprom_init(flash_chunk_t *flash_start) {
    m_status.flags = VEEPROM_NOTINITIALIZED;
    /* Set amount of pages that may be used for Virtual EEPROM.
     * Code and data are located on the flash.
     */
#ifdef CHIBIOS_ON
    extern uint32_t _veeprom_start;
    extern uint32_t _veeprom_end;
    VEEPROM_LOGDEBUG("veeprom_start=%d", &_veeprom_start);
    VEEPROM_LOGDEBUG("veeprom_end=%d", &_veeprom_end);
    /* FIXIT: in the case of pages of different sizes this is not true */
    VEEPROM_PAGE_COUNT = ((uint32_t)&_veeprom_end - (uint32_t)&_veeprom_start) / FLASH_PAGE_SIZE;
    VEEPROM_LOGDEBUG("veeprom_page_count=%d", VEEPROM_PAGE_COUNT);
#else
    VEEPROM_PAGE_COUNT = FLASH_PAGE_COUNT;
#endif
    m_status.flash_start = flash_start;

    memset(m_status.busy_map, 0xFF, (sizeof(m_status.busy_map)));

    memset(m_veeprom_ids, 0, sizeof(m_veeprom_ids));
    m_veeprom_ids_size = 0;

    memset(m_veeprom_pages, 0, sizeof(m_veeprom_pages));
    m_veeprom_pages_size = 0;

    m_status.busy_pages = 0;
    m_status.next_alloc = -1;

    RIFER (veeprom_order_pages());
    RIFER (veeprom_check_order());
    RIFER (veeprom_init_data());

    m_status.flags |= VEEPROM_INITIALIZED;
    return OK;
}


int veeprom_deinit() {
    memset(m_veeprom_ids, 0, sizeof(m_veeprom_ids));
    m_veeprom_ids_size = 0;
    m_status.flags = VEEPROM_NOTINITIALIZED;

    return OK;
}


#ifdef CHIBIOS_ON
int veeprom_clean() {
    int ret = OK;
    extern uint32_t _veeprom_start;
    extern uint32_t _veeprom_end;
    flash_chunk_t *page = (flash_chunk_t*)&_veeprom_start;
    for (; page < (flash_chunk_t*)&_veeprom_end; page += FLASH_PAGE_CHUNKS) {
        THROW ((ret=flash_erase_page(page)) == OK, ret);
    }
    return OK;
}
#endif


int veeprom_write(flash_chunk_t id, uint8_t *data, flash_chunk_t length) {
    THROW (VEEPROM_IS_INIT(), VEEPROM_ERROR_INIT);

    THROW (data != NULL, ERROR_NULLPTR);
    THROW (id > 0 && id < VEEPROM_MAX_ID, VEEPROM_ERROR_ID);
    THROW (length >= 0 && length < VEEPROM_MAX_LENGTH, VEEPROM_ERROR_LENGTH);

    veeprom_init_cursor();
    RIFER (veeprom_alloc_pages_set_cursor(veeprom_calculate_pages(length)));
    RIFER (veeprom_write_chunk(id));
    flash_chunk_t *p_id = m_cursor.p_current;

    int ret = OK;
    THROW ((ret=veeprom_write_chunk(length)) == OK, ret);
    THROW ((ret=veeprom_write_data(data, length)) == OK, ret);
    THROW ((ret=veeprom_write_chunk(m_cursor.checksum)) == OK, ret);
    THROW (m_cursor.checksum == 0, ERROR_DCNSTY);

    if (ret != OK) {
        RIFER (veeprom_erase_receiving());
        THROW (0, ret);
    }

    RIFER (veeprom_receiving_to_valid());
    RIFER (veeprom_reg_id_rm_prev(p_id));

    return OK;
}


int veeprom_read(veeprom_read_t *read_buf) {
    THROW (VEEPROM_IS_INIT(), VEEPROM_ERROR_INIT);

    THROW (read_buf != NULL && read_buf->buf != NULL, ERROR_NULLPTR);

    int index = veeprom_binsearch(m_veeprom_ids, m_veeprom_ids_size, read_buf->id);
    THROW (index != -1, VEEPROM_ERROR_ID_NOTFOUND,
        VEEPROM_LOGDEBUG("not found id=%" VEEPROM_FLASH_CHUNK_FMT, read_buf->id));

    flash_chunk_t *p = m_veeprom_ids[index];
    THROW (p != NULL && p+1 != NULL, ERROR_NULLPTR);
    THROW (*p == read_buf->id, -ERROR_DCNSTY);

    read_buf->length = *(p+1);
    int length = TO_CHUNKS(*(p+1)) * sizeof(flash_chunk_t);
    THROW (length <= read_buf->buf_size, VEEPROM_ERROR_BUFSIZE);

    uint8_t *p_buf = read_buf->buf;
    int pages = veeprom_calculate_pages(length);

    if (pages > 0) {
        index = veeprom_binsearch(m_veeprom_pages, m_veeprom_pages_size, *(p - 1));
        THROW (index != -1, ERROR_DCNSTY);
    }
    p += 2;

    while (pages-- > 0) {

        int page_length = FLASH_PAGE_SIZE - VEEPROM_HEADER_SIZE - 2*sizeof(flash_chunk_t);

        if (page_length > length)
            page_length = length;

        memcpy(p_buf, p, page_length);
        p_buf += page_length;

        length -= page_length;

        THROW (length >= 0, ERROR_DCNSTY);

        if (length == 0) {
            p += TO_CHUNKS(page_length);
            read_buf->checksum = p;
            break;
        }

        THROW (pages > 0, ERROR_DCNSTY);
        THROW (index + 1 < m_veeprom_pages_size, ERROR_DCNSTY);
        THROW (m_veeprom_pages[index + 1] != NULL, ERROR_NULLPTR);
        index++;
        p = m_veeprom_pages[index];
        THROW (*p > 0 && *p < VEEPROM_MAX_VIRTNUM, VEEPROM_ERROR_VIRTNUM);
        THROW (*(p+1) == read_buf->id, ERROR_DCNSTY);
        p += 2;
    }

    return OK;
}

flash_chunk_t* veeprom_find(flash_chunk_t id) {
    int index = veeprom_binsearch(m_veeprom_ids, m_veeprom_ids_size, id);
    if (index == -1) {
        return NULL;
    }
    return m_veeprom_ids[index];
}

int veeprom_delete(flash_chunk_t id) {
    THROW (VEEPROM_IS_INIT(), VEEPROM_ERROR_INIT);

    int index = veeprom_binsearch(m_veeprom_ids, m_veeprom_ids_size, id);
    if (index == -1) {
        VEEPROM_LOGDEBUG("not found id=%" VEEPROM_FLASH_CHUNK_FMT, id);
        return OK;
    }

    RIFER (veeprom_rm_data_dereg_pages(m_veeprom_ids[index]));
    RIFER (veeprom_sortedrm(m_veeprom_ids, &m_veeprom_ids_size, index));

    return OK;
}


veeprom_status_t* veeprom_get_status() {
    TRACE (VEEPROM_IS_INIT(), VEEPROM_ERROR_INIT, return NULL;)
    return &m_status;
}


flash_chunk_t** veeprom_get_pages() {
    return m_veeprom_pages;
}


int veeprom_get_pages_size() {
    return m_veeprom_pages_size;
}


flash_chunk_t** veeprom_get_ids() {
    return m_veeprom_ids;
}


int veeprom_get_ids_size() {
    return m_veeprom_ids_size;
}

veeprom_cursor_t* veeprom_get_cursor() {
    return &m_cursor;
}
