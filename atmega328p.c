#include <stdlib.h>
#include <string.h>
#include "atmega328p.h"

void atmega328p_init(struct atmega328p *mcu)
{
	memset(mcu, 0, sizeof(*mcu));

    mcu->cpu.core = CORE_AVREP;
	mcu->data.size = ATMEGA328P_SRAM_SIZE;
	mcu->data.memory = malloc(mcu->data.size);
	mcu->cpu.data = &mcu->data;

	mcu->program.size = ATMEGA328P_FLASH_SIZE;
	mcu->program.memory = malloc(mcu->program.size);
	mcu->cpu.program = &mcu->program;

	mcu->eeprom.size = ATMEGA328P_EEPROM_SIZE;
	mcu->eeprom.memory = malloc(mcu->eeprom.size);

	mcu->cpu.reg_file = &mcu->data.memory[0];
	mcu->cpu.io_reg = &mcu->data.memory[0x20];
	mcu->cpu.sreg = (struct sreg *)&mcu->data.memory[0x5f];
	mcu->cpu.sp = (uint16_t *)&mcu->data.memory[0x5d];
	*mcu->cpu.sp = ATMEGA328P_SRAM_SIZE - 1;
}
