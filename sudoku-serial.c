////////////////////////////////////////////////////////////
//// Includes
////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>


////////////////////////////////////////////////////////////
//// Structures
////////////////////////////////////////////////////////////
struct Puzzle {
	int root_n;
	int n;
	int ** matrix;
};


////////////////////////////////////////////////////////////
//// Types
////////////////////////////////////////////////////////////
typedef struct Puzzle Puzzle;
typedef int bool;


////////////////////////////////////////////////////////////
//// Defines 
////////////////////////////////////////////////////////////
#define false 0
#define true 1


////////////////////////////////////////////////////////////
//// Global Variables
////////////////////////////////////////////////////////////
static double _start_;
static double _end_;
static bool _time_flag_ = false;
static bool _time_only_flag_ = false;
static int _states_searched_ = 0;

////////////////////////////////////////////////////////////
//// Function Prototypes  
////////////////////////////////////////////////////////////
void debug_puzzle(Puzzle * puzzle);
void print_puzzle_to_file(FILE * file,Puzzle * puzzle);
bool check_grid(Puzzle * puzzle, int row, int column, int number);
bool check_column(Puzzle * puzzle, int column, int number);
bool check_row(Puzzle * puzzle, int row, int number);
bool is_valid(Puzzle * puzzle, int row, int column, int number);
bool find_empty(Puzzle * puzzle, int * row, int * column);
bool solve(Puzzle * puzzle);
void end_on_solution_found(Puzzle * puzzle);


////////////////////////////////////////////////////////////
//// Main Execution 
////////////////////////////////////////////////////////////

/**
 * Serial Sudoku Solver.
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return Returns EXIT_SUCCESS on finishing the execution successful.
 */
