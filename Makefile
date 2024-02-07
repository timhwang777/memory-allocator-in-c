CC = gcc
CFLAGS = -Wall -Wextra

all: main

run: all
	./main

main: umem.o main.o
	$(CC) $(CFLAGS) -o main umem.o main.o

umem.o: umem.c
	$(CC) $(CFLAGS) -c umem.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f main umem struct-sizecheck check-wordsize umem.o main.o

.PHONY: all clean run
