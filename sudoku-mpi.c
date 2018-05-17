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
void on_solution_found(int size, int ** matrix);
void debug_matrix(int size, int ** matrix);

void master(int argc, char *argv[]);
void slave();

int * toMatrix1D (int ** matrix);
int ** toMatrix2D (int * matrix, int n);

void free1DMatrix(int * matrix);
void free2DMatrix(int ** matrix);


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
    
    // Check command line arguments
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
    
    int rank;
    // init mpi
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (WORLD, &rank);

    // wait for all processes to init
    MPI_Barrier (WORLD);

    // starts counter
	_secs_ = - MPI_Wtime();

    if(rank == 0) {
        printf("master\n");
        fflush(stdout);
        master(argc, argv);
    } else {
        printf("slave with id: %d\n", rank);
        fflush(stdout);
        slave();
    }
    printf("ENDING process %d...\n", rank);
    fflush(stdout);

    // wait for all processes to reach this point - crucial
    MPI_Barrier(WORLD);
    printf("FINAL BARRIER\n");
    fflush(stdout);
    MPI_Finalize();
    printf("EXIT_SUCESS\n");
    return EXIT_SUCCESS;
}

void master(int argc, char *argv[]) {
    int availableProcesses;
    MPI_Comm_size (WORLD, &availableProcesses);

    FILE * file_input;
	FILE * file_output;

	char * filename; 

	filename = argv[1];

	// Open file in read mode
	if ((file_input = fopen(filename,"r")) == NULL){
		printf("ERROR: Could not open file %s\n",filename);
        fflush(stdout);
        MPI_Finalize();
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
    int * matrix_aux = toMatrix1D(puzzle->matrix);
    printf("master converted matrix aux\n");
    fflush(stdout);
    printf("Number of available processes: %d\n", availableProcesses - 1);
    fflush(stdout);
    int k;
    for (k = 1; k < availableProcesses; k++) {
        printf("Sending to process %d\n", k);
        fflush(stdout);
        //MPI_Send(&matrix_aux, n, MPI_INT, k, START_WORK, WORLD);
        MPI_Send(matrix_aux, n * n, MPI_INT, k, START_WORK, WORLD);
        
    }
    // enquanto existir processos a trabalhar ou enquanto nao for encontrada uma solucao fico a escuta de novas mensanges
    



    /////////////// NOT WORKING YET
    MPI_Status status;
    int size;
    //use probe
    //MPI_Probe(0, START_WORK, WORLD, &status);
    MPI_Probe(0, MPI_ANY_TAG, WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &size);
    if (status.MPI_TAG == NO_SOLUTION_FOUND) {
        // save info related to the work performed
        // send new work to the slave - probably need to ask a slave for work, contact one at a time, ou entao varios e o mais rapido e que vai ter o trabvalho distribuido
        printf("Master receiving a no solution message\n");
        fflush(stdout);
    } else if (status.MPI_TAG == SOLUTION_FOUND) {
        printf("Master receiving a solution found message\n");
        fflush(stdout);
        int n = (int) sqrt((double)size);
        int * matrix_aux = (int *) malloc(size * sizeof(int));
        MPI_Recv(matrix_aux, size, MPI_INT, 0, START_WORK, WORLD, &status);    
        on_solution_found(n, toMatrix2D(matrix_aux, n));
        // send a termite to all the slaves
        // print the solution
        
    } else {
        printf("Master receiving a a message with the tag %d\n", status.MPI_TAG);
        fflush(stdout);
    }
    

    //free1DMatrix(matrix_aux);


}

void slave() {
    int rank, size;
    MPI_Status status;
    
    MPI_Comm_rank (WORLD, &rank);

    printf("Receiving on process %d\n", rank);
    fflush(stdout);
    ////////
    // receive initial work 
    ////////
    // check metadata of the request, to get the size of the array
    
    MPI_Probe(0, START_WORK, WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &size);

    int n = (int) sqrt((double)size);
    int * matrix_aux = (int *) malloc(size * sizeof(int));
    MPI_Recv(matrix_aux, size, MPI_INT, 0, START_WORK, WORLD, &status);    
    
    printf("Receiving on process %d, array with size %d\n", rank, size);
    fflush(stdout);

    printf("end receiving..\n");
    fflush(stdout);
    printf("Slave %d solving puzzle\n", rank);
    
    // create puzzle
    Puzzle * puzzle = malloc(sizeof(Puzzle));
	puzzle->n = n;
	puzzle->root_n = (int) sqrt((double) n);
	puzzle->depth = 1;
	puzzle->matrix = toMatrix2D(matrix_aux, n);

    if(solve(puzzle)) {
        printf("Slave %d FOUND A SOLUTION\n", rank);
        // send puzzle
        int * matrix_solution = toMatrix1D(puzzle->matrix);
        MPI_Send(matrix_solution, n * n, MPI_INT, 0, SOLUTION_FOUND, WORLD);
    } else {
        printf("Slave %d DIDN'T FOUND A SOLUTION\n", rank);
        MPI_Send(0, 1, MPI_INT, 0, NO_SOLUTION_FOUND, WORLD);
    }

    // todo: expect a message, new work or terminate!


    //free1DMatrix(matrix_aux);
    //free2DMatrix(matrix);
    
}


int * toMatrix1D (int ** matrix) {
    // size = size of a line
    int n = sizeof(matrix[0]);
    int * matrix_aux = (int *) malloc(n * n * sizeof(int));
    int i, j, pos = 0;
    printf("\n TO MATRIX 1D\n");
    fflush(stdout);
    printf("SIZE: %d\n", n);
    fflush(stdout);
    for (i = 0; i <= n; i++) {
        for (j = 0; j <= n; j++) {
            matrix_aux[pos] = matrix[i][j];
            printf("%d ", matrix_aux[pos]);
            fflush(stdout);
            pos++;
        }
        printf("\n");
        fflush(stdout);
    }
    printf("\n\n");
    fflush(stdout);

    return matrix_aux;

    //int * puzzle;
    //puzzle = (int *) malloc (n * sizeof(int));   diferença? malloc (n * sizeof(int))
}

int ** toMatrix2D (int * matrix, int n) {
    int ** matrix_aux = (int**) malloc(n * sizeof(int*));
    int i, j, pos, k;
    printf("\n TO MATRIX 2D\n");
    fflush(stdout);
    printf("SIZE: %d\n", n);
    fflush(stdout);
    for (k = 0; k < n; k++){
		matrix_aux[k] = (int * )malloc(n * sizeof(int));
	}

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            matrix_aux[i][j] = matrix[pos++];
            printf("%d ", matrix_aux[i][j]);
            fflush(stdout);
        }
        printf("\n");
        fflush(stdout);
    }

    return matrix_aux;
}

void free1DMatrix(int * matrix) {
    free(matrix);
}

void free2DMatrix(int ** matrix) {
    int n = sizeof(matrix[0]);
    /*int i;
    for (i = 0; i < n; i++){
        free(matrix[i]);
    }*/
    free(matrix);
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
 * Print the puzzle matrix.
 *
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





void on_solution_found(int size, int ** matrix) {
    _secs_ += MPI_Wtime();
    if (_time_only_flag_) {
        printf("Elapsed time: %12.6f (s)\n", _secs_);
    } else if (_time_flag_) {
        debug_matrix(size, matrix);
        printf("Searched %d states in total.\n", _states_searched_);
        printf("Elapsed time: %12.6f (s)\n", _secs_);
    } else {
        debug_matrix(size, matrix);
    }
    fflush(stdout);    
    //MPI_Finalize();
    //exit(EXIT_SUCCESS);
}

void debug_matrix(int size, int ** matrix){
    if (matrix != NULL) {
        int n = size;
        int i, j;
        for (i = 0; i < n; ++i){
            for (j = 0; j < n; ++j){
                printf("%d ", matrix[i][j]);
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
        }
    }
}
