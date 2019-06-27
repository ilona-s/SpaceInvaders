#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "i8080.h"

#define MSB(BYTE) ((BYTE & 0x80) >> 7)
#define LSB(BYTE) (BYTE & 0x1)

#define HIGH_NIBBLE(BYTE) ((BYTE & 0xF0) >> 4)
#define LOW_NIBBLE(BYTE) (BYTE & 0xF)

#define HIGH_BYTE(WORD) ((WORD & 0xFF00) >> 8)
#define LOW_BYTE(WORD) (WORD & 0xFF)

#define PAIR(HIGH, LOW) ((HIGH << 8) | LOW)

bool parity_table[256] = {1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
			  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};

i8080 *i8080_init() {
	i8080 *cpu = malloc(sizeof(i8080));
	if (!cpu) {
		perror("malloc");
		exit(1);
	}

	cpu->PC = 0;
	cpu->SP = 0;

	memset(cpu->memory, 0, sizeof(cpu->memory));

	cpu->A = 0; cpu->F = 2; // Flag bit 1 always high
	cpu->B = 0; cpu->C = 0;
	cpu->D = 0; cpu->E = 0;
	cpu->H = 0; cpu->L = 0;

	cpu->inte = false;
	cpu->halt = false;

	cpu->interrupt_occurred = false;
	cpu->interrupt_opcode = 0;

	cpu->input_handler = NULL;
	cpu->output_handler = NULL;
	cpu->user_data = NULL;

	return cpu;
}

void i8080_quit(i8080 *cpu) {
	free(cpu);
}

