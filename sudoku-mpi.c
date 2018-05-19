#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

#include <unistd.h>

struct Puzzle {
        int root_n;
        int depth;
        int n;
        int * matrix;
};

typedef struct Puzzle Puzzle;

struct Node {
    int * matrix;
    int n;
    struct Node * next;
};

typedef int bool;

#define false 0
#define true 1

// Get the size of elements on an array
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

#define START_WORK 123
#define ASK_FOR_WORK 234
#define NO_SOLUTION_FOUND 345
#define SOLUTION_FOUND 456
#define STOP_WORK 567

#define WORLD MPI_COMM_WORLD


static bool ORDER_STOP_WORKING = false;


void init(struct Node * head);
void display(struct Node * head);
bool is_empty(struct Node * head);
struct Node * push(struct Node * head, int n, int * matrix);
struct Node * pop(struct Node * head, int * n, int ** matrix);
void print_puzzle(Puzzle * puzzle);
bool check_grid(Puzzle * puzzle, int row, int column, int number);
bool check_column(Puzzle * puzzle, int column, int number);
bool check_row(Puzzle * puzzle, int row, int number);
bool is_valid(Puzzle * puzzle, int row, int column, int number);
bool find_empty(Puzzle * puzzle, int * row, int * column);
bool solve(Puzzle * puzzle);
Puzzle * copy_puzzle(Puzzle * puzzle);
int * copy_matrix(int n, int * matrix);
void cleanPuzzle(Puzzle * puzzle);
void on_solution_found(int size, int * matrix, double secs);
void debug_matrix(int size, int * matrix);
void master(int argc, char *argv[]);
void slave();

/**
 * Parallel Sudoku Solver using MPI
 *
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return Returns EXIT_SUCCESS on finishing the execution successful.
 */
int main(int argc, char *argv[]){

    // Check command line arguments
     if (argc != 2){
        printf("ERROR: Invalid number of arguments arguments.\n");
        exit(EXIT_FAILURE);
    }
    int rank;

    // Initialize MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (WORLD, &rank);

    // Wait for all processes to init
    MPI_Barrier (WORLD);

    if(rank == 0) {
        master(argc, argv);
    } else {
        slave();
    }

    // Wait for all processes to reach this point - crucial
    //MPI_Barrier(WORLD);

    MPI_Finalize();
    return EXIT_SUCCESS;
}

