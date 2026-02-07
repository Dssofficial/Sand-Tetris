CC = gcc

CFLAGS = -Wall -std=c11 `sdl2-config --cflags`
# CFLAGS += -O2
CFLAGS += -fsanitize=address,undefined
# CFLAGS += -Wextra

LIBS = `sdl2-config --libs`
LIBS += -lm -lSDL2_mixer -lSDL2_ttf

# SRC = src/main.c src/font.c src/game.c
SRC = $(wildcard src/*.c)
OUT = build/game

all:
	@mkdir -p build
	@cp -r assets build
	@$(CC) $(SRC) $(CFLAGS) -o $(OUT) $(LIBS)
	@./$(OUT)