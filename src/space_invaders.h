#ifndef INVADERS_H
#define INVADERS_H

#include "i8080.h"
#include "display.h"
#include "audio.h"
#include "io_handler.h"

typedef struct space_invaders {
	i8080 *cpu;
	si_display *display;
	si_audio *audio;
	si_io_handler *io;

	uint8_t interrupt_opcode;
} space_invaders;

space_invaders *si_machine_init();

void si_load_game_rom(space_invaders *si);

void si_run_game_loop(space_invaders *si);

#endif //INVADERS_H
