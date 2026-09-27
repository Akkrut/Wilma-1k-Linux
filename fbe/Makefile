# Framebuffer Emulator — build with SDL2
#
# Usage: make && ./fbe
# Optional: ./fbe 1024 768   (custom resolution)

CC = gcc
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = $(shell sdl2-config --cflags --libs)
W ?= 1024
H ?= 768

.PHONY: all clean

all: fbe

fbe: fbe.c
	$(CC) $(CFLAGS) -DWIDTH=$(W) -DHEIGHT=$(H) -o fbe fbe.c $(LDFLAGS)

clean:
	rm -f fbe
