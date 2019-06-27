#include "audio.h"

const char *wav_files[9] = {"../res/sounds/BaseHit.wav",
			    "../res/sounds/InvHit.wav",
			    "../res/sounds/Shot.wav",
			    "../res/sounds/Walk1.wav",
			    "../res/sounds/Walk2.wav",
			    "../res/sounds/Walk3.wav",
			    "../res/sounds/Walk4.wav",
			    "../res/sounds/Ufo.wav",
			    "../res/sounds/UfoHit.wav"};

si_audio *si_audio_init() {
	si_audio *audio = malloc(sizeof(struct si_audio));
	if (!audio) {
		perror("malloc");
		exit(1);
	}

	int res = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);
	if (res < 0) {
		fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
		exit(1);
	}

	for (int i = 0; i < NUM_SOUNDS; i++) {
		audio->sounds[i] = Mix_LoadWAV(wav_files[i]);
		if (!audio->sounds[i]) {
			fprintf(stderr, "Mix_LoadWav: %s\n", Mix_GetError());
			exit(1);
		}
	}

	Mix_AllocateChannels(NUM_SOUNDS);

	audio->prev_port_3 = 0;
	audio->prev_port_5 = 0;

	return audio;
}

void si_audio_quit(si_audio *audio) {
	for (int i = 0; i < NUM_SOUNDS; i++) {
		Mix_FreeChunk(audio->sounds[i]);
	}

	Mix_CloseAudio();
	free(audio);
}

void si_play_sound_once(si_audio *audio, sound_effect idx) {
	int channel = Mix_PlayChannel(idx, audio->sounds[idx], 0);
	if (channel < 0) {
		fprintf(stderr, "Mix_PlayChannel: %s", Mix_GetError());
		exit(1);
	}
}

void si_play_sound_loop(si_audio *audio, sound_effect idx) {
	int channel = Mix_PlayChannel(idx, audio->sounds[idx], -1);
	if (channel < 0) {
		fprintf(stderr, "Mix_PlayChannel: %s", Mix_GetError());
		exit(1);
	}
}

void si_stop_sound(sound_effect idx) {
	Mix_HaltChannel(idx);
}

void si_handle_port_3_sound(si_audio *audio, uint8_t curr_port_3) {
	// If sound bit transitioned from low to high, play corresponding sound
	if (curr_port_3 != audio->prev_port_3) {
		if ((curr_port_3 & 0x01) && !(audio->prev_port_3 & 0x01)) {
			// Play UFO sound on loop when bit is high
			si_play_sound_loop(audio, UFO);
		} else if (!(curr_port_3 & 0x01) && (audio->prev_port_3 & 0x01)) {
			// Stop UFO sound when bit is low
			si_stop_sound(UFO);
		} else if ((curr_port_3 & 0x02) && !(audio->prev_port_3 & 0x02)) {
			si_play_sound_once(audio, SHOT);
		} else if ((curr_port_3 & 0x04) && !(audio->prev_port_3 & 0x04)) {
			si_play_sound_once(audio, BASE_HIT);
		} else if ((curr_port_3 & 0x08) && !(audio->prev_port_3 & 0x08)) {
			si_play_sound_once(audio, INV_HIT);
		}


		audio->prev_port_3 = curr_port_3;
	}
}

void si_handle_port_5_sound(si_audio *audio, uint8_t curr_port_5) {
	// If sound bit transitioned from low to high, play corresponding sound
	if (curr_port_5 != audio->prev_port_5) {
		if ((curr_port_5 & 0x01) && !(audio->prev_port_5 & 0x01)) {
			si_play_sound_once(audio, WALK1);
		} else if ((curr_port_5 & 0x02) && !(audio->prev_port_5 & 0x02)) {
			si_play_sound_once(audio, WALK2);
		} else if ((curr_port_5 & 0x04) && !(audio->prev_port_5 & 0x04)) {
			si_play_sound_once(audio, WALK3);
		} else if ((curr_port_5 & 0x08) && !(audio->prev_port_5 & 0x08)) {
			si_play_sound_once(audio, WALK4);
		} else if ((curr_port_5 & 0x10) && !(audio->prev_port_5 & 0x10)) {
			si_play_sound_once(audio, UFO_HIT);
		}

		audio->prev_port_5 = curr_port_5;
	}
}