void master(int argc, char *argv[]) {

    struct Node * work_pool = NULL;
    init(work_pool);

    // Start counter
    double secs = - MPI_Wtime();
    int nprocs;
    MPI_Comm_size (WORLD, &nprocs);
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
    Puzzle * puzzle = malloc(sizeof(Puzzle));
    puzzle->n = n;
    puzzle->root_n = root_n;
    puzzle->depth = 1;
    puzzle->matrix = (int*) malloc(n * n * sizeof(int));

    // Read matrix from the file
    int cursor;
    int row = 0, col = 0, i, j;
    for (i = 0; i < n; ++i){
            for (j = 0; j < n; ++j){
                    fscanf(file_input, "%d", &puzzle->matrix[i * puzzle->n + j]);
            }
            fscanf(file_input, "\n");
    }
    // ======================================
    // Close file
    fclose(file_input);




    //Add candidates to a pool of tasks.
    int r, c;
    if(find_empty(puzzle, &r, &c)){
        int num;
        for(num = 1; num <= puzzle->n; num++){
            if(is_valid(puzzle, r, c, num)){
        int * mat = copy_matrix(puzzle->n, puzzle->matrix);
        mat[r * puzzle->n + c] = num;
                work_pool = push(work_pool, puzzle->n, mat);
        }
        }
    }

    MPI_Status status;
    MPI_Status status2;
    int size;
    bool exit = false;

    bool procs[nprocs];
    // Number of active slaves
    int procs_count = nprocs - 1;

    // Initialize available processes status
    int iter;
    for(iter = 1; i < nprocs; i++)
    procs[iter] = true;


    while(!exit){

    if(procs_count == 0 && is_empty(work_pool)){
        secs += MPI_Wtime();
        exit = true;
        printf("No solution\n");
        //printf("Elapsed time: %12.6f (s)\n", secs);
        fflush(stdout);
    }

    // Block until receive that a message as been sent.
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, WORLD ,&status);

    // Slave is availave to do some work.
    if(status.MPI_TAG == ASK_FOR_WORK){
        MPI_Recv(0, 0, MPI_INT, MPI_ANY_SOURCE, ASK_FOR_WORK, WORLD, &status2);

        // Check if there is any work to be done.
        if(!is_empty(work_pool)){
            int nsize;
            int * matrix;
            work_pool = pop(work_pool, &nsize, &matrix);
            MPI_Send(matrix, nsize * nsize, MPI_INT, status.MPI_SOURCE, START_WORK, WORLD );
            procs[status.MPI_SOURCE] = true;
        } else {
            // Terminate the process
            MPI_Send(0, 0, MPI_INT, status.MPI_SOURCE, STOP_WORK, WORLD);
            procs[status.MPI_SOURCE] = false;
            procs_count--;
        }

    } else if (status.MPI_TAG == SOLUTION_FOUND){
        secs += MPI_Wtime();

        MPI_Get_count(&status, MPI_INT, &size);
        int * matrix_solution = malloc(size * sizeof(int));
        int n = (int) sqrt((double) size);

        MPI_Recv(matrix_solution, size, MPI_INT, MPI_ANY_SOURCE, SOLUTION_FOUND, WORLD ,&status);
        exit = true;

        on_solution_found(n, matrix_solution, secs);

        int i;
        for(i = 1; i < nprocs; i++){
            if(procs[i]){
                MPI_Send(0, 0, MPI_INT, i,  STOP_WORK, WORLD);
                procs[i] = false;
                procs_count--;
            }
        }
        free(matrix_solution);

    } else if (status.MPI_TAG == NO_SOLUTION_FOUND){
         MPI_Recv(0,0,MPI_INT, MPI_ANY_SOURCE, NO_SOLUTION_FOUND, WORLD, &status2);

        }
    }
}

void slave() {
    int rank, size;
    bool stopped = false;
    MPI_Status status , status2;
    MPI_Comm_rank(WORLD, &rank);

    do{
        //Request master for a job
        MPI_Send(0, 0, MPI_INT, 0, ASK_FOR_WORK, WORLD);

    MPI_Probe(0, MPI_ANY_TAG, WORLD, &status);

        if(status.MPI_TAG == START_WORK){
            int size;
            MPI_Get_count(&status, MPI_INT, &size);
            int n = (int) sqrt((double) size);
            int * partial_matrix = malloc(size * sizeof(int));

            MPI_Recv(partial_matrix, size, MPI_INT, 0, START_WORK, WORLD, &status2);

            //Solve the puzzle
            Puzzle * puzzle = malloc(sizeof(Puzzle));
            puzzle->n = n;
            puzzle->root_n = (int) sqrt((double) n);
            puzzle->depth = 1;
            puzzle->matrix = partial_matrix;

            if(solve(puzzle)){
                MPI_Send(puzzle->matrix, puzzle->n * puzzle->n, MPI_INT, 0, SOLUTION_FOUND, WORLD);
            } else {
                MPI_Send(0, 0, MPI_INT, 0, NO_SOLUTION_FOUND, WORLD);
            }

            cleanPuzzle(puzzle);

        } else if (status.MPI_TAG == STOP_WORK){
            MPI_Recv(0,0, MPI_INT, 0, STOP_WORK, WORLD, &status2);
            stopped = true;
        ORDER_STOP_WORKING = true;
        return;
        }

    } while (!stopped);

}


