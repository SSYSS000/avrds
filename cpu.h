#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "instruction_set.h"

struct flash_bus {
    /*
     * These functions are called to access parts of flash memory by
     * a CPU. They read data from Flash Memory and write data into Flash
     * Memory respectively.
     *
     * A return value of 0 is expected on success. A negative return value is
     * expected if addr or addr+size is invalid.
     */
    int (*read)(void *mcu, unsigned addr, void *data, unsigned size);
    int (*write)(void *mcu, unsigned addr, const void *data, unsigned size);
};

struct data_bus {
    /*
     * These functions are called to access parts of data memory by
     * a CPU. They load a byte from Data Memory and store a byte into Data
     * Memory respectively.
     *
     * A return value of 0 is expected on success. A negative return value is
     * expected if the address is invalid.
     */
    int (*load)(void *mcu, unsigned addr, uint8_t *byte);
    int (*store)(void *mcu, unsigned addr, uint8_t byte);
};

/* Status REGister */
struct sreg {
    uint8_t C : 1; /* Carry flag */
    uint8_t Z : 1; /* Zero flag */
    uint8_t N : 1; /* Negative flag */
    uint8_t V : 1; /* Two's complement overflow flag */
    uint8_t S : 1; /* Sign flag */
    uint8_t H : 1; /* Half carry flag */
    uint8_t T : 1; /* Transfer bit */
    uint8_t I : 1; /* Global interrupt enable bit */
};

enum cpu_core {
    CORE_AVR,   /* AVR */
    CORE_AVRE,  /* AVRe */
    CORE_AVREP, /* AVRe+ */
    CORE_AVRXM, /* AVRxm*/
    CORE_AVRXT, /* AVRxt */
    CORE_AVRRC  /* AVRrc */
};

struct cpu {
    enum cpu_core core;

    /* Flash bus with access to program memory and other flash memory */
    const struct flash_bus *flash_bus;
    /*
     * There are two data buses. One that can access all data and the I/O
     * data bus that accesses I/O memory only.
     */
    const struct data_bus *bus; /* Data bus with access to all */
    const struct data_bus *io_bus; /* Data bus with access to I/O */
    void *mcu; /* Pointer to the MCU which contains this CPU */

    uint8_t *reg_file; /* General purpose registers */
    struct sreg sreg; /* Status register */
    uint16_t sp; /* Stack pointer value */
    uint16_t pc; /* Program counter */

    struct instruction current_inst; /* Currently executing instruction */
    _Bool is_executing_inst; /* Instruction is being executed */
    unsigned cycle_count_inst_fetch; /* cycle_count when current_inst was set */
    unsigned cycle_count; /* CPU cycles passed */
};

/* Run one CPU cycle. */
void cpu_cycle(struct cpu *cpu);

#endif
