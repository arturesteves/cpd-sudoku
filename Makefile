OS := $(shell uname)
ifeq ($(OS),Darwin)
CC=gcc-7
else
CC=gcc
endif

all: sudoku-serial sudoku-omp sudoku-mpi

sudoku-serial: sudoku-serial.c
	$(CC) -o sudoku-serial sudoku-serial.c
sudoku-omp: sudoku-omp.c
	$(CC) -fopenmp -o sudoku-omp sudoku-omp.c
sudoku-mpi: sudoku-mpi.c
	mpicc -o sudoku-mpi sudoku-mpi.c

clean:
	-rm -f input/*.out
	-rm -f *.o
	-rm -f sudoku-serial
	-rm -f sudoku-omp
	-rm -f sudoku-mpi
