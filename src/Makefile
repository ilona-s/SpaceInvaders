CC = gcc
CFLAGS = -std=c99 -Wall -g

SRCS = $(wildcard *.c)
BINS = $(SRCS:%.c=%.o)

SDL2_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_mixer)
SDL2_LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_mixer)

invaders: $(BINS)
	$(CC) $(SDL2_LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) $< -c

clean:
	rm $(BINS) invaders
