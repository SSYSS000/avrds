#ifndef ATMEGA328P_H
#define ATMEGA328P_H

#include "cpu.h"

#define ATMEGA328P_SRAM_SIZE	2304
#define ATMEGA328P_FLASH_SIZE	(16384 * 2)
#define ATMEGA328P_EEPROM_SIZE	1024

struct atmega328p {
	struct cpu cpu;
	struct memory data;
	struct memory program;
	struct memory eeprom;
};

void atmega328p_init(struct atmega328p *mcu);

#endif
