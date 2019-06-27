#ifndef IO_HANDLER_H
#define IO_HANDLER_H

#include <stdbool.h>
#include "SDL.h"

typedef struct si_io_handler {
	SDL_Event event;
	bool quit;

	uint8_t port_1;
	uint8_t port_2;

	uint8_t shift_data_high;
	uint8_t shift_data_low;
	uint8_t shift_amount;
} si_io_handler;

si_io_handler *si_io_handler_init();
void si_io_handler_quit(si_io_handler *io);

void si_handle_events(si_io_handler *io);

uint8_t si_handle_in_instr(void *user_data, uint8_t port);
void si_handle_out_instr(void *user_data, uint8_t port, uint8_t val);

#endif //IO_HANDLER_H
