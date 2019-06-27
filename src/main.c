#include "space_invaders.h"

int main(int argc, char **argv) {
	struct space_invaders *si = si_machine_init();
	si_load_game_rom(si);
	si_run_game_loop(si);
	return 0;
}