/**
 * Attemp to solve the sudoku puzzle using backtracking.
 *
 * @param puzzle Sudoku puzzle data structure.
 * @return Returns true if the sudoku has a solution.
 */
bool solve(Puzzle * puzzle){
    if(ORDER_STOP_WORKING){
    return false;
    }

    int i, row = 0, column = 0;
    if (!find_empty(puzzle, &row, &column)){
        return true;
    }

    for (i = 1; i <= puzzle->n; ++i){
    // Check if number can be placed in a cell
        if (is_valid(puzzle, row, column, i)){
            puzzle->matrix[row * puzzle->n + column] = i;
            puzzle->depth++;

            if (solve(puzzle)){
                return true;
            }

            puzzle->matrix[row * puzzle->n + column] = 0;
        }
    }
    return false;
}

/**
 * Print the puzzle matrix.
 *
 * @param puzzle Sudoku puzzle data structure.
 */
void print_puzzle(Puzzle * puzzle){
    if (puzzle != NULL) {
        int n = puzzle->n;
        int i, j;
        for (i = 0; i < n; ++i){
            for (j = 0; j < n; ++j){
                printf("%d ", puzzle->matrix[i*n + j]);
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
        }
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
            if (puzzle->matrix[(i + row) * puzzle->n + (j + column)] == number){
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
        if(puzzle->matrix[i* puzzle->n + column] == number){
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
        if (puzzle->matrix[row * puzzle->n + i] == number){
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
            if (puzzle->matrix[*row* puzzle->n + *column] == 0){
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
Puzzle * copy_puzzle(Puzzle * puzzle) {
    if (puzzle == NULL) {
        return NULL;
    }

    Puzzle * copy = malloc(sizeof(Puzzle));
    copy->root_n = puzzle->root_n;
    copy->n = puzzle->n;
    copy->depth = puzzle->depth;
    copy->matrix = (int*) malloc(puzzle->n * puzzle->n * sizeof(int));

    int i,j;
    for (i = 0; i < puzzle->n; ++i){
        for (j = 0; j < puzzle->n; ++j){
            copy->matrix[i * puzzle->n + j] = puzzle->matrix[i * puzzle->n + j];
        }
    }
    return copy;
}

int * copy_matrix(int n , int * matrix){
    int * copy = malloc( n * n * sizeof(int));
    int i,j;
    for(i = 0; i < n; i++){
        for(j = 0; j < n; j++){
            int index = i * n + j;
            copy[index] = matrix[index];
        }
    }
    return copy;
}

/**
 * Free's a Sudoku puzzle structure.
 *
 * @param puzzle Sudoku puzzle data structure.
 */
void cleanPuzzle (Puzzle * puzzle) {
    if (puzzle != NULL) {
        int n = puzzle->n;

        free(puzzle->matrix);
        free(puzzle);
    }
}

void on_solution_found(int size, int * matrix, double secs) {
    debug_matrix(size, matrix);
    //printf("Elapsed time: %12.6f (s)\n", secs);
    fflush(stdout);
}

void debug_matrix(int size, int * matrix){
    if (matrix != NULL) {
        int n = size;
        int i, j;
        for (i = 0; i < n; ++i){
            for (j = 0; j < n; ++j){
                printf("%d ", matrix[i*n + j]);
                fflush(stdout);
            }
            printf("\n");
            fflush(stdout);
        }
    }
}



void init(struct Node * head){
    head = NULL;
}

bool is_empty(struct Node * top){
    return (top == NULL) ? true : false;
}

struct Node * push(struct Node * head, int n, int * matrix){
    struct Node * tmp = (struct Node *) malloc(sizeof(struct Node));
    tmp->n = n;
    tmp->matrix = matrix;
    tmp->next = head;
    head = tmp;
    return head;
}

struct Node * pop(struct Node * head, int * n, int ** matrix){
    struct Node * tmp = head;
    *n = head->n;
    *matrix = head->matrix;
    head = head->next;
    free(tmp);
    return head;
}
