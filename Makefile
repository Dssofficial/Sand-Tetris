CC = gcc
CFLAGS = -Wall -std=c11
CFLAGS += `sdl2-config --cflags`
# CFLAGS += -02

# CFLAGS += `pkg-config --cflags SDL2_image SDL2_ttf`
# CFLAGS += -Wextra

LIBS = `sdl2-config --libs`
## LIBS += -lSDL2_ttf
# LIBS += `pkg-config --libs SDL2_image SDL2_ttf`

SRC = src/main.c src/game.c
OUT = build/game

all:
	@mkdir -p build
	@$(CC) $(SRC) $(CFLAGS) -o $(OUT) $(LIBS)
	@./$(OUT)
	@rm -rf build