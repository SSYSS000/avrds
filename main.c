#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define FAILED(status) ((status) < 0)

void warn(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

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
	//OP_CBR, (OP_ANDI with K complemented)
	OP_CLC,
	OP_CLH,
	OP_CLI,
	OP_CLN,
	//OP_CLR, (OP_EOR)
	OP_CLS,
	OP_CLT,
	OP_CLV,
	OP_CLZ,
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
	//OP_LSL, (OP_ADD)
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
	//OP_ROL, (OP_ADC)
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
	OP_SEC,
	OP_SEH,
	OP_SEI,
	OP_SEN,
	OP_SER,
	OP_SES,
	OP_SET,
	OP_SEV,
	OP_SEZ,
	OP_SLEEP,
	OP_SPM,
	OP_STD,
	OP_ST,
	OP_STS,
	OP_SUB,
	OP_SUBI,
	OP_SWAP,
	//OP_TST, (OP_AND)
	OP_WDR,
	OP_XCH,
};

enum base_pointer {
	BP_X,
	BP_Y,
	BP_Z
};

struct sreg {
	uint8_t C : 1;
	uint8_t Z : 1;
	uint8_t N : 1;
	uint8_t V : 1;
	uint8_t S : 1;
	uint8_t H : 1;
	uint8_t T : 1;
	uint8_t I : 1;
};

_Static_assert(sizeof(struct sreg) == 1, "sreg takes up 1 byte in data memory");

typedef struct instruction {
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
} instruction_t;

struct memory {
	unsigned int size;
	uint8_t *memory;
};

enum cpu_core {
	core_avr,   /* AVR */
	core_avre,  /* AVRe */
	core_avrep, /* AVRe+ */
	core_avrxm, /* AVRxm*/
	core_avrxt, /* AVRxt */
	core_avrrc  /* AVRrc */
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
	instruction_t current_inst;
	unsigned cycles_to_finish;
	uintmax_t cycle_count;
};

#define ATMEGA328P_SRAM_SIZE	2304
#define ATMEGA328P_FLASH_SIZE	(16384 * 2)
#define ATMEGA328P_EEPROM_SIZE	1024

struct atmega328p {
	struct cpu cpu;
	struct memory data;
	struct memory program;
	struct memory eeprom;
};

void atmega328p_init(struct atmega328p *mcu)
{
	memset(mcu, 0, sizeof(*mcu));

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
	mcu->cpu.sreg = (struct sreg *)&mcu->data.memory[0x3f];
	mcu->cpu.sp = (uint16_t *)&mcu->data.memory[0x3d];
	*mcu->cpu.sp = ATMEGA328P_SRAM_SIZE - 1;
}

void get_params_adc_like(const uint16_t *opcode, uint8_t *Rd, uint8_t *Rr)
{
	*Rd = (opcode[0] >> 4) & 0x1f;
	*Rr = (opcode[0] & 0xf) | (opcode[0] & 0x200 ? 0x10 : 0);
}

uint8_t get_param_bclr_like(const uint16_t *opcode)
{
	return (opcode[0] >> 4) & 0x7;
}

uint8_t get_param_asr_like(const uint16_t *opcode)
{
	return (opcode[0] >> 4) & 0x1f;
}

void get_params_andi_like(const uint16_t *opcode, uint8_t *Rd, uint8_t *K)
{
	*Rd = ((opcode[0] >> 4) & 0xf) + 16;
	*K = (opcode[0] >> 4) & 0xf0 | opcode[0] & 0xf;
}

void get_params_adiw_like(const uint16_t *opcode, uint8_t *Rd, uint8_t *K)
{
	*Rd = ((opcode[0] >> 4) & 0x3) * 2 + 24;
	*K = (((opcode[0] >> 6) << 4) | opcode[0] & 0xf) & 0x3f;
}

void get_params_sbrc_like(const uint16_t *opcode, uint8_t *Rd, uint8_t *b)
{
	*b = opcode[0] & 0x7;
	*Rd = (opcode[0] >> 4) & 0x1f;
}

void get_params_cbi_like(const uint16_t *opcode, uint8_t *A, uint8_t *b)
{
	*b = opcode[0] & 0x7;
	*A = (opcode[0] >> 3) & 0x1f;
}

