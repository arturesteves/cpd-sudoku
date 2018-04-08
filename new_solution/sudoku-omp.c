////////////////////////////////////////////////////////////
//// Includes
////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>


////////////////////////////////////////////////////////////
//// Structures
////////////////////////////////////////////////////////////
struct Puzzle {
    int row;
    int col;
    int depth;
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
// get the size of elements on an array
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))


////////////////////////////////////////////////////////////
//// Global Variables
////////////////////////////////////////////////////////////
static double _start_;
static double _end_;
static int _offset_ = 10;
static bool _time_flag_ = false;
static bool _time_only_flag_ = false;

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
Puzzle * copy(Puzzle * puzzle);
void cleanPuzzle(Puzzle * puzzle);
void end_on_solution_found(Puzzle * puzzle);


////////////////////////////////////////////////////////////
//// Main Execution 
////////////////////////////////////////////////////////////

/**
 * Parallel Sudoku Solver using OpenMP
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
	puzzle->depth = 1;
	puzzle->row = 0;
	puzzle->col = 0;
	puzzle->matrix = (int**) malloc(n * sizeof(int*));
    int i;
	for (i = 0; i < n; ++i){
		puzzle->matrix[i] = (int * )malloc(n * sizeof(int));
	}

	// Read matrix from the file
	int cursor;
	int row = 0, col = 0, j;
	for (i = 0; i < n; ++i){
		
		for (j = 0; j < n; ++j){
			fscanf(file_input,"%d",&puzzle->matrix[i][j]);
		}
		fscanf(file_input, "\n");
	}
	// ======================================

	// Close file
	fclose(file_input);


    //////////////////////////////////////////////////////////
    ////// START
    //////////////////////////////////////////////////////////
    //omp_set_num_threads(2);

    // initialize threads
    #pragma omp parallel
    {
        //printf("nº threads: %d\n", omp_get_num_threads());
        // only one of the threads initiate the solve method
        #pragma omp single
        {
            if(!solve(puzzle)){
                // if no solution was found
	            _end_ = omp_get_wtime();
                if (_time_only_flag_) {
                    printf("Elapsed time: %f (s)\n", _end_ - _start_);
                } else if (_time_flag_) {
                    printf("No solution\n");
                    printf("Elapsed time: %f (s)\n", _end_ - _start_);
                } else {
                    printf("No solution\n");
                }
        	}
        }
    }

	return EXIT_SUCCESS;
}

/**
* Print the puzzle matrix.

* @param puzzle Sudoku puzzle data structure.
*/
void debug_puzzle(Puzzle * puzzle){
    #pragma omp critical 
    {
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
    for (i = 0; i < puzzle->n; ++i){
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
bool solve(Puzzle * puzzle) {

    int row = 0, col = 0;
    int depth = puzzle->depth;

	// Check if puzzle is complete
	if (!find_empty(puzzle, &row, &col)){
		return true;
	}
    
    int i;
	for (i = 1; i <= puzzle->n; ++i){

		// Check if number can be placed in a cell
		if (is_valid(puzzle, row, col, i)){
			puzzle->matrix[row][col] = i;
            puzzle->depth = depth + 1;
            
            // if the depth is lower then a offset create a task
            if(puzzle->depth < _offset_) {
                // creates a new copy of the sudoku puzzle
                Puzzle * successor = copy(puzzle);
                // creates a task
                #pragma omp task default(shared) firstprivate(row, col, depth, successor)
                {
                    //Proceeds with a copy as a task
                    if (solve(successor)){
                        end_on_solution_found(successor);
        			}
                }
            } else {    // continues the program in serial mode
                if (solve(puzzle)){
    			    end_on_solution_found(puzzle);
        		}
            }
            
            // the value on the position didn't reach the solution, so change it to zero
			puzzle->matrix[row][col] = 0;
		}
	}
	
	#pragma omp taskwait

	return false;

}

/**
 * Creates a new puzzle based on a puzzle received as argument.
 * 
 * @param Puzzle Sudoku puzzle data structure to copy. 
 * @return Returns a puzzle data data structure if the puzzle received
 * is not NULL if it is NULL then returns NULL.
 */
Puzzle * copy(Puzzle * puzzle) {
    if (puzzle == NULL) {
        return NULL;
    }
    Puzzle * copyPuzzle = malloc(sizeof(Puzzle));
    copyPuzzle->depth = puzzle->depth;
    copyPuzzle->row = puzzle->row;
    copyPuzzle->col = puzzle->col;
    copyPuzzle->root_n = puzzle->root_n;
    copyPuzzle->n = puzzle->n;
    copyPuzzle->matrix = (int**) malloc(puzzle->n * sizeof(int*));          // alloc space for matrix
    int i,j;
    // manual copy
    for (i = 0; i < puzzle->n; ++i){
        copyPuzzle->matrix[i] = (int * )malloc(puzzle->n * sizeof(int));    // alloc space
        for (j = 0; j < puzzle->n; ++j){
            copyPuzzle->matrix[i][j] = puzzle->matrix[i][j];                // copy values
        }
    }
    return copyPuzzle;
}

/**
 * Free's a Sudoku puzzle structure.
 * 
 * @param puzzle Sudoku puzzle data structure.
 */
void cleanPuzzle (Puzzle * puzzle) {
    if (puzzle != NULL) {
        int n = puzzle->n;
        int i;
        // Free memory   
        for (i = 0; i <  n ; i++){
            free(puzzle->matrix[i]);
        }
        free(puzzle->matrix);
        free(puzzle);
    }
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
        printf("Elapsed time: %f (s)\n", _end_ - _start_);
    } else {
        debug_puzzle(puzzle);
    }
    exit(EXIT_SUCCESS);
}