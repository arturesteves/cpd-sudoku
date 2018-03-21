OS := $(shell uname)
ifeq ($(OS),Darwin)
CC=gcc-7
else
CC=gcc
endif

all: sudoku-serial sudoku-omp

sudoku-serial: sudoku-serial.c
	$(CC) -fopenmp -o sudoku-serial sudoku-serial.c
sudoku-omp: sudoku-omp.c
	$(CC) -fopenmp -o sudoku-omp sudoku-omp.c

clean:
	-rm -f *.o
	-rm -f sudoku-serial
	-rm -f sudoku-omp