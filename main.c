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

struct cpu {
	struct memory *program;
	struct memory *data;
	uint8_t *reg_file; /* General purpose registers R0-R31 */
	uint8_t *sreg; /* Status register */
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
	mcu->cpu.sreg = &mcu->data.memory[0x3f];
	mcu->cpu.sp = (uint16_t *)&mcu->data.memory[0x3d];
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

void get_branch_sreg_params(const uint16_t *opcode, int32_t *k, uint8_t *s)
{
	*s = opcode[0] & 0x7;
	*k = (opcode[0] >> 3) & 0x7f; // twos
}

void get_branch_no_sreg_params(const uint16_t *opcode, int32_t *k)
{
	*k = (opcode[0] >> 3) & 0x7f; // twos
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