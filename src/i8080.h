#ifndef I8080_H
#define I8080_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE 0x10000

typedef enum flag_bit {
	CARRY_FLAG, FLAG_1, PARITY_FLAG, FLAG_3,
	AUX_CARRY_FLAG, FLAG_5, ZERO_FLAG, SIGN_FLAG
} flag_bit;

typedef struct i8080 {
	uint16_t PC;
	uint16_t SP;

	uint8_t memory[MEM_SIZE];

	// Accumulator register and flags register
	uint8_t A; uint8_t F;

	// General-purpose registers
	uint8_t B; uint8_t C;
	uint8_t D; uint8_t E;
	uint8_t H; uint8_t L;

	bool inte;
	bool halt;

	bool interrupt_occurred;
	uint8_t interrupt_opcode;

	uint8_t (*input_handler)(void *, uint8_t);
	void (*output_handler)(void *, uint8_t, uint8_t);
	void *user_data;
} i8080;

i8080 *i8080_init();
void i8080_quit(i8080 *cpu);

void i8080_load_ROM(i8080 *cpu, char *path, int offset);

void i8080_generate_interrupt(i8080 *cpu, uint8_t opcode);

uint8_t i8080_mem_read_byte(i8080 *cpu, uint16_t addr);

uint8_t i8080_get_opcode(i8080 *cpu);

int i8080_step(i8080 *cpu, uint8_t opcode);

#endif //I8080_H
