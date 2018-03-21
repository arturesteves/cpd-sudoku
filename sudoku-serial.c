#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

typedef int bool;
#define false 0
#define true 1

struct Puzzle {
	int root_n;
	int n;
	int ** matrix;
};

typedef struct Puzzle Puzzle;


void print_puzzle(Puzzle * puzzle){
	int n = puzzle->n;
	for (int i = 0; i < n; ++i){
		for (int j = 0; j < n; ++j){
			printf("%d ", puzzle->matrix[i][j]);
		}
		printf("\n");
	}
}

bool check_grid(Puzzle * puzzle, int row, int col, int num){
	for (int i = 0; i < puzzle->root_n; ++i){
		for (int j = 0; j < puzzle->root_n; ++j){
			if (puzzle->matrix[i + row][j + col] == num){
				return true;
			}
		}
	}
	return false;
}

/**
* Check if a number is already in a column.
* @param n Number of rows.
* @param puzzle Sudoku puzzle matrix.
* @param col Column.
* @param num Comparison value.
* @return Returns true if the number is in the column.
*/

bool check_column(Puzzle * puzzle, int col, int num){
	for (int i = 0; i < puzzle->n; ++i){
		if(puzzle->matrix[i][col] == num){
			return true;
		}
	}
	return false;
}

/**
* Check if a number is already in a row.
* @param n Number of columns.
* @param puzzle Sudoku puzzle matrix.
* @param row Row.
* @param num Comparison value.
* @return Returns true if the number is in the row.
*/

bool check_row(Puzzle * puzzle, int row, int num){
	for (int i = 0; i < puzzle->n; ++i){
		if (puzzle->matrix[row][i] == num){
			return true;
		}
	}
	return false;
}


bool is_valid(Puzzle * puzzle, int row, int col, int num){
	return !(check_row(puzzle,row,num)) && !(check_column(puzzle,col,num)) && !check_grid(puzzle, row - row % puzzle->root_n, col - col % puzzle->root_n, num);
}

/**
* Find empty cell in the sudoku.
*
* @param row Row number reference
* @param col Column number reference
* @return Returns true if the puzzle has an empty position.
*/

bool find_empty(Puzzle * puzzle, int * row, int * col){
	for (*row = 0; *row < puzzle->n; (*row)++){
		for (*col = 0; *col < puzzle->n; (*col)++){
			if (puzzle->matrix[*row][*col] == 0){
				return true;
			}
		}
	}
	return false;
}


bool solve(Puzzle * puzzle){
	int row = 0, col = 0;

	// Check if puzzle is complete
	if (!find_empty(puzzle, &row, &col)){
		return true;
	}

	for (int i = 1; i <= puzzle->n; ++i){

		// Check if number can be placed in a cell
		if (is_valid(puzzle, row, col, i)){
			puzzle->matrix[row][col] = i;

			if (solve(puzzle)){
				return true;
			}

			puzzle->matrix[row][col] = 0;
		}
	}


	return false;

}


int main(int argc, char const *argv[]){
	double start = omp_get_wtime();

	FILE * file;

	// Check if file path was passed as an argument
	if (argc > 2){
		printf("ERROR: Too many arguments.\n");
		exit(EXIT_FAILURE);
	} else if (argc < 2) {
		printf("ERROR: Missing arguments.\n");
		exit(EXIT_FAILURE);
	}

	// Open file in read mode
	if ((file = fopen(argv[1],"r")) == NULL){
		printf("ERROR: Could not open file %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}



	// Number of rows and columns
	int n;
	// Square root of n
	int root_n;

	// Read first line from the file to get n
	fscanf(file, "%d\n", &root_n);

	n = root_n * root_n;


	// Puzzle matrix N x N
	Puzzle * puzzle = malloc(sizeof(Puzzle));
	puzzle->n = n;
	puzzle->root_n = root_n;

	puzzle->matrix = (int**) malloc(n * sizeof(int*));

	for (int i = 0; i < n; ++i){
		puzzle->matrix[i] = (int * )malloc(n * sizeof(int));
	}


	// Read matrix from the file
	int cursor;
	int row = 0, col = 0;

	while( (cursor = getc(file)) != EOF ){
		switch(cursor){
			case ' ':
				break;
			case '\n':
				row++;
				col = 0;
				break;
			default:
				puzzle->matrix[row][col] = (cursor - '0');
				col++;
				break;
		}
	}

	// Close file
	fclose(file);

	
	if(solve(puzzle)){
		print_puzzle(puzzle);
	} else {
		printf("No solution\n");
	}


	// Free memory
	for (int i = 0; i <  n ; ++i){
		free(puzzle->matrix[i]);
	}
	
	free(puzzle->matrix);
	free(puzzle);

	double end = omp_get_wtime();
	printf("Elapsed time: %f (s)\n", end - start);
	return EXIT_SUCCESS;
}