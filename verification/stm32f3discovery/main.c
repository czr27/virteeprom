/*
 *  main.c
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

#include "ch.h"
#include "hal.h"
#include "eeprom.h"
#include "errnum.h"
#include "errmsg.h"
#include "examine.h"
#include "shell.h"
#include "chprintf.h"
#include "chtm.h"
#include "chsys.h"

veeprom_status *VSTATUS = NULL;

char TEST_VECTOR[] = { '3', '1', '4', '1', '5', '9', '2', '6', '5', '3', '5',
		'8', '9', '7', '9', '3', '2', '3', '8', '4', '6', '2', '6', '4', '3',
		'3', '8', '3', '2', '7', '9', '5', '0', '2', '8', '8', '4', '1', '9',
		'7', '1', '6', '9', '3', '9', '9', '3', '7', '5', '1', '0', '5', '8',
		'2', '0', '9', '7', '4', '9', '4', '4', '5', '9', '2', '3', '0', '7',
		'8', '1', '6', '4', '0', '6', '2', '8', '6', '2', '0', '8', '9', '9',
		'8', '6', '2', '8', '0', '3', '4', '8', '2', '5', '3', '4', '2', '1',
		'1', '7', '0', '6', '7', '9', '8', '2', '1', '4', '8', '0', '8', '6',
		'5', '1', '3', '2', '8', '2', '3', '0', '6', '6', '4', '7', '0', '9',
		'3', '8', '4', '4', '6', '0', '9', '5', '5', '0', '5', '8', '2', '2',
		'3', '1', '7', '2', '5', '3', '5', '9', '4', '0', '8', '1', '2', '8',
		'4', '8', '1', '1', '1', '7', '4', '5', '0', '2', '8', '4', '1', '0',
		'2', '7', '0', '1', '9', '3', '8', '5', '2', '1', '1', '0', '5', '5',
		'5', '9', '6', '4', '4', '6', '2', '2', '9', '4', '8', '9', '5', '4',
		'9', '3', '0', '3', '8', '1', '9' };

MEMORYPOOL_DECL(char_pool, sizeof(struct vpage_status), chCoreAllocI);

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION( Thread1, arg) {

	(void) arg;

	chRegSetThreadName("blinker");
	while (true) {
		palSetPad(GPIOE, GPIOE_LED3_RED);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED3_RED);
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED7_GREEN);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED7_GREEN);
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED10_RED);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED10_RED);
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED6_GREEN);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED6_GREEN);
		chThdSleepMilliseconds(125);
	}
}

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
	size_t n, size;

	(void) argv;
	if (argc > 0) {
		chprintf(chp, "Usage: mem\r\n");
		return;
	}
	n = chHeapStatus(NULL, &size);
	chprintf(chp, "core free memory : %u bytes\r\n", chCoreGetStatusX());
	chprintf(chp, "heap fragments   : %u\r\n", n);
	chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_initflash(BaseSequentialStream *chp, int argc, char *argv[]) {
	int ret = OK;

	if (VSTATUS != NULL) {
		chprintf(chp, "release old veeprom status\r\n");
		if ((ret = veeprom_status_release(VSTATUS)) != OK) {
			ERROR(emsg(ret));
			return;
		}
	}

	VSTATUS = veeprom_create_status();
	if (VSTATUS == NULL) {
		ERROR("virteeprom creation failed\r\n");
		return;
	}

	extern uint8_t _veeprom_start;

	time_measurement_t mem_tmu;
	chTMObjectInit(&mem_tmu);
	chTMStartMeasurementX(&mem_tmu);
	ret = veeprom_status_init(VSTATUS, (uint16_t*) &_veeprom_start);
	COND_ERROR(ret == OK, emsg(ret));
	if (ret != OK) {
		chTMStopMeasurementX(&mem_tmu);
		veeprom_status_release(VSTATUS);
		return;
	}

	ret = veeprom_init(VSTATUS);
	chTMStopMeasurementX(&mem_tmu);

	COND_ERROR(ret == OK, emsg(ret));
	chprintf(chp, "successfully initialized during %d mks\r\n",
			RTC2US(STM32_SYSCLK, mem_tmu.last));
}

static void cmd_wipeflash(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void) argc;
	(void*) argv;
	COND_ERROR(VSTATUS != NULL, emsg(ERROR_NULLPTR));
	time_measurement_t mem_tmu;
	chTMObjectInit(&mem_tmu);
	chTMStartMeasurementX(&mem_tmu);
	int ret = veeprom_clean(VSTATUS);
	COND_ERROR(ret == OK, emsg(ret));
	chprintf(chp, "successfully wiped during %d mks\r\n",
			RTC2US(STM32_SYSCLK, mem_tmu.last));
	chTMStopMeasurementX(&mem_tmu);
	cmd_initflash(chp, argc, argv);
}

static void cmd_writeflash(BaseSequentialStream *chp, int argc, char *argv[]) {
	int ret = OK;
	uint16_t i = 1;
	time_measurement_t mem_tmu;
	chTMObjectInit(&mem_tmu);
	chTMStartMeasurementX(&mem_tmu);
	for (; i < 101; i++) {
		COND_ERROR(
				(ret = veeprom_write(i, (uint8_t*) &TEST_VECTOR[i], 1, VSTATUS))
						== OK, emsg(ret));
	}
	chTMStopMeasurementX(&mem_tmu);
	chprintf(chp, "successfully written 100 char values during %d mks\r\n",
			RTC2US(STM32_SYSCLK, mem_tmu.last));
}

static void cmd_readflash(BaseSequentialStream *chp, int argc, char *argv[]) {
	uint16_t i = 1;
	time_measurement_t mem_tmu;
	chTMObjectInit(&mem_tmu);
	chTMStartMeasurementX(&mem_tmu);
	for (; i < 101; i++) {
		vdata *v = veeprom_read(i, VSTATUS);
		if (v == NULL) {
			ERROR(emsg(ERROR_NULLPTR));
			continue;
		}
		uint16_t *p = v->p;
		COND_ERROR(*p == i, "id error");
		p++;
		COND_ERROR(*p == 1, "length error");
		p++;
		COND_ERROR(*(char*) p == TEST_VECTOR[i], "value error");
		p++;
		uint16_t echecksum = i ^ 1 ^ TEST_VECTOR[i];
		COND_ERROR(*p == echecksum, "checksum error");
	}
	chTMStopMeasurementX(&mem_tmu);
	chprintf(chp, "successfully read 100 values during %d mks\r\n",
			RTC2US(STM32_SYSCLK, mem_tmu.last));
}

static const ShellCommand commands[] = {
    { "mem", cmd_mem },
    { "initflash", cmd_initflash },
    { "wipeflash", cmd_wipeflash },
    { "writeflash", cmd_writeflash },
    { "readflash", cmd_readflash },
    { NULL, NULL } };

const ShellConfig shell_cfg = { (BaseSequentialStream *) &SD1, commands };

static THD_WORKING_AREA(waThread2, 1024);
static THD_FUNCTION( Thread2, arg) {

	(void) arg;

	chRegSetThreadName("blinker");
	while (true) {
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED5_ORANGE);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED5_ORANGE);
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED9_BLUE);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED9_BLUE);
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED8_ORANGE);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED8_ORANGE);
		chThdSleepMilliseconds(125);
		palSetPad(GPIOE, GPIOE_LED4_BLUE);
		chThdSleepMilliseconds(125);
		palClearPad(GPIOE, GPIOE_LED4_BLUE);
	}
}

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

void FLASH_Unlock();

int main(void) {
	static thread_t *shelltp = NULL;

	halInit();
	chSysInit();
	FLASH_Unlock();

	sdStart(&SD1, NULL);
	palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(7));
	palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(7));

	shellInit();

	chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO + 1, Thread1,
			NULL);
	chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO + 1, Thread2,
			NULL);

	shelltp = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);

	while (true) {
		chThdSleepMilliseconds(1000);
	}
}
