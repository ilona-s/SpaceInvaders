#include <sys/time.h>
#include <printf.h>

#include "space_invaders.h"

#define INTERRUPT_1 0xCF // RST 1 instruction jumps to address 0x0008
#define INTERRUPT_2 0xD7 // RST 2 instruction jumps to address 0x0010

#define FILE_1 "../res/roms/invaders.h"
#define FILE_2 "../res/roms/invaders.g"
#define FILE_3 "../res/roms/invaders.f"
#define FILE_4 "../res/roms/invaders.e"

space_invaders *si_machine_init() {
	space_invaders *si = malloc(sizeof(space_invaders));
	if (!si) {
		perror("malloc");
		exit(1);
	}

	si->cpu = i8080_init();
	si->cpu->input_handler = si_handle_in_instr;
	si->cpu->output_handler = si_handle_out_instr;
	si->cpu->user_data = si;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	si->display = si_display_init();
	si->audio = si_audio_init();
	si->io = si_io_handler_init();

	si->interrupt_opcode = INTERRUPT_1;

	return si;
}

void si_machine_quit(space_invaders *si) {
	i8080_quit(si->cpu);
	si_display_quit(si->display);
	si_audio_quit(si->audio);
	si_io_handler_quit(si->io);

	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_Quit();

	free(si);
}

void si_load_game_rom(space_invaders *si) {
	i8080_load_ROM(si->cpu, FILE_1, 0x0000);
	i8080_load_ROM(si->cpu, FILE_2, 0x0800);
	i8080_load_ROM(si->cpu, FILE_3, 0x1000);
	i8080_load_ROM(si->cpu, FILE_4, 0x1800);
}

void si_generate_interrupt(space_invaders *si) {
	i8080_generate_interrupt(si->cpu, si->interrupt_opcode);

	// Interrupts alternate
	si->interrupt_opcode = (si->interrupt_opcode == INTERRUPT_1) ? INTERRUPT_2 : INTERRUPT_1;
}

void si_run_cpu_one_frame(space_invaders *si) {
	/*
	 * Approximately 2MHz/60FPS = 33332 cycles per frame
	 * Interrupts occur every 33332 / 2 = 16666 cycles
	 */
	int num_interrupts = 0;
	while (num_interrupts < 2) {
		int cycles_complete = 0;
		while (cycles_complete < 16666) {
			uint8_t opcode = i8080_get_opcode(si->cpu);
			cycles_complete += i8080_step(si->cpu, opcode);
		}
		si_generate_interrupt(si);
		num_interrupts += 1;
	}
}

void si_run_game_loop(space_invaders *si) {
	uint32_t timer = SDL_GetTicks(); // Timer in milliseconds

	while (!si->io->quit) {
		si_handle_events(si->io);

		// Call necessary functions every 1/60 seconds to achieve frame rate of 60FPS
		if (SDL_GetTicks() - timer > ((float) 1 / (float) 60) * 1000) {
			timer = SDL_GetTicks();

			si_run_cpu_one_frame(si);
			si_load_frame_buffer(si->display, si->cpu);
			si_render_frame(si->display);
		}
	}

	si_machine_quit(si);
}