void i8080_load_ROM(i8080 *cpu, char *path, int offset) {
	FILE *file = fopen(path, "rb");
	if (!file) {
		perror("fopen");
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	uint32_t fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (fsize + offset > MEM_SIZE) {
		fprintf(stderr, "File too large\n");
		exit(1);
	}

	if (fread(cpu->memory + offset, 1, fsize, file) != fsize) {
		perror("fread");
		exit(1);
	}

	if (fclose(file) < 0) {
		perror("fclose");
		exit(1);
	}
}

void i8080_generate_interrupt(i8080 *cpu, uint8_t opcode) {
	cpu->halt = false;

	if (cpu->inte) {
		cpu->inte = false;

		cpu->interrupt_occurred = true;
		cpu->interrupt_opcode = opcode;
	}
}

uint8_t i8080_mem_read_byte(i8080 *cpu, uint16_t addr) {
	return cpu->memory[addr];
}

void i8080_mem_write_byte(i8080 *cpu, uint16_t addr, uint8_t data) {
	cpu->memory[addr] = data;
}

uint8_t i8080_get_next_byte(i8080 *cpu) {
	return i8080_mem_read_byte(cpu, cpu->PC++);
}

uint16_t i8080_get_next_word(i8080 *cpu) {
	uint8_t low_byte = i8080_get_next_byte(cpu);
	uint8_t high_byte = i8080_get_next_byte(cpu);

	return high_byte << 8 | low_byte;
}

uint8_t i8080_get_opcode(i8080 *cpu) {
	if (cpu->interrupt_occurred) {
		cpu->interrupt_occurred = false;
		return cpu->interrupt_opcode;
	} else {
		return i8080_get_next_byte(cpu);
	}
}

void i8080_push_stack_byte(i8080 *cpu, uint8_t data) {
	i8080_mem_write_byte(cpu, --cpu->SP, data);
}

uint8_t i8080_pop_stack_byte(i8080 *cpu) {
	return i8080_mem_read_byte(cpu, cpu->SP++);
}

void i8080_push_stack_word(i8080 *cpu, uint16_t data) {
	i8080_push_stack_byte(cpu, HIGH_BYTE(data));
	i8080_push_stack_byte(cpu, LOW_BYTE(data));
}

uint16_t i8080_pop_stack_word(i8080 *cpu) {
	uint8_t low_byte = i8080_pop_stack_byte(cpu);
	uint8_t high_byte = i8080_pop_stack_byte(cpu);

	return high_byte << 8 | low_byte;
}

uint8_t i8080_get_flag_bit_mask(flag_bit bit) {
	return 1 << bit;
}

bool i8080_get_flag_bit(i8080 *cpu, flag_bit bit) {
	return (cpu->F & i8080_get_flag_bit_mask(bit)) >> bit;
}

bool i8080_check_condition(i8080 *cpu, uint8_t condition) {
	switch (condition) {
		case 0:
			return !i8080_get_flag_bit(cpu, ZERO_FLAG);
		case 1:
			return i8080_get_flag_bit(cpu, ZERO_FLAG);
		case 2:
			return !i8080_get_flag_bit(cpu, CARRY_FLAG);
		case 3:
			return i8080_get_flag_bit(cpu, CARRY_FLAG);
		case 4:
			return !i8080_get_flag_bit(cpu, PARITY_FLAG);
		case 5:
			return i8080_get_flag_bit(cpu, PARITY_FLAG);
		case 6:
			return !i8080_get_flag_bit(cpu, SIGN_FLAG);
		case 7:
			return i8080_get_flag_bit(cpu, SIGN_FLAG);
		default:
			fprintf(stderr, "Invalid condition\n");
			exit(1);
	}
}

void i8080_set_flag_bit(i8080 *cpu, flag_bit bit) {
	cpu->F |= i8080_get_flag_bit_mask(bit);
}

void i8080_clear_flag_bit(i8080 *cpu, flag_bit bit) {
	cpu->F &= (~i8080_get_flag_bit_mask(bit) & 0xFF);
}

void i8080_set_AC_add(i8080 *cpu, uint8_t addend1, uint8_t addend2, bool carry) {
	uint8_t bit4 = ((addend1 & 0xF) + (addend2 & 0xF) + carry) & 0x10;
	bit4 ? i8080_set_flag_bit(cpu, AUX_CARRY_FLAG) : i8080_clear_flag_bit(cpu, AUX_CARRY_FLAG);
}

void i8080_set_AC_sub(i8080 *cpu, uint8_t minuend, uint8_t subtrahend, bool borrow) {
	uint8_t bit4 = ((minuend & 0xF) + (~subtrahend & 0xF) + (borrow ? 0 : 1)) & 0x10;
	bit4 ? i8080_set_flag_bit(cpu, AUX_CARRY_FLAG) : i8080_clear_flag_bit(cpu, AUX_CARRY_FLAG);
}

void i8080_set_SZP(i8080 *cpu, uint8_t val) {
	(val & 0x80) ? i8080_set_flag_bit(cpu, SIGN_FLAG) : i8080_clear_flag_bit(cpu, SIGN_FLAG);
	(val == 0x00) ? i8080_set_flag_bit(cpu, ZERO_FLAG) : i8080_clear_flag_bit(cpu, ZERO_FLAG);
	(parity_table[val]) ? i8080_set_flag_bit(cpu, PARITY_FLAG) : i8080_clear_flag_bit(cpu, PARITY_FLAG);
}

uint16_t i8080_get_PSW(i8080 *cpu) {
	return cpu->A << 8 | cpu->F;
}

void i8080_set_PSW(i8080 *cpu, uint16_t val) {
	cpu->A = HIGH_BYTE(val); cpu->F = LOW_BYTE(val);

	i8080_set_flag_bit(cpu, FLAG_1);
	i8080_clear_flag_bit(cpu, FLAG_3);
	i8080_clear_flag_bit(cpu, FLAG_5);
}

uint16_t i8080_get_register_pair(i8080 *cpu, int pair_no) {
	switch (pair_no) {
		case 0:
			return cpu->B << 8 | cpu->C;
		case 1:
			return cpu->D << 8 | cpu->E;
		case 2:
			return cpu->H << 8 | cpu->L;
		case 3:
			return cpu->SP;
		default:
			fprintf(stderr, "Invalid register pair\n");
			exit(1);
	}
}

void i8080_set_register_pair(i8080 *cpu, int pair_no, uint16_t val) {
	switch (pair_no) {
		case 0:
			cpu->B = HIGH_BYTE(val);
			cpu->C = LOW_BYTE(val);
			break;
		case 1:
			cpu->D = HIGH_BYTE(val);
			cpu->E = LOW_BYTE(val);
			break;
		case 2:
			cpu->H = HIGH_BYTE(val);
			cpu->L = LOW_BYTE(val);
			break;
		case 3:
			cpu->SP = val;
			break;
		default:
			fprintf(stderr, "Invalid register pair\n");
			exit(1);
	}
}

uint8_t i8080_get_register(i8080 *cpu, int reg_no) {
	switch (reg_no) {
		case 0:
			return cpu->B;
		case 1:
			return cpu->C;
		case 2:
			return cpu->D;
		case 3:
			return cpu->E;
		case 4:
			return cpu->H;
		case 5:
			return cpu->L;
		case 6: // Memory reference addressed by registers H and L
			return i8080_mem_read_byte(cpu, PAIR(cpu->H, cpu->L));
		case 7:
			return cpu->A;
		default:
			fprintf(stderr, "Invalid register\n");
			exit(1);
	}
}

void i8080_set_register(i8080 *cpu, int reg_no, uint8_t val) {
	switch (reg_no) {
		case 0:
			cpu->B = val;
			break;
		case 1:
			cpu->C = val;
			break;
		case 2:
			cpu->D = val;
			break;
		case 3:
			cpu->E = val;
			break;
		case 4:
			cpu->H = val;
			break;
		case 5:
			cpu->L = val;
			break;
		case 6: // Memory reference addressed by registers H and L
			i8080_mem_write_byte(cpu, PAIR(cpu->H, cpu->L), val);
			break;
		case 7:
			cpu->A = val;
			break;
		default:
			fprintf(stderr, "Invalid register\n");
			exit(1);
	}
}

uint8_t i8080_general_add(i8080 *cpu, uint8_t addend1, uint8_t addend2, bool carry) {
	uint16_t sum16 = addend1 + addend2 + carry;
	uint8_t sum8 = sum16 & 0xFF;

	(sum16 & 0x100) ? i8080_set_flag_bit(cpu, CARRY_FLAG) : i8080_clear_flag_bit(cpu, CARRY_FLAG);
	i8080_set_AC_add(cpu, addend1, addend2, carry);
	i8080_set_SZP(cpu, sum8);

	return sum8;
}

uint8_t i8080_general_sub(i8080 *cpu, uint8_t minuend, uint8_t subtrahend, bool borrow) {
	// Need to add 1 to form two's complement -- if borrow is true, carry is cancelled out
	// Reference: https://en.wikipedia.org/wiki/Carry_flag
	uint16_t diff16 = minuend + (~subtrahend & 0xFF) + (borrow ? 0 : 1);
	uint16_t diff8 = diff16 & 0xFF;

	!(diff16 & 0x100) ? i8080_set_flag_bit(cpu, CARRY_FLAG) : i8080_clear_flag_bit(cpu, CARRY_FLAG);
	i8080_set_AC_sub(cpu, minuend, subtrahend, borrow);
	i8080_set_SZP(cpu, diff8);

	return diff8;
}

uint8_t i8080_logic_and(i8080 *cpu, uint8_t val1, uint8_t val2) {
	uint8_t logical_and = val1 & val2;

	i8080_clear_flag_bit(cpu, CARRY_FLAG);
	// Auxiliary carry reflects the logical OR of bit 3 of the operands
	((val1 | val2) & 0x08) ? i8080_set_flag_bit(cpu, AUX_CARRY_FLAG) : i8080_clear_flag_bit(cpu, AUX_CARRY_FLAG);
	i8080_set_SZP(cpu, logical_and);

	return logical_and;
}

uint8_t i8080_logic_xor(i8080 *cpu, uint8_t val1, uint8_t val2) {
	uint8_t logical_xor = val1 ^ val2;

	i8080_clear_flag_bit(cpu, CARRY_FLAG);
	i8080_clear_flag_bit(cpu, AUX_CARRY_FLAG);
	i8080_set_SZP(cpu, logical_xor);

	return logical_xor;
}

uint8_t i8080_logic_or(i8080 *cpu, uint8_t val1, uint8_t val2) {
	uint8_t logical_or = val1 | val2;

	i8080_clear_flag_bit(cpu, CARRY_FLAG);
	i8080_clear_flag_bit(cpu, AUX_CARRY_FLAG);
	i8080_set_SZP(cpu, logical_or);

	return logical_or;
}

uint8_t i8080_rotate_left(i8080 *cpu, uint8_t val, bool through_carry) {
	uint8_t shift_left = val << 1;

	uint8_t bit0 = through_carry ? i8080_get_flag_bit(cpu, CARRY_FLAG) : MSB(val);

	uint8_t rotate_left = shift_left | bit0;

	// Carry bit is set to the high-order bit of the value
	MSB(val) ? i8080_set_flag_bit(cpu, CARRY_FLAG) : i8080_clear_flag_bit(cpu, CARRY_FLAG);

	return rotate_left;
}

uint8_t i8080_rotate_right(i8080 *cpu, uint8_t val, bool through_carry) {
	uint8_t shift_right = val >> 1;

	uint8_t bit7 = through_carry ? (i8080_get_flag_bit(cpu, CARRY_FLAG) << 7) : (LSB(val) << 7);

	uint8_t rotate_right = shift_right | bit7;

	// Carry bit is set to the low-order bit of the value
	LSB(val) ? i8080_set_flag_bit(cpu, CARRY_FLAG) : i8080_clear_flag_bit(cpu, CARRY_FLAG);

	return rotate_right;
}

// Complement carry
int i8080_cmc(i8080 *cpu) {
	i8080_get_flag_bit(cpu, CARRY_FLAG) ? i8080_clear_flag_bit(cpu, CARRY_FLAG) : i8080_set_flag_bit(cpu, CARRY_FLAG);

	return 4;
}

// Set carry bit
int i8080_stc(i8080 *cpu) {
	i8080_set_flag_bit(cpu, CARRY_FLAG);

	return 4;
}

// Increment register or memory
int i8080_inr(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = (opcode & 0x38) >> 3; // Bits 3-5 identify register
	uint8_t sum = i8080_get_register(cpu, reg_no) + 1;

	i8080_set_AC_add(cpu, i8080_get_register(cpu, reg_no), 1, 0);
	i8080_set_SZP(cpu, sum);

	i8080_set_register(cpu, reg_no, sum);

	return (reg_no == 6) ? 10 : 5;
}

// Decrement register or memory
int i8080_dcr(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = (opcode & 0x38) >> 3; // Bits 3-5 identify register
	uint8_t diff = i8080_get_register(cpu, reg_no) - 1;

	i8080_set_AC_sub(cpu, i8080_get_register(cpu, reg_no), 1, 0);
	i8080_set_SZP(cpu, diff);

	i8080_set_register(cpu, reg_no, diff);

	return (reg_no == 6) ? 10 : 5;
}

// Complement accumulator
int i8080_cma(i8080 *cpu) {
	cpu->A = (~cpu->A) & 0xFF;

	return 4;
}

// Decimal adjust accumulator
int i8080_daa(i8080 *cpu) {
	uint8_t temp_A = cpu->A;

	if (i8080_get_flag_bit(cpu, AUX_CARRY_FLAG) || LOW_NIBBLE(temp_A) > 0x09) {
		i8080_set_AC_add(cpu, temp_A, 0x06, 0);
		// Carry condition bit remains unaffected if carry out does not occur
		if ((temp_A + 0x06) & 0x100) i8080_set_flag_bit(cpu, CARRY_FLAG);
		temp_A += 0x06;
	}

	if (i8080_get_flag_bit(cpu, CARRY_FLAG) || HIGH_NIBBLE(temp_A) > 0x09) {
		// Carry condition bit remains unaffected if carry out does not occur
		if ((temp_A + 0x60) & 0x100) i8080_set_flag_bit(cpu, CARRY_FLAG);
		temp_A += 0x60;
	}

	i8080_set_SZP(cpu, temp_A);
	cpu->A = temp_A;

	return 4;
}

// No operation
int i8080_nop() {
	return 4;
}

// Move contents of source register to destination register
int i8080_mov(i8080 *cpu, uint8_t opcode) {
	uint8_t src_reg_no = opcode & 0x07; // Bits 0-2 identify source register
	uint8_t dst_reg_no = (opcode & 0x38) >> 3; // Bits 3-5 identify destination register

	i8080_set_register(cpu, dst_reg_no, i8080_get_register(cpu, src_reg_no));

	return (src_reg_no == 6 || dst_reg_no == 6) ?  7 : 5;
}

// Store accumulator in memory
int i8080_stax(i8080 *cpu, uint8_t opcode) {
	uint8_t pair_no = (opcode & 0x30) >> 4; // Bits 4-5 identify register pair
	uint16_t addr = i8080_get_register_pair(cpu, pair_no); // Register pair contains memory address
	i8080_mem_write_byte(cpu, addr, cpu->A);

	return 7;
}

// Load accumulator from memory
int i8080_ldax(i8080 *cpu, uint8_t opcode) {
	uint8_t pair_no = (opcode & 0x30) >> 4; // Bits 4-5 identify register pair
	uint16_t addr = i8080_get_register_pair(cpu, pair_no); // Register pair contains memory address
	cpu->A = i8080_mem_read_byte(cpu, addr);

	return 7;
}

// Add register or memory to accumulator
int i8080_add(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x7; // Bits 0-2 identify register
	cpu->A = i8080_general_add(cpu, cpu->A, i8080_get_register(cpu, reg_no), 0);

	return (reg_no == 6) ? 7 : 4;
}

// Add register or memory to accumulator with carry
int i8080_adc(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x7; // Bits 0-2 identify register
	cpu->A = i8080_general_add(cpu, cpu->A, i8080_get_register(cpu, reg_no), i8080_get_flag_bit(cpu, CARRY_FLAG));

	return (reg_no == 6) ? 7 : 4;
}

// Subtract register or memory from accumulator
int i8080_sub(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x7; // Bits 0-2 identify register
	cpu->A = i8080_general_sub(cpu, cpu->A, i8080_get_register(cpu, reg_no), 0);

	return (reg_no == 6) ? 7 : 4;
}

// Subtract register or memory from accumulator with borrow
int i8080_sbb(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x7; // Bits 0-2 identify register
	cpu->A = i8080_general_sub(cpu, cpu->A, i8080_get_register(cpu, reg_no), i8080_get_flag_bit(cpu, CARRY_FLAG));

	return (reg_no == 6) ? 7 : 4;
}

// Logical and register or memory with accumulator
int i8080_ana(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x07; // Bits 0-2 identify register
	cpu->A = i8080_logic_and(cpu, cpu->A, i8080_get_register(cpu, reg_no));

	return (reg_no == 6) ? 7 : 4;
}

// Logical exclusive-or register or memory with accumulator
int i8080_xra(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x07; // Bits 0-2 identify register
	cpu->A = i8080_logic_xor(cpu, cpu->A, i8080_get_register(cpu, reg_no));

	return (reg_no == 6) ? 7 : 4;
}

// Logical or register or memory with accumulator
int i8080_ora(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x07; // Bits 0-2 identify register
	cpu->A = i8080_logic_or(cpu, cpu->A, i8080_get_register(cpu, reg_no));

	return (reg_no == 6) ? 7 : 4;
}

// Compare register or memory with accumulator
int i8080_cmp(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = opcode & 0x07; // Bits 0-2 identify register
	i8080_general_sub(cpu, cpu->A, i8080_get_register(cpu, reg_no), 0);

	return (reg_no == 6) ? 7 : 4;
}

// Rotate accumulator left
int i8080_rlc(i8080 *cpu) {
	cpu->A = i8080_rotate_left(cpu, cpu->A, false);

	return 4;
}

// Rotate accumulator right
int i8080_rrc(i8080 *cpu) {
	cpu->A = i8080_rotate_right(cpu, cpu->A, false);

	return 4;
}

// Rotate accumulator left through carry
int i8080_ral(i8080 *cpu) {
	cpu->A = i8080_rotate_left(cpu, cpu->A, true);

	return 4;
}

// Rotate accumulator right through carry
int i8080_rar(i8080 *cpu) {
	cpu->A = i8080_rotate_right(cpu, cpu->A, true);

	return 4;
}

// Push data onto stack
int i8080_push(i8080 *cpu, uint8_t opcode) {
	int pair_no = (opcode & 0x30) >> 4; // Bits 4-5 identify register pair
	uint16_t data = (pair_no == 3) ? i8080_get_PSW(cpu) : i8080_get_register_pair(cpu, pair_no); // Bits 4-5 set to 11 refer to PSW
	i8080_push_stack_word(cpu, data);

	return 11;
}

// Pop data off stack
int i8080_pop(i8080 *cpu, uint8_t opcode) {
	int pair_no = (opcode & 0x30) >> 4;  // Bits 4-5 identify register pair
	uint16_t data = i8080_pop_stack_word(cpu);
	(pair_no == 3) ? i8080_set_PSW(cpu, data) : i8080_set_register_pair(cpu, pair_no, data); // Bits 4-5 set to 11 refer to PSW

	return 10;
}

// Double add
int i8080_dad(i8080 *cpu, uint8_t opcode) {
	uint8_t pair_no = (opcode & 0x30) >> 4; // Bits 4-5 identify register pair

	uint32_t sum32 = PAIR(cpu->H, cpu->L) + i8080_get_register_pair(cpu, pair_no);
	uint16_t sum16 = sum32 & 0xFFFF;

	(sum32 & 0x10000) ? i8080_set_flag_bit(cpu, CARRY_FLAG) : i8080_clear_flag_bit(cpu, CARRY_FLAG);

	cpu->H = HIGH_BYTE(sum16);
	cpu->L = LOW_BYTE(sum16);

	return 10;
}

// Increment register pair
int i8080_inx(i8080 *cpu, uint8_t opcode) {
	uint8_t pair_no = (opcode & 0x30) >> 4; // Bits 4-5 identify register pair
	i8080_set_register_pair(cpu, pair_no, i8080_get_register_pair(cpu, pair_no) + 1);

	return 5;
}

// Decrement register pair
int i8080_dcx(i8080 *cpu, uint8_t opcode) {
	uint8_t pair_no = (opcode & 0x30) >> 4;  // Bits 4-5 identify register pair
	i8080_set_register_pair(cpu, pair_no, i8080_get_register_pair(cpu, pair_no) - 1);

	return 5;
}

// Exchange registers
int i8080_xchg(i8080 *cpu) {
	uint8_t temp_H = cpu->H;
	uint8_t temp_L = cpu->L;

	cpu->H = cpu->D; cpu->D = temp_H;
	cpu->L = cpu->E; cpu->E = temp_L;

	return 5;
}

// Exchange stack
int i8080_xthl(i8080 *cpu) {
	uint8_t temp_H = cpu->H;
	uint8_t temp_L = cpu->L;

	cpu->L = i8080_pop_stack_byte(cpu); cpu->H = i8080_pop_stack_byte(cpu);
	i8080_push_stack_byte(cpu, temp_H);
	i8080_push_stack_byte(cpu, temp_L);

	return 18;
}

// Load SP from H and L
int i8080_sphl(i8080 *cpu) {
	cpu->SP = PAIR(cpu->H, cpu->L);

	return 5;
}

// Load register pair immediate
int i8080_lxi(i8080 *cpu, uint8_t opcode) {
	uint8_t pair_no = (opcode & 0x30) >> 4; // Bits 4-5 identify register pair
	i8080_set_register_pair(cpu, pair_no, i8080_get_next_word(cpu));

	return 10;
}

// Move immediate data
int i8080_mvi(i8080 *cpu, uint8_t opcode) {
	uint8_t reg_no = (opcode & 0x38) >> 3; // Bits 3-5 identify register
	i8080_set_register(cpu, reg_no, i8080_get_next_byte(cpu));

	return (reg_no == 6) ? 10 : 7;
}

// Add immediate to accumulator
int i8080_adi(i8080 *cpu) {
	cpu->A = i8080_general_add(cpu, cpu->A, i8080_get_next_byte(cpu), 0);

	return 7;
}

// Add immediate to accumulator with carry
int i8080_aci(i8080 *cpu) {
	cpu->A = i8080_general_add(cpu, cpu->A, i8080_get_next_byte(cpu), i8080_get_flag_bit(cpu, CARRY_FLAG));

	return 7;
}

// Subtract immediate from accumulator
int i8080_sui(i8080 *cpu) {
	cpu->A = i8080_general_sub(cpu, cpu->A, i8080_get_next_byte(cpu), 0);

	return 7;
}

// Subtract immediate from accumulator with borrow
int i8080_sbi(i8080 *cpu) {
	cpu->A = i8080_general_sub(cpu, cpu->A, i8080_get_next_byte(cpu), i8080_get_flag_bit(cpu, CARRY_FLAG));

	return 7;
}

// And immediate with accumulator
int i8080_ani(i8080 *cpu) {
	cpu->A = i8080_logic_and(cpu, cpu->A, i8080_get_next_byte(cpu));

	return 7;
}

// Exclusive-or immediate with accumulator
int i8080_xri(i8080 *cpu) {
	cpu->A = i8080_logic_xor(cpu, cpu->A, i8080_get_next_byte(cpu));

	return 7;
}

// Or immediate with accumulator
int i8080_ori(i8080 *cpu) {
	cpu->A = i8080_logic_or(cpu, cpu->A, i8080_get_next_byte(cpu));

	return 7;
}

// Compare immediate with accumulator
int i8080_cpi(i8080 *cpu) {
	i8080_general_sub(cpu, cpu->A, i8080_get_next_byte(cpu), 0);

	return 7;
}

// Store accumulator direct
int i8080_sta(i8080 *cpu) {
	uint16_t addr = i8080_get_next_word(cpu);
	i8080_mem_write_byte(cpu, addr, cpu->A);

	return 13;
}

// Load accumulator direct
int i8080_lda(i8080 *cpu) {
	uint16_t addr = i8080_get_next_word(cpu);
	cpu->A = i8080_mem_read_byte(cpu, addr);

	return 13;
}

// Store H and L direct
int i8080_shld(i8080 *cpu) {
	uint16_t addr = i8080_get_next_word(cpu);

	i8080_mem_write_byte(cpu, addr, cpu->L);
	i8080_mem_write_byte(cpu, addr + 1, cpu->H);

	return 16;
}

// Load H and L direct
int i8080_lhld(i8080 *cpu) {
	uint16_t addr = i8080_get_next_word(cpu);

	cpu->L = i8080_mem_read_byte(cpu, addr);
	cpu->H = i8080_mem_read_byte(cpu, addr + 1);

	return 16;
}

// Load program counter
int i8080_pchl(i8080 *cpu) {
	cpu->PC = PAIR(cpu->H, cpu->L);

	return 5;
}

int i8080_jump(i8080 *cpu, uint8_t opcode) {
	/*
	 * Least significant bit set for unconditional jump, 0 otherwise
	 * Bits 3-5 identify jump condition
	 */
	uint16_t addr = i8080_get_next_word(cpu);
	if ((opcode & 0x01) || i8080_check_condition(cpu, (opcode & 0x38) >> 3)) {
		cpu->PC = addr;
	}

	return 10;
}

int i8080_call(i8080 *cpu, uint8_t opcode) {
	/*
	 * Least significant bit set for unconditional call, 0 otherwise
	 * Bits 3-5 identify call condition
	 */
	uint16_t addr = i8080_get_next_word(cpu);
	if ((opcode & 0x01) || i8080_check_condition(cpu, (opcode & 0x38) >> 3)) {
		i8080_push_stack_word(cpu, cpu->PC);
		cpu->PC = addr;
		return 17;
	}

	return 11;
}

int i8080_return(i8080 *cpu, uint8_t opcode) {
	/*
	 * Least significant bit set for unconditional return, 0 otherwise
	 * Bits 3-5 identify return condition
	 */
	if ((opcode & 0x01) || i8080_check_condition(cpu, (opcode & 0x38) >> 3)) {
		cpu->PC = i8080_pop_stack_word(cpu);
		return 11;
	}

	return 5;
}

// Restart
int i8080_rst(i8080 *cpu, uint8_t opcode) {
	i8080_push_stack_word(cpu, cpu->PC);
	cpu->PC = opcode & 0x38;

	return 11;
}

// Enable interrupts
int i8080_ei(i8080 *cpu) {
	cpu->inte = true;

	return 4;
}

// Disable interrupts
int i8080_di(i8080 *cpu) {
	cpu->inte = false;

	return 4;
}

// Input
int i8080_in(i8080 *cpu) {
	uint8_t dev = i8080_get_next_byte(cpu);
	if (cpu->input_handler && cpu->user_data) {
		cpu->A = cpu->input_handler(cpu->user_data, dev);
	}

	return 10;
}

// Output
int i8080_out(i8080 *cpu) {
	uint8_t dev = i8080_get_next_byte(cpu);
	if (cpu->input_handler && cpu->user_data) {
		cpu->output_handler(cpu->user_data, dev, cpu->A);
	}

	return 10;
}

// Halt
int i8080_hlt(i8080 *cpu) {
	cpu->halt = true;

	return 7;
}

int i8080_step(i8080 *cpu, uint8_t opcode) {
	if (cpu->halt) {
		return 0;
	}

	switch (opcode) {
		case 0x40: // MOV B,B
		case 0x41: // MOV B,C
		case 0x42: // MOV B,D
		case 0x43: // MOV B,E
		case 0x44: // MOV B,H
		case 0x45: // MOV B,L
		case 0x46: // MOV B,M
		case 0x47: // MOV B,A
		case 0x48: // MOV C,B
		case 0x49: // MOV C,C
		case 0x4A: // MOV C,D
		case 0x4B: // MOV C,E
		case 0x4C: // MOV C,H
		case 0x4D: // MOV C,L
		case 0x4E: // MOV C,M
		case 0x4F: // MOV C,A
		case 0x50: // MOV D,B
		case 0x51: // MOV D,C
		case 0x52: // MOV D,D
		case 0x53: // MOV D,E
		case 0x54: // MOV D,H
		case 0x55: // MOV D,L
		case 0x56: // MOV D,M
		case 0x57: // MOV D,A
		case 0x58: // MOV E,B
		case 0x59: // MOV E,C
		case 0x5A: // MOV E,D
		case 0x5B: // MOV E,E
		case 0x5C: // MOV E,H
		case 0x5D: // MOV E,L
		case 0x5E: // MOV E,M
		case 0x5F: // MOV E,A
		case 0x60: // MOV H,B
		case 0x61: // MOV H,C
		case 0x62: // MOV H,D
		case 0x63: // MOV H,E
		case 0x64: // MOV H,H
		case 0x65: // MOV H,L
		case 0x66: // MOV H,M
		case 0x67: // MOV H,A
		case 0x68: // MOV L,B
		case 0x69: // MOV L,C
		case 0x6A: // MOV L,D
		case 0x6B: // MOV L,E
		case 0x6C: // MOV L,H
		case 0x6D: // MOV L,L
		case 0x6E: // MOV L,M
		case 0x6F: // MOV L,A
		case 0x70: // MOV M,B
		case 0x71: // MOV M,C
		case 0x72: // MOV M,D
		case 0x73: // MOV M,E
		case 0x74: // MOV M,H
		case 0x75: // MOV M,L
		case 0x77: // MOV M,A
		case 0x78: // MOV A,B
		case 0x79: // MOV A,C
		case 0x7A: // MOV A,D
		case 0x7B: // MOV A,E
		case 0x7C: // MOV A,H
		case 0x7D: // MOV A,L
		case 0x7E: // MOV A,M
		case 0x7F: // MOV A,A
			return i8080_mov(cpu, opcode);

		case 0x06: // MVI B,d8
		case 0x0E: // MVI C,d8
		case 0x16: // MVI D,d8
		case 0x1E: // MVI E,d8
		case 0x26: // MVI H,d8
		case 0x2E: // MVI L,d8
		case 0x36: // MVI M,d8
		case 0x3E: // MVI A,d8
			return i8080_mvi(cpu, opcode);

		case 0x01: // LXI B,d16
		case 0x11: // LXI D,d16
		case 0x21: // LXI H,d16
		case 0x31: // LXI SP,d16
			return i8080_lxi(cpu, opcode);

		case 0x32: // STA a16
			return i8080_sta(cpu);

		case 0x3A: // LDA a16
			return i8080_lda(cpu);

		case 0x22: // SHLD a16
			return i8080_shld(cpu);

		case 0x2A: // LHLD a16
			return i8080_lhld(cpu);

		case 0x02: // STAX B
		case 0x12: // STAX D
			return i8080_stax(cpu, opcode);

		case 0x0A: // LDAX B
		case 0x1A: // LDAX D
			return i8080_ldax(cpu, opcode);

		case 0xEB: // XCHG
			return i8080_xchg(cpu);

		case 0x80: // ADD B
		case 0x81: // ADD C
		case 0x82: // ADD D
		case 0x83: // ADD E
		case 0x84: // ADD H
		case 0x85: // ADD L
		case 0x86: // ADD M
		case 0x87: // ADD A
			return i8080_add(cpu, opcode);

		case 0xC6: // ADI d8
			return i8080_adi(cpu);

		case 0x88: // ADC B
		case 0x89: // ADC C
		case 0x8A: // ADC D
		case 0x8B: // ADC E
		case 0x8C: // ADC H
		case 0x8D: // ADC L
		case 0x8E: // ADC M
		case 0x8F: // ADC A
			return i8080_adc(cpu, opcode);

		case 0xCE: // ACI d8
			return i8080_aci(cpu);

		case 0x90: // SUB B
		case 0x91: // SUB C
		case 0x92: // SUB D
		case 0x93: // SUB E
		case 0x94: // SUB H
		case 0x95: // SUB L
		case 0x96: // SUB M
		case 0x97: // SUB A
			return i8080_sub(cpu, opcode);

		case 0xD6: // SUI d8
			return i8080_sui(cpu);

		case 0x98: // SBB B
		case 0x99: // SBB C
		case 0x9A: // SBB D
		case 0x9B: // SBB E
		case 0x9C: // SBB H
		case 0x9D: // SBB L
		case 0x9E: // SBB M
		case 0x9F: // SBB A
			return i8080_sbb(cpu, opcode);

		case 0xDE:// SBI d8
			return i8080_sbi(cpu);

		case 0x04: // INR B
		case 0x14: // INR D
		case 0x24: // INR H
		case 0x34: // INR M
		case 0x0C: // INR C
		case 0x1C: // INR E
		case 0x2C: // INR L
		case 0x3C: // INR A
			return i8080_inr(cpu, opcode);

		case 0x05: // DCR B
		case 0x15: // DCR D
		case 0x25: // DCR H
		case 0x35: // DCR M
		case 0x0D: // DCR C
		case 0x1D: // DCR E
		case 0x2D: // DCR L
		case 0x3D: // DCR A
			return i8080_dcr(cpu, opcode);

		case 0x03: // INX B
		case 0x13: // INX D
		case 0x23: // INX H
		case 0x33: // INX SP
			return i8080_inx(cpu, opcode);

		case 0x0B: // DCX B
		case 0x1B: // DCX D
		case 0x2B: // DCX H
		case 0x3B: // DCX SP
			return i8080_dcx(cpu, opcode);

		case 0x09: // DAD B
		case 0x19: // DAD D
		case 0x29: // DAD H
		case 0x39: // DAD SP
			return i8080_dad(cpu, opcode);

		case 0x27: // DAA
			return i8080_daa(cpu);

		case 0xA0: // ANA B
		case 0xA1: // ANA C
		case 0xA2: // ANA D
		case 0xA3: // ANA E
		case 0xA4: // ANA H
		case 0xA5: // ANA L
		case 0xA6: // ANA M
		case 0xA7: // ANA A
			return i8080_ana(cpu, opcode);

		case 0xE6: // ANI d8
			return i8080_ani(cpu);

		case 0xA8: // XRA B
		case 0xA9: // XRA C
		case 0xAA: // XRA D
		case 0xAB: // XRA E
		case 0xAC: // XRA H
		case 0xAD: // XRA L
		case 0xAE: // XRA M
		case 0xAF: // XRA A
			return i8080_xra(cpu, opcode);

		case 0xEE: // XRI d8
			return i8080_xri(cpu);

		case 0xB0: // ORA B
		case 0xB1: // ORA C
		case 0xB2: // ORA D
		case 0xB3: // ORA E
		case 0xB4: // ORA H
		case 0xB5: // ORA L
		case 0xB6: // ORA M
		case 0xB7: // ORA A
			return i8080_ora(cpu, opcode);

		case 0xF6: // ORI d8
			return i8080_ori(cpu);

		case 0xB8: // CMP B
		case 0xB9: // CMP C
		case 0xBA: // CMP D
		case 0xBB: // CMP E
		case 0xBC: // CMP H
		case 0xBD: // CMP L
		case 0xBE: // CMP M
		case 0xBF: // CMP A
			return i8080_cmp(cpu, opcode);

		case 0xFE: // CPI d8
			return i8080_cpi(cpu);

		case 0x07: // RLC
			return i8080_rlc(cpu);

		case 0xF: // RRC
			return i8080_rrc(cpu);

		case 0x17: // RAL
			return i8080_ral(cpu);

		case 0x1F: // RAR
			return i8080_rar(cpu);

		case 0x2F: // CMA
			return i8080_cma(cpu);

		case 0x3F: // CMC
			return i8080_cmc(cpu);

		case 0x37: // STC
			return i8080_stc(cpu);

		case 0xC3: // JMP a16
		case 0xCB: // JMP 16 (alternate)
		case 0xC2: // JNZ a16
		case 0xCA: // JZ a16
		case 0xD2: // JNC a16
		case 0xDA: // JC a16
		case 0xE2: // JPO a16
		case 0xEA: // JPE a16
		case 0xF2: // JP a16
		case 0xFA: // JM a16
			return i8080_jump(cpu, opcode);

		case 0xCD: // CALL a16
		case 0xDD: // CALL a16 (alternate)
		case 0xED: // CALL a16 (alternate)
		case 0xFD: // CALL a16 (alternate)
		case 0xC4: // CNZ a16
		case 0xCC: // CZ a16
		case 0xD4: // CNC a16
		case 0xDC: // CC a16
		case 0xE4: // CPO a16
		case 0xEC: // CPE a16
		case 0xF4: // CP a16
		case 0xFC: // CM a16
			return i8080_call(cpu, opcode);

		case 0xC9: // RET
		case 0xD9: // RET (alternate)
		case 0xC0: // RNZ
		case 0xC8: // RZ
		case 0xD0: // RNC
		case 0xD8: // RC
		case 0xE0: // RPO
		case 0xE8: // RPE
		case 0xF0: // RP
		case 0xF8: // RM
			return i8080_return(cpu, opcode);

		case 0xC7: // RST 0
		case 0xCF: // RST 1
		case 0xD7: // RST 2
		case 0xDF: // RST 3
		case 0xE7: // RST 4
		case 0xEF: // RST 5
		case 0xF7: // RST 6
		case 0xFF: // RST 7
			return i8080_rst(cpu, opcode);

		case 0xE9: // PCHL
			return i8080_pchl(cpu);

		case 0xC1: // POP B
		case 0xD1: // POP D
		case 0xE1: // POP H
		case 0xF1: // POP PSW
			return i8080_pop(cpu, opcode);

		case 0xC5: // PUSH B
		case 0xD5: // PUSH D
		case 0xE5: // PUSH H
		case 0xF5: // PUSH PSW
			return i8080_push(cpu, opcode);

		case 0xE3: // XTHL
			return i8080_xthl(cpu);

		case 0xF9: // SPHL
			return i8080_sphl(cpu);

		case 0xD3: // OUT d8
			return i8080_out(cpu);

		case 0xDB: // IN d8
			return i8080_in(cpu);

		case 0xF3: // DI
			return i8080_di(cpu);

		case 0xFB: // EI
			return i8080_ei(cpu);

		case 0x76: // HLT
			return i8080_hlt(cpu);

		case 0x00: // NOP
		case 0x08: // NOP (alternate)
		case 0x10: // NOP (alternate)
		case 0x18: // NOP (alternate)
		case 0x20: // NOP (alternate)
		case 0x28: // NOP (alternate)
		case 0x30: // NOP (alternate)
		case 0x38: // NOP (alternate)
			return i8080_nop();

		default:
			fprintf(stderr, "Invalid opcode\n");
			exit(1);
	}
}