int main(int argc, char *argv[]){
    // starts counter
	_start_ = omp_get_wtime();

	FILE * file_input;
	FILE * file_output;

	char * filename;

	// Check if file path was passed as an argument
	if (argc > 4){
		printf("ERROR: Too many arguments.\n");
		exit(EXIT_FAILURE);
	} else if (argc < 2) {
		printf("ERROR: Missing arguments.\n");
		exit(EXIT_FAILURE);
	} else if(argc == 3 && strcmp(argv[2], "-to") == 0) {
        _time_only_flag_ = true;
    } else if(argc == 3 && strcmp(argv[2], "-t") == 0) {
        _time_flag_ = true;
    }  else if(argc > 3 && (strcmp(argv[2], "-to") == 0 || strcmp(argv[3], "-to") == 0)) {
        _time_only_flag_ = true;
    } 

	filename = argv[1];

	// Open file in read mode
	if ((file_input = fopen(filename, "r")) == NULL){
		printf("ERROR: Could not open file %s\n", filename);
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
	int i;
	for (i = 0; i < n; ++i){
		puzzle->matrix[i] = (int * )malloc(n * sizeof(int));
	}


	// Read matrix from the file
	int cursor;
	int row = 0, col = 0;
	int j;
	for (i = 0; i < n; ++i){
		for (j = 0; j < n; ++j){
			fscanf(file_input, "%d", &puzzle->matrix[i][j]);
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
        // output puzzle to file
		print_puzzle_to_file(file_output, puzzle);
		// Close output file
		fclose(file_output);
    
        end_on_solution_found(puzzle);

	} else {
        // if no solution was found
        _end_ = omp_get_wtime();
        if (_time_only_flag_) {
            printf("Elapsed time: %f (s)\n", _end_ - _start_);
        } else if (_time_flag_) {
            printf("No solution\n");
            printf("Searched %d states in total.\n", _states_searched_);
            printf("Elapsed time: %f (s)\n", _end_ - _start_);
        } else {
            printf("No solution\n");
        }
	}

    // ======================================
    /** Free puzzle memory */
	for (i = 0; i <  n ; ++i){
		free(puzzle->matrix[i]);
	}
	free(puzzle->matrix);
	free(puzzle);
    // ======================================
    
	return EXIT_SUCCESS;
}

/**
* Print the puzzle matrix.

* @param puzzle Sudoku puzzle data structure.
*/
void debug_puzzle(Puzzle * puzzle){
    if (puzzle != NULL) {
        int n = puzzle->n;
        int i, j;
        for (i = 0; i < n; ++i){
            for (j = 0; j < n; ++j){
                printf("%d ", puzzle->matrix[i][j]);
            }
            printf("\n");
        }
    }
}

/**
 * Print the puzzle matrix on file.
 * 
 * @param file file data structure to print the sudoku puzzle.
 * @param puzzle Sudoku puzzle data structure.
 */
void print_puzzle_to_file(FILE * file, Puzzle * puzzle){
	int n = puzzle->n;
    int i, j;
	for (i = 0; i < n; ++i){
		for (j = 0; j < n; ++j){
			fprintf(file, "%d ", puzzle->matrix[i][j]);
		}
		fprintf(file, "\n");
	}
}

/**
 * Check if number is already in a sub grid of the puzzle matrix.
 * 
 * @param puzzle Sudoku puzzle data structure.
 * @param row Row of the puzzle to check the value.
 * @param column Column of the puzzle to check the value.
 * @param number Value to compare to the one in the position given by the row and column.
 * @return Returns true if the number is inside one of the sub-grids of the matrix.
 */
bool check_grid(Puzzle * puzzle, int row, int column, int number){
    int i, j;
	for (i = 0; i < puzzle->root_n; ++i){
		for (j = 0; j < puzzle->root_n; ++j){
			if (puzzle->matrix[i + row][j + column] == number){
				return true;
			}
		}
	}
	return false;
}

/**
 * Check if a number is already in a column.
 * 
 * @param puzzle Sudoku puzzle data structure.
 * @param column Column of the puzzle to check the value.
 * @param number Value to compare to the one in the position given by the column.
 * @return Returns true if the number is in the column.
 */
bool check_column(Puzzle * puzzle, int column, int number){
    int i;
	for (i = 0; i < puzzle->n; i++){
		if(puzzle->matrix[i][column] == number){
			return true;
		}
	}
	return false;
}

/**
 * Check if a number is already in a row.
 * 
 * @param puzzle Sudoku puzzle data structure.
 * @param row Row of the puzzle to check the value.
 * @param number Value to compare to the one in the position given by the row.
 * @return Returns true if the number is in the row.
 */
bool check_row(Puzzle * puzzle, int row, int number){
	int i;
    for (i = 0; i < puzzle->n; ++i){
		if (puzzle->matrix[row][i] == number){
			return true;
		}
	}
	return false;
}

/**
 * Check if a number is already in a matrix cell according to sudoku rules.
 * 
 * @param puzzle Sudoku puzzle data structure.
 * @param row Row of the puzzle to check the value.
 * @param column Column of the puzzle to check the value.
 * @param number Comparison value.
 * @return Returns true if the number is not valid.
 */
bool is_valid(Puzzle * puzzle, int row, int column, int number){
	return !(check_row(puzzle, row, number)) && 
		   !(check_column(puzzle, column, number)) && 
		   !(check_grid(puzzle, row - row % puzzle->root_n, column - column % puzzle->root_n, number));
}

/**
 * Find a empty cell in the sudoku puzzle.
 * 
 * @param puzzle Sudoku puzzle data structure.
 * @param row Row number reference.
 * @param column Column number reference.
 * @return Returns true if the puzzle has an empty position.
 */
bool find_empty(Puzzle * puzzle, int * row, int * column){
	for (*row = 0; *row < puzzle->n; (*row)++){
		for (*column = 0; *column < puzzle->n; (*column)++){
			if (puzzle->matrix[*row][*column] == 0){
				return true;
			}
		}
	}
	return false;
}

/**
 * Attemp to solve the sudoku puzzle using backtracking.
 * 
 * @param puzzle Sudoku puzzle data structure.
 * @return Returns true if the sudoku has a solution.
 */
bool solve(Puzzle * puzzle){
    _states_searched_ ++;
	int i, row = 0, column = 0;

	// Check if puzzle is complete
	if (!find_empty(puzzle, &row, &column)){
        // solution found
		return true;
	}

    // Iterate over the N cells of a sudoku puzzle
	for (i = 1; i <= puzzle->n; ++i){

		// Check if number can be placed in a cell
		if (is_valid(puzzle, row, column, i)){
			puzzle->matrix[row][column] = i;

            // call solve with the new value on the sudoku puzzle
			if (solve(puzzle)){
                // solution found
				return true;
			}

            // if the change didn't led to a solution set it to zero to be changed by other value
			puzzle->matrix[row][column] = 0;
		}
	}
    // no solution found
	return false;
}

/**
 * Prints the sudoku puzzle solved and the time accordingly to the flags passed as arguments.
 * 
 * @param puzzle Sudoku puzzle data structure.
 */
void end_on_solution_found(Puzzle * puzzle) {
    _end_ = omp_get_wtime();
    if (_time_only_flag_) {
        printf("Elapsed time: %f (s)\n", _end_ - _start_);
    } else if (_time_flag_) {
        debug_puzzle(puzzle);
        printf("Searched %d states in total.\n", _states_searched_);
        printf("Elapsed time: %f (s)\n", _end_ - _start_);
    } else {
        debug_puzzle(puzzle);
    }
    exit(EXIT_SUCCESS);
}