#include "io_handler.h"
#include "space_invaders.h"

si_io_handler *si_io_handler_init() {
	si_io_handler *io = malloc(sizeof(si_io_handler));
	if (!io) {
		perror("malloc");
		exit(1);
	}

	io->quit = false;

	io->port_1 = 8; // Bit 3 always high
	io->port_2 = 0;

	io->shift_data_high = 0;
	io->shift_data_low = 0;
	io->shift_amount = 0;

	return io;
}

void si_io_handler_quit(si_io_handler *io) {
	free(io);
}

uint8_t si_handle_in_instr(void *user_data, uint8_t port) {
	space_invaders *si = (space_invaders *) user_data;

	switch(port) {
		case 0:
			// Port 0
		case 1:
			return si->io->port_1;
		case 2:
			return si->io->port_2;
		case 3:
			return ((si->io->shift_data_high << 8 | si->io->shift_data_low) << si->io->shift_amount) >> 8;
		default:
			fprintf(stderr, "Invalid input port %d\n", port);
			exit(1);
	}
}

void si_handle_out_instr(void *user_data, uint8_t port, uint8_t val) {
	space_invaders *si = (space_invaders *) user_data;

	switch (port) {
		case 2:
			// Bits 0-2 specify shift amount for 16-bit shift register
			si->io->shift_amount = val & 0x07;
			break;
		case 3:
			si_handle_port_3_sound(si->audio, val);
			break;
		case 4:
			// Write to 16-bit shift register, LSB on first write, MSB on second write
			si->io->shift_data_low = si->io->shift_data_high;
			si->io->shift_data_high = val;
			break;
		case 5:
			si_handle_port_5_sound(si->audio, val);
			break;
		case 6:
			// Watchdog timer
			break;
		default:
			fprintf(stderr, "Invalid output port %d\n", port);
			exit(1);
	}
}

void si_handle_key_up(si_io_handler *io, SDL_Keycode key) {
	switch (key) {
		case SDLK_c: // Coin
			io->port_1 &= (~0x01) & 0xFF;
			break;
		case SDLK_2: // Two player
			io->port_1 &= (~0x02) & 0xFF;
			break;
		case SDLK_1: // One player
			io->port_1 &= (~0x04) & 0xFF;
			break;
		case SDLK_SPACE: // Shoot
			io->port_1 &= (~0x10) & 0xFF;
			io->port_2 &= (~0x10) & 0xFF;
			break;
		case SDLK_LEFT: // Move left
			io->port_1 &= (~0x20) & 0xFF;
			io->port_2 &= (~0x20) & 0xFF;
			break;
		case SDLK_RIGHT: // Move right
			io->port_1 &= (~0x40) & 0xFF;
			io->port_2 &= (~0x40) & 0xFF;
			break;
		default:
			break;
	}
}

void si_handle_key_down(si_io_handler *io, SDL_Keycode key) {
	switch (key) {
		case SDLK_c: // Coin
			io->port_1 |= 0x01;
			break;
		case SDLK_2: // Two player
			io->port_1 |= 0x02;
			break;
		case SDLK_1: // One player
			io->port_1|= 0x04;
			break;
		case SDLK_SPACE: // Shoot
			io->port_1 |= 0x10;
			io->port_2 |= 0x10;
			break;
		case SDLK_LEFT: // Move left
			io->port_1 |= 0x20;
			io->port_2 |= 0x20;
			break;
		case SDLK_RIGHT: // Move right
			io->port_1 |= 0x40;
			io->port_2 |= 0x40;
			break;
		default:
			break;
	}
}

void si_handle_events(si_io_handler *io) {
	while (SDL_PollEvent(&io->event)) {
		switch (io->event.type) {
			case (SDL_QUIT):
				io->quit = true;
				break;
			case (SDL_KEYDOWN):
				si_handle_key_down(io, io->event.key.keysym.sym);
				break;
			case (SDL_KEYUP):
				si_handle_key_up(io, io->event.key.keysym.sym);
				break;
			default:
				break;
		}
	}
}
