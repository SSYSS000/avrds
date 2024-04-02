#include <stdlib.h>
#include <string.h>
#include "atmega328p.h"

static int load_io(void *m, unsigned addr, uint8_t *byte)
{
    struct atmega328p *mcu = m;

    switch (addr) {
    case 0x3d: /* SPL */
        *byte = mcu->cpu.sp;
        break;
    case 0x3e: /* SPH */
        *byte = mcu->cpu.sp >> 8;
        break;
    case 0x3f: /* SREG */
        memcpy(byte, &mcu->cpu.sreg, 1);
        break;
    default:
        if (addr < ATMEGA328P_IO_REGISTER_COUNT) {
            *byte = mcu->io_registers[addr];
        }
        else {
            return -1;
        }
        break;
    }

    return 0;
}

static int store_io(void *m, unsigned addr, uint8_t byte)
{
    struct atmega328p *mcu = m;

    switch (addr) {
    case 0x3d: /* SPL */
        mcu->cpu.sp = mcu->cpu.sp & 0xff00 | byte;
        break;
    case 0x3e: /* SPH */
        mcu->cpu.sp = byte << 8 | mcu->cpu.sp & 0xff;
        break;
    case 0x3f: /* SREG */
        memcpy(&mcu->cpu.sreg, &byte, 1);
        break;
    default:
        if (addr < ATMEGA328P_IO_REGISTER_COUNT) {
            mcu->io_registers[addr] = byte;
        }
        else {
            return -1;
        }
        break;
    }

    return 0;
}

static int load_data(void *m, unsigned addr, uint8_t *byte)
{
    struct atmega328p *mcu = m;

    if (addr <= 0x1f) {
        /* General Purpose Working Register */
        *byte = mcu->gpwr[addr];
    }
    else if (addr <= 0x5f) {
        /* I/O register */
        return load_io(mcu, addr - 0x20, byte);
    }
    else if (addr <= 0xff) {
        /* extended io register */
    }
    else if (addr <= 0x8ff) {
        *byte = mcu->sram[addr - 0x100];
    }
    else {
        // out of bounds.
        return -1;
    }

    return 0;
}

static int store_data(void *m, unsigned addr, uint8_t byte)
{
    struct atmega328p *mcu = m;

    if (addr <= 0x1f) {
        /* General Purpose Working Register */
        mcu->gpwr[addr] = byte;
    }
    else if (addr <= 0x5f) {
        /* I/O register */
        return store_io(mcu, addr - 0x20, byte);
    }
    else if (addr <= 0xff) {
        /* extended io register */
    }
    else if (addr <= 0x8ff) {
        mcu->sram[addr - 0x100] = byte;
    }
    else {
        // out of bounds.
        return -1;
    }

    return 0;
}

static int read_flash(void *m, unsigned addr, void *data, unsigned size)
{
    struct atmega328p *mcu = m;
    if (addr + size > ATMEGA328P_FLASH_SIZE) {
        return -1;
    }
    memcpy(data, &mcu->flash[addr], size);
    return 0;
}

static int write_flash(void *m, unsigned addr, const void *data, unsigned size)
{
    struct atmega328p *mcu = m;
    if (addr + size > ATMEGA328P_FLASH_SIZE) {
        return -1;
    }
    memcpy(&mcu->flash[addr], data, size);
    return 0;
}

void atmega328p_init(struct atmega328p *mcu)
{
    memset(mcu, 0, sizeof(*mcu));

    mcu->bus.load = load_data;
    mcu->bus.store = store_data;
    mcu->io_bus.load = load_io;
    mcu->io_bus.store = store_io;
    mcu->flash_bus.read = read_flash;
    mcu->flash_bus.write = write_flash;

    /* CPU */
    mcu->cpu.mcu = mcu;
    mcu->cpu.core = CORE_AVREP;
    mcu->cpu.reg_file = mcu->gpwr;
    mcu->cpu.sp = ATMEGA328P_DATA_MEMORY_SIZE - 1;
    mcu->cpu.bus = &mcu->bus;
    mcu->cpu.io_bus = &mcu->io_bus;
    mcu->cpu.flash_bus = &mcu->flash_bus;
}
