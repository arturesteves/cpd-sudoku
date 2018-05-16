////////////////////////////////////////////////////////////
//// Includes
////////////////////////////////////////////////////////////
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>


////////////////////////////////////////////////////////////
//// Structures
////////////////////////////////////////////////////////////
struct Puzzle {
	int root_n;
	int depth;
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
static double _secs_;
static int _offset_ = 10;
static bool _time_flag_ = false;
static bool _time_only_flag_ = false;
static int _tasks_in_process_ = 0;
static int _states_searched_ = 0;
// mpi global variables
static int _id_; 
static int _world_size_;

////////////////////////////////////////////////////////////
//// MPI TAGS
////////////////////////////////////////////////////////////
#define START_WORK 0
#define NO_SOLUTION_FOUND 1
#define SOLUTION_FOUND 2
#define STOP_WORK 3


////////////////////////////////////////////////////////////
//// MPI COMM
////////////////////////////////////////////////////////////
#define WORLD MPI_COMM_WORLD


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


    // init mpi
    MPI_Init (&argc, &argv);


    MPI_Comm_rank (MPI_COMM_WORLD, &_id_);
    MPI_Comm_size (MPI_COMM_WORLD, &_world_size_);

    // not considereing the timinig of mpi initialization
    MPI_Barrier (MPI_COMM_WORLD);

    // starts counter
	_secs_ = - MPI_Wtime();

	FILE * file_input;
	FILE * file_output;

	char * filename;

	// Check if file path was passed as an argument
	if (argc > 4){
		printf("ERROR: Too many arguments.\n");
        fflush(stdout);
		exit(EXIT_FAILURE);
	} else if (argc < 2) {
		printf("ERROR: Missing arguments.\n");
        fflush(stdout);
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
        fflush(stdout);
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


// send something to somewhere?

    if(!solve(puzzle)){
        // if no solution was found
        _secs_ += MPI_Wtime();
        if (_time_only_flag_) {
            printf("Elapsed time: %12.6f (s)\n", _secs_);
        } else if (_time_flag_) {
            printf("No solution\n");
            printf("Searched %d states in total.\n", _states_searched_);
            printf("Elapsed time: %12.6f (s)\n", _secs_);
        } else {
            printf("No solution\n");
        }
        fflush(stdout);
    }

    MPI_Finalize();

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
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
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
            fflush(stdout);
		}
		fprintf(file, "\n");
        fflush(stdout);
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


bool solve(Puzzle * puzzle) {
    int availableProcesses = _world_size_;
    //MPI_Request requests[puzzle->n];
    //MPI_Status statuses[puzzle->n];
    MPI_Status status;


    _states_searched_ ++;
    int row = 0, col = 0;
    int depth = puzzle->depth;

	// Check if puzzle is complete
	if (!find_empty(puzzle, &row, &col)){
		return true;
	}
    


    /////////// colocar uma condicao while usando a variable availableprocesses, para apenas enviar trabalho quando existirem processos disponiveis
    
    int i;
	for (i = 1; i <= puzzle->n; ++i){

		// Check if number can be placed in a cell
		if (is_valid(puzzle, row, col, i)){
			puzzle->matrix[row][col] = i;
            puzzle->depth = depth + 1;
            /*
                if master send i
                if slave receive i and solve it, only the slave will solve it, not the master
            */
            // If is the MASTER, id = 0
            if(!_id_) {
                printf("Sending %d to process %d\n", i, 1);
                fflush(stdout);
                MPI_Send(&i, 1, MPI_INT, 1, START_WORK, WORLD);
                availableProcesses--;


                // receive message: found solution
                MPI_Recv(&i, 1, MPI_INT, 1, SOLUTION_FOUND, WORLD, &status);
                availableProcesses++;
                printf("Receiving from process %d - SOLUTION FOUND\n", _id_);
                fflush(stdout);

                //wait for a complete or a no solution
            } else {
                printf("Receiving %d from process %d\n", i, 0);
                fflush(stdout);
                MPI_Recv(&i, 1, MPI_INT, _id_-1, START_WORK, WORLD, &status);
                if (solve_aux(puzzle)){
                    end_on_solution_found(puzzle);
                }
                // the value on the position didn't reach the solution, so change it to zero
			    puzzle->matrix[row][col] = 0;
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
bool solve_aux(Puzzle * puzzle) {
    _states_searched_ ++;
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
            if (solve_aux(puzzle)){
                // slave found a solution - inform master
                MPI_Send(&i, 1, MPI_INT, 0, SOLUTION_FOUND, WORLD);
                end_on_solution_found(puzzle);
            }
            
            // the value on the position didn't reach the solution, so change it to zero
			puzzle->matrix[row][col] = 0;
		}
	}

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
    Puzzle * copy_puzzle = malloc(sizeof(Puzzle));
    copy_puzzle->root_n = puzzle->root_n;
    copy_puzzle->n = puzzle->n;
    copy_puzzle->depth = puzzle->depth;
    copy_puzzle->matrix = (int**) malloc(puzzle->n * sizeof(int*));          // alloc space for matrix
    int i,j;
    // manual copy
    for (i = 0; i < puzzle->n; ++i){
        copy_puzzle->matrix[i] = (int * )malloc(puzzle->n * sizeof(int));    // alloc space
        for (j = 0; j < puzzle->n; ++j){
            copy_puzzle->matrix[i][j] = puzzle->matrix[i][j];                // copy values
        }
    }
    return copy_puzzle;
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
    _secs_ += MPI_Wtime();
    if (_time_only_flag_) {
        printf("Elapsed time: %12.6f (s)\n", _secs_);
    } else if (_time_flag_) {
        debug_puzzle(puzzle);
        printf("Searched %d states in total.\n", _states_searched_);
        printf("Elapsed time: %12.6f (s)\n", _secs_);
    } else {
        debug_puzzle(puzzle);
    }
    fflush(stdout);

    
    MPI_Finalize();
    exit(EXIT_SUCCESS);
}
