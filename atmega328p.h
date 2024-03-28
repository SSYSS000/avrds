#ifndef ATMEGA328P_H
#define ATMEGA328P_H

#include "cpu.h"

#define ATMEGA328P_DATA_MEMORY_SIZE     0x900
#define ATMEGA328P_SRAM_SIZE            0x800
#define ATMEGA328P_FLASH_SIZE           0x4000
#define ATMEGA328P_EEPROM_SIZE          0x400

/* The number of General Purpose Working Registers. */
#define ATMEGA328P_GPWR_COUNT           32
#define ATMEGA328P_IO_REGISTER_COUNT    64

struct atmega328p {
    struct cpu cpu;
    struct data_bus bus;
    struct data_bus io_bus;
    struct flash_bus flash_bus;

    uint8_t gpwr[ATMEGA328P_GPWR_COUNT];
    uint8_t io_registers[ATMEGA328P_IO_REGISTER_COUNT];
    uint8_t sram[ATMEGA328P_SRAM_SIZE];
    uint8_t eeprom[ATMEGA328P_EEPROM_SIZE];
    uint8_t flash[ATMEGA328P_FLASH_SIZE];
};

void atmega328p_init(struct atmega328p *mcu);

#endif
