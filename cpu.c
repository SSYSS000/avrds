#include <string.h>
#include <stdio.h>
#include "cpu.h"
#include "defines.h"
#include "log.h"

#define REG(n) cpu->reg_file[n]
#define A cpu->io_reg[cpu->current_inst.A]
#define Rd REG(cpu->current_inst.Rd)
#define Rr REG(cpu->current_inst.Rr)
#define SREG (*cpu->sreg)
#define K cpu->current_inst.K
#define k cpu->current_inst.k
#define s cpu->current_inst.s
#define b cpu->current_inst.b
#define FAILED(status) ((status) < 0)

static void stack_push(struct cpu *cpu, void *data, int size)
{
	*cpu->sp -= size;
	memcpy(&cpu->data->memory[*cpu->sp], data, size);
}

static void stack_pop(struct cpu *cpu, void *data, int size)
{
	memcpy(data, &cpu->data->memory[*cpu->sp], size);
	*cpu->sp += size;
}

void cpu_cycle(struct cpu *cpu)
{
	uint16_t *opcode;
	int opcode_words;
	int decode_status;
	uint16_t R = 0;

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
