.PHONY = all clean

CC     = gcc
CFLAGS = -Wall -Iinclude

all: main produttore consumatore globals programma

main: src/main.c
	$(CC) -c src/main.c $(CFLAGS) -o build/main.o

produttore: src/produttore.c
	$(CC) -c src/produttore.c $(CFLAGS) -o build/produttore.o

consumatore: src/consumatore.c
	$(CC) -c src/consumatore.c $(CFLAGS) -o build/consumatore.o

globals: src/globals.c
	$(CC) -c src/globals.c $(CFLAGS) -o build/globals.o

programma:
	$(CC) build/*.o -o bin/programma

clean:
	rm -f build/*.o bin/programma