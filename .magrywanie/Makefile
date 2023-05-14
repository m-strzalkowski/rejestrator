CC = gcc
.PHONY: all
all: nagrywaj

rejestrator: nagrywaj.cpp
	$(CC) -Wall -O2 nagrywaj.cpp -o rejestrator -llsm9ds1

clean:
	rm -f rejestrator
