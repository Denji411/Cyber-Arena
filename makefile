# ──────────────────────────────────────────────────────────────
#  Makefile – CobotRop  (C-2026 Extreme Edition)
#  Dipendenze: gcc, ncurses, pthreads, ZeroMQ (libzmq)
#
#  Installazione dipendenze su Debian/Ubuntu:
#    sudo apt install libncurses-dev libzmq3-dev
#  Su Arch:
#    sudo pacman -S ncurses zeromq
# ──────────────────────────────────────────────────────────────

.PHONY: all clean run

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
# -lncurses  → ncurses
# -lpthread  → POSIX thread
# -lzmq      → ZeroMQ
# -lm        → math (abs, sqrt...)
LDFLAGS = -lncurses -lpthread -lzmq -lm

# Sorgenti e oggetti
SRCS = src/main.c    \
       src/robot.c   \
       src/grafica.c \
       src/eventi.c  \
       src/globals.c

OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

TARGET = bin/cobot_rop

# ── Regola principale ──────────────────────────────────────────
all: dirs $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compila ogni .c in un .o dentro build/
build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Crea le directory se non esistono
dirs:
	@mkdir -p bin build

# ── Esecuzione rapida ──────────────────────────────────────────
run: all
	./$(TARGET)

# ── Pulizia ───────────────────────────────────────────────────
clean:
	rm -f build/*.o $(TARGET)
