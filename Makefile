
CC=gcc
CFLAGS=-std=c11 -Os -O2 
CFLAGS+=-Wall -Wextra -Wshadow -Wfatal-errors -Werror -Wpedantic

all: bsc.o crc8scd30.o scd30.o

clean: 
	rm -rf bsc.o crc8scd30.o scd30.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
