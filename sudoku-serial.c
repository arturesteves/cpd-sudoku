#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


/**
* Print the puzzle matrix.
* @param puzzle Sudoku puzzle data structure.
*/
void print_puzzle(FILE * file,Puzzle * puzzle){
	int n = puzzle->n;
	for (int i = 0; i < n; ++i){
		for (int j = 0; j < n; ++j){
			fprintf(file,"%d ", puzzle->matrix[i][j]);
		}
		fprintf(file,"\n");
	}
}


/**
* Check if number is already in a sub grid of the puzzle matrix.
* @return Returns true if the number is inside one of the sub-grids of the matrix.
*/
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
* @param puzzle Sudoku puzzle data structure.
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
* @param puzzle Sudoku puzzle data structure.
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


/**
* Check if a number is already in a matrix cell according to sudoku rules.
* @param puzzle Sudoku puzzle data structure.
* @param row Row.
* @param col Column.
* @param num Comparison value.
* @return Returns true if the number is not valid.
*/
bool is_valid(Puzzle * puzzle, int row, int col, int num){
	return !(check_row(puzzle,row,num)) && 
		   !(check_column(puzzle,col,num)) && 
		   !(check_grid(puzzle, row - row % puzzle->root_n, col - col % puzzle->root_n, num));
}

/**
* Find empty cell in the sudoku.
* @param puzzle Sudoku puzzle data structure.
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


/**
* Attemp to solve the sudoku puzzle using backtracking.
* @param puzzle Sudoku puzzle data structure.
* @return Returns true if the sudoku has a solution.
*/
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


int main(int argc, char *argv[]){
	double start = omp_get_wtime();

	FILE * file_input;
	FILE * file_output;

	char * filename;

	// Check if file path was passed as an argument
	if (argc > 2){
		printf("ERROR: Too many arguments.\n");
		exit(EXIT_FAILURE);
	} else if (argc < 2) {
		printf("ERROR: Missing arguments.\n");
		exit(EXIT_FAILURE);
	}

	filename = argv[1];

	// Open file in read mode
	if ((file_input = fopen(filename,"r")) == NULL){
		printf("ERROR: Could not open file %s\n",filename);
		exit(EXIT_FAILURE);
	}

	// Number of rows and columns
	int n;
	// Square root of n
	int root_n;

	// Read first line from the file to get n
	fscanf(file_input, "%d\n", &root_n);

	n = root_n * root_n;

	// ======================================
	/** Initialize puzzle data structure */

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

	for (int i = 0; i < n; ++i){
		
		for (int j = 0; j < n; ++j){
			fscanf(file_input,"%d",&puzzle->matrix[i][j]);
		}
		fscanf(file_input, "\n");
	}
	// ======================================

	// Close file
	fclose(file_input);
	
	if(solve(puzzle)){

		/* Write solution to .out file. */
		char * name_out;
		
		// Split file name
		filename[strlen(filename) - 3 ] = '\0';
		name_out = (char *) malloc(sizeof(char) * (strlen(filename) + 4));
		strcpy(name_out, filename);
		strcat(name_out, ".out");

		// Open file in write mode
		file_output = fopen(name_out, "w");
		print_puzzle(file_output, puzzle);
		// Close output file
		fclose(file_output);
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