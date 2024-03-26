#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "instruction_set.h"

struct memory {
	unsigned int size;
	uint8_t *memory;
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
_Static_assert(sizeof(struct sreg) == 1, "sreg takes up 1 byte in data memory");

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
	struct memory *program;
	struct memory *data;
	uint8_t *reg_file; /* General purpose registers R0-R31 */
	uint8_t *io_reg; /* I/O registers */
	struct sreg *sreg; /* Status register */
	uint16_t *sp; /* Pointer to stack pointer */
	uint16_t pc; /* Program counter */
	struct instruction current_inst;
	unsigned cycles_to_finish;
	uintmax_t cycle_count;
};

/* Run one CPU cycle. */
void cpu_cycle(struct cpu *cpu);

#endif
