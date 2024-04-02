#ifndef INSTRUCTION_SET_H
#define INSTRUCTION_SET_H

#include <stdint.h>

enum operation {
    OP_ADC,
    OP_ADD,
    OP_ADIW,
    OP_AND,
    OP_ANDI,
    OP_ASR,
    OP_BCLR,
    OP_BLD,
    OP_BRBC,
    OP_BRBS,
    OP_BRCC,
    OP_BRCS,
    OP_BREAK,
    OP_BREQ,
    OP_BRGE,
    OP_BRHC,
    OP_BRHS,
    OP_BRID,
    OP_BRIE,
    OP_BRLO,
    OP_BRLT,
    OP_BRMI,
    OP_BRNE,
    OP_BRPL,
    OP_BRSH,
    OP_BRTC,
    OP_BRTS,
    OP_BRVC,
    OP_BRVS,
    OP_BSET,
    OP_BST,
    OP_CALL,
    OP_CBI,
    OP_COM,
    OP_CP,
    OP_CPC,
    OP_CPI,
    OP_CPSE,
    OP_DEC,
    OP_DES,
    OP_EICALL,
    OP_EIJMP,
    OP_ELPM_R0,
    OP_ELPM,
    OP_EOR,
    OP_FMUL,
    OP_FMULS,
    OP_FMULSU,
    OP_ICALL,
    OP_IJMP,
    OP_IN,
    OP_INC,
    OP_JMP,
    OP_LAC,
    OP_LAS,
    OP_LAT,
    OP_LDD,
    OP_LD,
    OP_LDI,
    OP_LDS,
    OP_LPM_R0,
    OP_LPM,
    OP_LSR,
    OP_MOV,
    OP_MOVW,
    OP_MUL,
    OP_MULS,
    OP_MULSU,
    OP_NEG,
    OP_NOP,
    OP_OR,
    OP_ORI,
    OP_OUT,
    OP_POP,
    OP_PUSH,
    OP_RCALL,
    OP_RET,
    OP_RETI,
    OP_RJMP,
    OP_ROR,
    OP_SBC,
    OP_SBCI,
    OP_SBI,
    OP_SBIC,
    OP_SBIS,
    OP_SBIW,
    OP_SBR,
    OP_SBRC,
    OP_SBRS,
    OP_SER,
    OP_SLEEP,
    OP_SPM,
    OP_STD,
    OP_ST,
    OP_STS,
    OP_SUB,
    OP_SUBI,
    OP_SWAP,
    OP_WDR,
    OP_XCH,
};

enum base_pointer {
    BP_X,
    BP_Y,
    BP_Z
};

struct instruction {
    enum operation op;

    /*
     * Operands; value is defined only if the operand is used by this
     * instruction.
     */
    uint8_t Rd; /* Destination (and source) register in the Register File */
    uint8_t Rr; /* Source register in the Register File */
    uint8_t A; /* I/O memory address */
    uint8_t K; /* Constant data */
    int32_t k; /* Constant address */
    uint8_t s; /* Bit position (0..7) in the Status Register */
    uint8_t b; /* Bit position (0..7) in the Register File or I/O Register */
    uint8_t q; /* Displacement for direct addressing */
    enum base_pointer bp;
    enum {
        BP_NO_OP,   /* do nothing to base pointer */
        BP_PRE_DEC, /* pre-decrement base pointer */
        BP_POST_INC /* post-increment base pointer */
    } bp_operation;
};

int decode_instruction(const uint16_t *opcode, struct instruction *inst);

/* Return opcode length (1 or 2) in words. */
int opcode_length(uint16_t opcode_beginning);

#endif
