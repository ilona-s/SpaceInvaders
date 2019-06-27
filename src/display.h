#ifndef DISPLAY_H
#define DISPLAY_H

#include "SDL.h"
#include "i8080.h"

#define SCREEN_HEIGHT 256
#define SCREEN_WIDTH 224

typedef struct si_display {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;

	uint32_t frame_buffer[SCREEN_HEIGHT * SCREEN_WIDTH];
} si_display;

si_display *si_display_init();
void si_display_quit(si_display *display);

void si_load_frame_buffer(si_display *display, i8080 *cpu);
void si_render_frame(si_display *display);

#endif //DISPLAY_H
