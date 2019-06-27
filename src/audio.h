#ifndef AUDIO_H
#define AUDIO_H

#include "SDL_mixer.h"

#define NUM_SOUNDS 9

typedef enum sound_effect {
	BASE_HIT, INV_HIT, SHOT,
	WALK1, WALK2, WALK3,
	WALK4, UFO, UFO_HIT
} sound_effect;

typedef struct si_audio {
	Mix_Chunk *sounds[NUM_SOUNDS];

	uint8_t prev_port_3;
	uint8_t prev_port_5;
} si_audio;

si_audio *si_audio_init();
void si_audio_quit(si_audio *audio);

void si_handle_port_3_sound(si_audio *audio, uint8_t curr_port_3);
void si_handle_port_5_sound(si_audio *audio, uint8_t curr_port_5);

#endif //AUDIO_H