/* Reinterpret val as a two's complement signed integer of x bits. */
#define SIGNED_X_BITS(x, val) (((struct {signed int i : x;}){.i = val}).i)

void get_branch_sreg_params(const uint16_t *opcode, int32_t *k, uint8_t *s)
{
	*s = opcode[0] & 0x7;
	*k = SIGNED_X_BITS(7, (opcode[0] >> 3) & 0x7f);
}

void get_branch_no_sreg_params(const uint16_t *opcode, int32_t *k)
{
	*k = SIGNED_X_BITS(7, (opcode[0] >> 3) & 0x7f);
}

uint32_t get_params_call_like(const uint16_t *opcode)
{
	uint32_t addr;
	addr = ((opcode[0] >> 4) << 1) | (opcode[0] & 1);
	addr = (addr << 16) | opcode[1];
	return addr;
}

int decode_instruction(const uint16_t *opcode, instruction_t *inst)
{
	if (opcode[0] == 0x9488) {
		inst->op = OP_CLC;
	}
	else if (opcode[0] == 0x94d8) {
		inst->op = OP_CLH;
	}
	else if (opcode[0] == 0x94f8) {
		inst->op = OP_CLI;
	}
	else if (opcode[0] == 0x94a8) {
		inst->op = OP_CLN;
	}
	else if (opcode[0] == 0x94c8) {
		inst->op = OP_CLS;
	}
	else if (opcode[0] == 0x94e8) {
		inst->op = OP_CLT;
	}
	else if (opcode[0] == 0x94b8) {
		inst->op = OP_CLV;
	}
	else if (opcode[0] == 0x9498) {
		inst->op = OP_CLZ;
	}
	else if (opcode[0] == 0x9408) {
		inst->op = OP_SEC;
	}
	else if (opcode[0] == 0x9458) {
		inst->op = OP_SEH;
	}
	else if (opcode[0] == 0x9478) {
		inst->op = OP_SEI;
	}
	else if (opcode[0] == 0x9428) {
		inst->op = OP_SEN;
	}
	else if (opcode[0] == 0x9448) {
		inst->op = OP_SES;
	}
	else if (opcode[0] == 0x9468) {
		inst->op = OP_SET;
	}
	else if (opcode[0] == 0x9438) {
		inst->op = OP_SEV;
	}
	else if (opcode[0] == 0x9418) {
		inst->op = OP_SEZ;
	}
	else if ((opcode[0] & 0xff8f) == 0x9488) {
		inst->op = OP_BCLR;
		inst->s = get_param_bclr_like(opcode);
	}
	else if ((opcode[0] & 0xff8f) == 0x9408) {
		inst->op = OP_BSET;
		inst->s = get_param_bclr_like(opcode);
	}
	else if (opcode[0] == 0x9509) {
		inst->op = OP_ICALL;
	}
	else if (opcode[0] == 0x9409) {
		inst->op = OP_IJMP;
	}
	else if (opcode[0] == 0x95c8) {
		inst->op = OP_LPM_R0;
	}
	else if ((opcode[0] & 0xfe0e) == 0x9004) {
		inst->op = OP_LPM;
		inst->Rd = (opcode[0] >> 4) & 0x1f;
		inst->bp_operation = opcode[0] & 1 ? BP_POST_INC : BP_NO_OP;
	}
	else if (opcode[0] == 0x95d8) {
		inst->op = OP_ELPM_R0;
	}
	else if ((opcode[0] & 0xfe0e) == 0x9006) {
		inst->op = OP_ELPM;
		inst->Rd = (opcode[0] >> 4) & 0x1f;
		inst->bp_operation = opcode[0] & 1 ? BP_POST_INC : BP_NO_OP;
	}
	else if (opcode[0] == 0x0) {
		inst->op = OP_NOP;
	}
	else if (opcode[0] == 0x9508) {
		inst->op = OP_RET;
	}
	else if (opcode[0] == 0x9518) {
		inst->op = OP_RETI;
	}
	else if (opcode[0] == 0x9588) {
		inst->op = OP_SLEEP;
	}
	else if (opcode[0] == 0x9598) {
		inst->op = OP_BREAK;
	}
	else if (opcode[0] == 0x95a8) {
		inst->op = OP_WDR;
	}
	else if (opcode[0] == 0x95e8) {
		inst->op = OP_SPM;
	}
	else if ((opcode[0] & 0xfc00) == 0x1c00) {
		inst->op = OP_ADC;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0xc00) {
		inst->op = OP_ADD;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x2000) {
		inst->op = OP_AND;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x1400) {
		inst->op = OP_CP;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x400) {
		inst->op = OP_CPC;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x1000) {
		inst->op = OP_CPSE;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x2400) {
		inst->op = OP_EOR;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x2c00) {
		inst->op = OP_MOV;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x9c00) {
		inst->op = OP_MUL;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x2800) {
		inst->op = OP_OR;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x800) {
		inst->op = OP_SBC;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xfc00) == 0x1800) {
		inst->op = OP_SUB;
		get_params_adc_like(opcode, &inst->Rd, &inst->Rr);
	}
	else if ((opcode[0] & 0xf000) == 0x7000) {
		inst->op = OP_ANDI;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xf000) == 0xe000) {
		inst->op = OP_LDI;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xff0f) == 0xef0f) {
		inst->op = OP_SER;
		inst->Rd = ((opcode[0] >> 4) & 0xf) + 16;
	}
	else if ((opcode[0] & 0xf000) == 0x6000) {
		inst->op = OP_ORI;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xf000) == 0x6000) {
		inst->op = OP_SBR;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xf000) == 0x3000) {
		inst->op = OP_CPI;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xf000) == 0x4000) {
		inst->op = OP_SBCI;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xf000) == 0x5000) {
		inst->op = OP_SUBI;
		get_params_andi_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xfe08) == 0xfc00) {
		inst->op = OP_SBRC;
		get_params_sbrc_like(opcode, &inst->Rd, &inst->b);
	}
	else if ((opcode[0] & 0xfe08) == 0xfe00) {
		inst->op = OP_SBRS;
		get_params_sbrc_like(opcode, &inst->Rd, &inst->b);
	}
	else if ((opcode[0] & 0xfe08) == 0xf800) {
		inst->op = OP_BLD;
		get_params_sbrc_like(opcode, &inst->Rd, &inst->b);
	}
	else if ((opcode[0] & 0xfe08) == 0xfa00) {
		inst->op = OP_BST;
		get_params_sbrc_like(opcode, &inst->Rd, &inst->b);
	}
	else if ((opcode[0] & 0xf800) == 0xb000) {
		inst->op = OP_IN;
		inst->Rd = (opcode[0] >> 4) & 0x1f;
		inst->A = opcode[0] & 0xf;
		inst->A |= (opcode[0] >> 5) & 0x30;
	}
	else if ((opcode[0] & 0xf800) == 0xb800) {
		inst->op = OP_OUT;
		inst->Rr = (opcode[0] >> 4) & 0x1f;
		inst->A = opcode[0] & 0xf;
		inst->A |= (opcode[0] >> 5) & 0x30;
	}
	else if ((opcode[0] & 0xff00) == 0x9600) {
		inst->op = OP_ADIW;
		get_params_adiw_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xff00) == 0x9700) {
		inst->op = OP_SBIW;
		get_params_adiw_like(opcode, &inst->Rd, &inst->K);
	}
	else if ((opcode[0] & 0xff00) == 0x9800) {
		inst->op = OP_CBI;
		get_params_cbi_like(opcode, &inst->A, &inst->b);
	}
	else if ((opcode[0] & 0xff00) == 0x9a00) {
		inst->op = OP_SBI;
		get_params_cbi_like(opcode, &inst->A, &inst->b);
	}
	else if ((opcode[0] & 0xff00) == 0x9900) {
		inst->op = OP_SBIC;
		get_params_cbi_like(opcode, &inst->A, &inst->b);
	}
	else if ((opcode[0] & 0xff00) == 0x9b00) {
		inst->op = OP_SBIS;
		get_params_cbi_like(opcode, &inst->A, &inst->b);
	}
	else if ((opcode[0] & 0xfc07) == 0xf400) {
		inst->op = OP_BRCC;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf000) {
		inst->op = OP_BRCS;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf001) {
		inst->op = OP_BREQ;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf404) {
		inst->op = OP_BRGE;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf405) {
		inst->op = OP_BRHC;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf005) {
		inst->op = OP_BRHS;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf407) {
		inst->op = OP_BRID;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf007) {
		inst->op = OP_BRIE;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf000) {
		inst->op = OP_BRLO;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf004) {
		inst->op = OP_BRLT;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf002) {
		inst->op = OP_BRMI;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf401) {
		inst->op = OP_BRNE;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf402) {
		inst->op = OP_BRPL;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf400) {
		inst->op = OP_BRSH;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf406) {
		inst->op = OP_BRTC;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf006) {
		inst->op = OP_BRTS;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf403) {
		inst->op = OP_BRVC;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc07) == 0xf003) {
		inst->op = OP_BRVS;
		get_branch_no_sreg_params(opcode, &inst->k);
	}
	else if ((opcode[0] & 0xfc00) == 0xf400) {
		inst->op = OP_BRBC;
		get_branch_sreg_params(opcode, &inst->k, &inst->s);
	}
	else if ((opcode[0] & 0xfc00) == 0xf000) {
		inst->op = OP_BRBS;
		get_branch_sreg_params(opcode, &inst->k, &inst->s);
	}
	else if ((opcode[0] & 0xf000) == 0xd000) {
		inst->op = OP_RCALL;
		inst->k = opcode[0] & 0xfff;
	}
	else if ((opcode[0] & 0xf000) == 0xc000) {
		inst->op = OP_RJMP;
		inst->k = opcode[0] & 0xfff;
	}
	else if ((opcode[0] & 0xfe0e) == 0x940e) {
		inst->op = OP_CALL;
		inst->k = get_params_call_like(opcode);
	}
	else if ((opcode[0] & 0xfe0e) == 0x940c) {
		inst->op = OP_JMP;
		inst->k = get_params_call_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9405) {
		inst->op = OP_ASR;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9400) {
		inst->op = OP_COM;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x940a) {
		inst->op = OP_DEC;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9403) {
		inst->op = OP_INC;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9406) {
		inst->op = OP_LSR;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9401) {
		inst->op = OP_NEG;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x900f) {
		inst->op = OP_POP;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x920f) {
		inst->op = OP_PUSH;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9407) {
		inst->op = OP_ROR;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xfe0f) == 0x9402) {
		inst->op = OP_SWAP;
		inst->Rd = get_param_asr_like(opcode);
	}
	else if ((opcode[0] & 0xff00) == 0x100) {
		inst->op = OP_MOVW;
		inst->Rd = ((opcode[0] >> 4) & 0xf) * 2;
		inst->Rr = (opcode[0] & 0xf) * 2;
	}
	else if ((opcode[0] & 0xff00) == 0x200) {
		inst->op = OP_MULS;
		inst->Rd = ((opcode[0] >> 4) & 0xf) + 16;
		inst->Rr = (opcode[0] & 0xf) + 16;
	}
	else if ((opcode[0] & 0xff88) == 0x300) {
		inst->op = OP_MULSU;
		inst->Rd = ((opcode[0] >> 4) & 0x7) + 16;
		inst->Rr = (opcode[0] & 0x7) + 16;
	}
	else if ((opcode[0] & 0xff88) == 0x308) {
		inst->op = OP_FMUL;
		inst->Rd = ((opcode[0] >> 4) & 0x7) + 16;
		inst->Rr = (opcode[0] & 0x7) + 16;
	}
	else if ((opcode[0] & 0xff88) == 0x380) {
		inst->op = OP_FMULS;
		inst->Rd = ((opcode[0] >> 4) & 0x7) + 16;
		inst->Rr = (opcode[0] & 0x7) + 16;
	}
	else if ((opcode[0] & 0xff88) == 0x388) {
		inst->op = OP_FMULSU;
		inst->Rd = ((opcode[0] >> 4) & 0x7) + 16;
		inst->Rr = (opcode[0] & 0x7) + 16;
	}
	else if ((opcode[0] & 0xfe0f) == 0x9200) {
		inst->op = OP_STS;
		inst->Rr = (opcode[0] >> 4) & 0x1f;
		inst->k = opcode[1];
	}
	else if ((opcode[0] & 0xfe0f) == 0x9000) {
		inst->op = OP_LDS;
		inst->Rd = (opcode[0] >> 4) & 0x1f;
		inst->k = opcode[1];
	}
	else if ((opcode[0] & 0xd200) == 0x8000) {
		inst->op = OP_LDD;
		inst->Rd = (opcode[0] >> 4) & 0x1f;
		inst->bp = opcode[0] & 0x8 ? BP_Y : BP_Z;
		inst->q = opcode[0] & 0x3;
		inst->q |= (opcode[0] >> 7) & 0x18;
		inst->q |= (opcode[0] >> 8) & 0x20;
	}
	else if ((opcode[0] & 0xee00) == 0x8000) {
		inst->op = OP_LD;
		inst->Rd = (opcode[0] >> 4) & 0x1f;
		switch (opcode[0] & 0xc) {
		case 0:
			inst->bp = BP_Z;
			break;
		case 8:
			inst->bp = BP_Y;
			break;
		case 12:
			inst->bp = BP_X;
			break;
		default:
			// illegal
			break;
		}
		switch (opcode[0] & 0x3) {
		case 0:
			inst->bp_operation = BP_NO_OP;
			break;
		case 1:
			inst->bp_operation = BP_POST_INC;
			break;
		case 2:
			inst->bp_operation = BP_PRE_DEC;
			break;
		default:
			// illegal
			break;
		}
	}
	else if ((opcode[0] & 0xd200) == 0x8200) {
		inst->op = OP_STD;
		inst->Rr = (opcode[0] >> 4) & 0x1f;
		inst->bp = opcode[0] & 0x8 ? BP_Y : BP_Z;
		inst->q = opcode[0] & 0x3;
		inst->q |= (opcode[0] >> 7) & 0x18;
		inst->q |= (opcode[0] >> 8) & 0x20;
	}
	else if ((opcode[0] & 0xee00) == 0x8200) {
		inst->op = OP_ST;
		inst->Rr = (opcode[0] >> 4) & 0x1f;
		switch (opcode[0] & 0xc) {
		case 0:
			inst->bp = BP_Z;
			break;
		case 8:
			inst->bp = BP_Y;
			break;
		case 12:
			inst->bp = BP_X;
			break;
		default:
			// illegal
			break;
		}
		switch (opcode[0] & 0x3) {
		case 0:
			inst->bp_operation = BP_NO_OP;
			break;
		case 1:
			inst->bp_operation = BP_POST_INC;
			break;
		case 2:
			inst->bp_operation = BP_PRE_DEC;
			break;
		default:
			// illegal
			break;
		}
	}
	else if (opcode[0] == 0x9519) {
		inst->op = OP_EICALL;
	}
	else if (opcode[0] == 0x9419) {
		inst->op = OP_EIJMP;
	}
	else {
		warn("unimplemented opcode, interpret as nop\n");
		inst->op = OP_NOP;
	}

	return 0;
}

