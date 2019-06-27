#include "display.h"

#define VRAM_START_ADDR 0x2400

si_display *si_display_init() {
	si_display *display = malloc(sizeof(si_display));
	if (!display) {
		perror("malloc");
		exit(1);
	}

	display->window = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                   SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
	if (!display->window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		exit(1);
	}

	display->renderer = SDL_CreateRenderer(display->window, -1, 0);
	if (!display->renderer) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		exit(1);
	}

	display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
	                                     SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!display->texture) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		exit(1);
	}

	memset(display->frame_buffer, 0, sizeof(display->frame_buffer));

	return display;
}

void si_display_quit(si_display *display) {
	SDL_DestroyTexture(display->texture);
	SDL_DestroyRenderer(display->renderer);
	SDL_DestroyWindow(display->window);
	free(display);
}

void si_set_pixel_colour(int x, int y, uint32_t *rgba) {
	// Use coordinates to determine pixel colour
	if (32 <= y && y < 64) { // Red
		*rgba = 0xFF000000;
	} else if ((184 <= y && y < 240) || (240 <= y && (16 <= x && x < 134))) { // Green
		*rgba = 0x00FF0000;
	} else { // White
		*rgba = 0xFFFFFF00;
	}
	*rgba |= 0x000000FF;
}

void si_clear_pixel_colour(uint32_t *rgba) {
	*rgba = 0x000000FF;
}

void si_load_frame_buffer(si_display *display, i8080 *cpu) {
	int num_bytes = (SCREEN_WIDTH * SCREEN_HEIGHT) / 8;

	for (int i = 0; i < num_bytes; i++) {
		// Each byte represents 8 pixels
		uint8_t byte = i8080_mem_read_byte(cpu, VRAM_START_ADDR + i);
		for (int j = 0; j < 8; j++) {
			// Each bit represents a pixel
			uint8_t bit_mask = 0x01 << j;
			uint8_t bit = byte & bit_mask;

			int x = (i * 8) % SCREEN_HEIGHT + j;
			int y = (i * 8) / SCREEN_HEIGHT;

			// Screen is rotated 90 degrees counterclockwise
			int rotate_x = y;
			int rotate_y = SCREEN_HEIGHT - x - 1;

			int index = rotate_y * SCREEN_WIDTH + rotate_x;
			uint32_t *pixel = &display->frame_buffer[index];

			if (bit) {
				si_set_pixel_colour(rotate_x, rotate_y, pixel);
			} else {
				si_clear_pixel_colour(pixel);
			}
		}
	}
}

void si_render_frame(si_display *display) {
	SDL_UpdateTexture(display->texture, NULL, display->frame_buffer, SCREEN_WIDTH * sizeof(uint32_t));
	SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
	SDL_RenderPresent(display->renderer);
}