/* Return opcode length (1 or 2) in words. */
int opcode_length(const uint16_t *opcode)
{
	if (((opcode[0] & 0xfe0e) == 0x940e) || // CALL
		((opcode[0] & 0xfe0e) == 0x940c) || // JMP
		((opcode[0] & 0xfe0f) == 0x9000) || // LDS
		((opcode[0] & 0xfe0f) == 0x9200)) { // STS
		return 2;
	}

	return 1;
}

void stack_push(struct cpu *cpu, void *data, int size)
{
	*cpu->sp -= size;
	memcpy(&cpu->data->memory[*cpu->sp], data, size);
}

void stack_pop(struct cpu *cpu, void *data, int size)
{
	memcpy(data, &cpu->data->memory[*cpu->sp], size);
	*cpu->sp += size;
}

void cpu_cycle(struct cpu *cpu)
{
	uint16_t *opcode;
	int opcode_words;
	int decode_status;

	if (cpu->cycles_to_finish == 0) {
		/* Fetch */
		opcode = (uint16_t *)cpu->program->memory + cpu->pc;
		opcode_words = opcode_length(opcode);
		if (cpu->pc + opcode_words > cpu->program->size / 2) {
			// out of bounds
		}
		cpu->pc += opcode_words;

		/* Decode */
		decode_status = decode_instruction(opcode, &cpu->current_inst);
		if (FAILED(decode_status)) {
			// decode error
		}
	}

	/* Execute */
	printf("inst op = %d\n", cpu->current_inst.op);

#define BIT2MASK(bitpos) (1 << (bitpos))
#define BITSET(value, bitpos) ((value) |= BIT2MASK(bitpos))
#define BITCLR(value, bitpos) ((value) &= ~BIT2MASK(bitpos))
#define BITVAL(value, bitpos) (((value) & BIT2MASK(bitpos)) != 0)

#define REG(n) cpu->reg_file[n]
#define A cpu->io_reg[cpu->current_inst.A]
#define Rd REG(cpu->current_inst.Rd)
#define Rr REG(cpu->current_inst.Rr)
#define SREG (*cpu->sreg)
#define K cpu->current_inst.K
#define k cpu->current_inst.k
#define s cpu->current_inst.s
#define b cpu->current_inst.b

	uint16_t R = 0;
	switch (cpu->current_inst.op) {
	case OP_ADC:
		R += SREG.C;
		/* fallthrough */
	case OP_ADD:
		R += Rd + Rr;

		/* H <=> there was a carry from bit 3. */
		SREG.H = BITVAL(Rd, 3) && BITVAL(Rr, 3) ||
				 BITVAL(Rr, 3) && !BITVAL(R, 3) ||
				 !BITVAL(R, 3) && BITVAL(Rd, 3);
		/* V <=> two's complement overflow resulted from the operation. */
		SREG.V = BITVAL(Rd, 7) && BITVAL(Rr, 7) && !BITVAL(R, 7) ||
				 !BITVAL(Rd, 7) && !BITVAL(Rr, 7) && BITVAL(R, 7);
		/* N <=> MSB of the result is set. */
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		/* C <=> there was a carry from the MSB of the result. */
		SREG.C = BITVAL(Rd, 7) && BITVAL(Rr, 7) ||
				 BITVAL(Rr, 7) && !BITVAL(R, 7) ||
				 !BITVAL(R, 7) && BITVAL(Rd, 7);
		Rd = R;
		break;

	case OP_ADIW:
		R = (int)Rd + K;

		/* V <=> two's complement overflow resulted from the operation. */
		SREG.V = BITVAL(R, 15) && !BITVAL(1[&Rd], 7);
		/* N <=> MSB of the result is set. */
		SREG.N = BITVAL(R, 15);
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		SREG.C = !BITVAL(R, 15) && BITVAL(1[&Rd], 7);

		memcpy(&Rd, &R, 2);
		break;

	case OP_AND:
	case OP_ANDI:
		if (cpu->current_inst.op == OP_AND) {
			R = Rd & Rr;
		}
		else {
			R = Rd & K;
		}
		SREG.V = 0;
		/* N <=> MSB of the result is set. */
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		Rd = R;
		break;

	case OP_ASR:
		R = Rd >> 1;
		R |= Rd & BIT2MASK(7); /* bit 7 is held constant. */
		SREG.C = Rd & 1; /* Bit 0 is loaded into the C flag. */
		/* N <=> MSB of the result is set. */
		SREG.N = BITVAL(R, 7);
		SREG.V = SREG.N ^ SREG.C;
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		Rd = R;
		break;

	case OP_BCLR:
		BITCLR(*(unsigned char *)&SREG, s);
		break;
	
	case OP_BLD:
		if (SREG.T) {
			BITSET(Rd, b);
		}
		else {
			BITCLR(Rd, b);
		}
		break;

	case OP_BRBC:
		if (BITVAL(*(unsigned char *)&SREG, s) == 0) {
			cpu->pc += k;
		}
		break;
	case OP_BRBS:
		if (BITVAL(*(unsigned char *)&SREG, s) == 1) {
			cpu->pc += k;
		}
		break;
	case OP_BRCC:
		if (!SREG.C) {
			cpu->pc += k;
		}
		break;
	case OP_BRCS:
		if (SREG.C) {
			cpu->pc += k;
		}
		break;
	case OP_BREQ:
		if (SREG.Z) {
			cpu->pc += k;
		}
		break;
	case OP_BRGE:
		if (!SREG.S) {
			cpu->pc += k;
		}
		break;
	case OP_BRHC:
		if (!SREG.H) {
			cpu->pc += k;
		}
		break;
	case OP_BRHS:
		if (SREG.H) {
			cpu->pc += k;
		}
		break;
	case OP_BRID:
		if (!SREG.I) {
			cpu->pc += k;
		}
		break;
	case OP_BRIE:
		if (SREG.I) {
			cpu->pc += k;
		}
		break;
	case OP_BRLO:
		if (SREG.C) {
			cpu->pc += k;
		}
		break;
	case OP_BRLT:
		if (SREG.S) {
			cpu->pc += k;
		}
		break;
	case OP_BRMI:
		if (SREG.N) {
			cpu->pc += k;
		}
		break;
	case OP_BRNE:
		if (!SREG.Z) {
			cpu->pc += k;
		}
		break;
	case OP_BRSH:
		if (!SREG.C) {
			cpu->pc += k;
		}
		break;
	case OP_BRTC:
		if (!SREG.T) {
			cpu->pc += k;
		}
		break;
	case OP_BRTS:
		if (SREG.T) {
			cpu->pc += k;
		}
		break;
	case OP_BRVC:
		if (!SREG.V) {
			cpu->pc += k;
		}
		break;
	case OP_BRVS:
		if (SREG.V) {
			cpu->pc += k;
		}
		break;

	case OP_BSET:
		BITSET(*(unsigned char *)&SREG, s);
		break;
	
	case OP_BST:
		SREG.T = BITVAL(Rd, b);
		break;
	
	case OP_CALL:
		stack_push(cpu, &cpu->pc, 2);
		/* fallthrough */
	case OP_JMP:
		if (k < 0 || k >= cpu->program->size) {
			// illegal
		}
		cpu->pc = k;
		break;
	
	case OP_CBI:
		BITCLR(cpu->io_reg[A], b);
		break;

	case OP_CLC:
		SREG.C = 0;
		break;
	case OP_CLH:
		SREG.H = 0;
		break;
	case OP_CLI:
		SREG.I = 0;
		break;
	case OP_CLN:
		SREG.N = 0;
		break;
	case OP_CLS:
		SREG.S = 0;
		break;
	case OP_CLT:
		SREG.T = 0;
		break;
	case OP_CLV:
		SREG.V = 0;
		break;
	case OP_CLZ:
		SREG.Z = 0;
		break;
	case OP_COM:
		R = 0xff - Rd;
		SREG.V = 0;
		SREG.C = 1;
		SREG.N = BITVAL(R, 7);
		SREG.Z = R == 0;
		SREG.S = SREG.N ^ SREG.V;
		Rd = R;
		break;
	case OP_CPC:
		R = SREG.C;
		goto case_OP_CP;
	case OP_CPI:
		Rr = K;/*bug*/
		goto case_OP_CP;
	case OP_CP:
case_OP_CP:
		R = (uint8_t)(Rd - Rr - R);
		SREG.Z = R == 0;
		SREG.H = !BITVAL(Rd, 3) && BITVAL(Rr, 3) ||
			     BITVAL(Rr, 3) && BITVAL(R, 3) ||
				 BITVAL(R, 3) && !BITVAL(Rd, 3);
		SREG.N = BITVAL(R, 7);
		/* V <=> two's complement overflow resulted from the operation. */
		SREG.V = BITVAL(Rd, 7) && !BITVAL(Rr, 7) && !BITVAL(R, 7) ||
				 !BITVAL(Rd, 7) && BITVAL(Rr, 7) && BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		/*
		 * CP: C <=> absolute value of the contents of Rr is larger than the
		 *	   absolute value of Rd.
		 * CPC: C <=> absolute value of the contents of Rr plus previous carry
		 *		is larger than the absolute value of Rd.
		 * CPI: C <=> absolute value of K is larger than the absolute value
		 *		of Rd.
		 */
		SREG.C = !BITVAL(Rd, 7) && BITVAL(Rr, 7) ||
			     BITVAL(Rr, 7) && BITVAL(R, 7) ||
				 BITVAL(R, 7) && !BITVAL(Rd, 7);
		break;
	
	case OP_CPSE:
		if (Rd == Rr) {
			/* skip next instruction */
			cpu->pc += opcode_length((uint16_t *)cpu->program->memory + cpu->pc);
		}
		break;

	case OP_DEC:
		R = (uint8_t)(Rd - 1);
		/* V <=> two's complement overflow resulted from the operation. */
		SREG.V = Rd == 0x80;
		SREG.Z = R == 0;
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		Rd = R;
		break;
	
	case OP_EOR:
		R = Rd ^ Rr;
		SREG.V = 0;
		SREG.Z = R == 0;
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		Rd = R;
		break;

	case OP_FMUL:
		R = Rd * Rr;
		SREG.C = BITVAL(R, 15);
		R <<= 1;
		SREG.Z = R == 0;
		/* Store in R1:R0. */
		memcpy(&REG(0), &R, 2);
		break;
	case OP_FMULS:
		R = (int8_t)Rd * (int8_t)Rr;
		SREG.C = BITVAL(R, 15);
		R <<= 1;
		SREG.Z = R == 0;
		/* Store in R1:R0. */
		memcpy(&REG(0), &R, 2);
		break;
	case OP_FMULSU:
		R = (int8_t)Rd * Rr;
		SREG.C = BITVAL(R, 15);
		R <<= 1;
		SREG.Z = R == 0;
		/* Store in R1:R0. */
		memcpy(&REG(0), &R, 2);
	
	case OP_ICALL:
		stack_push(cpu, &cpu->pc, 2);
		/* fallthrough */
	case OP_IJMP:
		/* fixme: put bounds checking */
		memcpy(&cpu->pc, &REG(30), 2);
		break;
	
	case OP_IN:
		Rd = cpu->io_reg[A];
		break;
	
	case OP_INC:
		R = (uint8_t)(Rd + 1);
		SREG.V = Rd == 0x7f;
		SREG.Z = R == 0;
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		Rd = R;
		break;
	
	case OP_LD:
		break;
	
	case OP_LDD:
		break;

	case OP_LDI:
		Rd = K;
		break;

	case OP_LSR:
		R = Rd >> 1;
		SREG.C = Rd & 1;
		SREG.N = 0;
		SREG.V = SREG.V ^ SREG.C;
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		Rd = R;
		break;

	case OP_MOV:
		Rd = Rr;
		break;
	case OP_MOVW:
		memmove(&Rd, &Rr, 2);
		break;
	case OP_MUL:
		R = Rd * Rr;
		SREG.C = BITVAL(R, 15);
		SREG.Z = R == 0;
		/* Store in R1:R0. */
		memcpy(&REG(0), &R, 2);
		break;
	case OP_MULS:
		R = (int8_t)Rd * (int8_t)Rr;
		SREG.C = BITVAL(R, 15);
		SREG.Z = R == 0;
		/* Store in R1:R0. */
		memcpy(&REG(0), &R, 2);
		break;
	case OP_MULSU:
		R = (int8_t)Rd * Rr;
		SREG.C = BITVAL(R, 15);
		SREG.Z = R == 0;
		/* Store in R1:R0. */
		memcpy(&REG(0), &R, 2);
		break;
	case OP_NEG:
		R = -Rd;
		SREG.H = BITVAL(R, 3) | BITVAL(Rd, 3);
		/* V <=> two's complement overflow resulted from the operation. */
		SREG.V = (R & 0xff) == 0x80;
		/* N <=> MSB of the result is set. */
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		SREG.C = R != 0;
		Rd = R;
		break;
	case OP_NOP:
		break;
	case OP_OR:
		R = Rd | Rr;
		SREG.V = 0;
		SREG.N = BITVAL(R, 7);
		SREG.S = SREG.N ^ SREG.V;
		SREG.Z = R == 0;
		Rd = R;
		break;

	case OP_SEC:
		SREG.C = 1;
		break;
	case OP_SEH:
		SREG.H = 1;
		break;
	case OP_SEI:
		SREG.I = 1;
		break;
	case OP_SEN:
		SREG.N = 1;
		break;
	case OP_SES:
		SREG.S = 1;
		break;
	case OP_SET:
		SREG.T = 1;
		break;
	case OP_SEV:
		SREG.V = 1;
		break;
	case OP_SEZ:
		SREG.Z = 1;
		break;
	case OP_SER:
		Rd = 0xff;
		break;
	case OP_SWAP:
		Rd = (Rd << 4) | (Rd >> 4);
		break;
	default:
		warn("unimplemented instruction\n");
		break;
	}


	cpu->cycle_count++;
}

int main(int argc, char *argv[])
{
	struct atmega328p mcu;

	atmega328p_init(&mcu);

	int actual = fread(mcu.program.memory, 2, 1000, stdin);

	for (int i = 0; i < actual + 20; ++i) {
		cpu_cycle(&mcu.cpu);
	}

	return 0;
}
