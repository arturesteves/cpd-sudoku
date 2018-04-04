#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

// get the size of elements on an array
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

typedef int bool;
#define false 0
#define true 1

struct Puzzle {
    int depth;
	int root_n;
	int n;
	int ** matrix;
};
typedef struct Puzzle Puzzle;



///////////////////////////////////////////////////////////
//// Priority Queue
///////////////////////////////////////////////////////////
 
typedef struct {
    long double priority;
    Puzzle * puzzle;
} Task;
 
typedef struct {
    Task *tasks;
    int len;
    int size;
} Queue;
 
void push (Queue *h, long double priority, Puzzle * puzzle) {
    #pragma omp critical 
    {
        if (h->len + 1 >= h->size) {
            h->size = h->size ? h->size * 2 : 4;
            h->tasks = (Task *)realloc(h->tasks, h->size * sizeof (Task));
        }
        int i = h->len + 1;
        int j = i / 2;
        while (i > 1 && h->tasks[j].priority > priority) {
            h->tasks[i] = h->tasks[j];
            i = j;
            j = j / 2;
        }
        h->tasks[i].priority = priority;
        h->tasks[i].puzzle = puzzle;
        h->len++;
    }
}
 
Puzzle *pop (Queue *h) {
    Puzzle *puzzle;
    #pragma omp critical 
    {
        int i, j, k;
        
        if (!h->len) {
            puzzle = NULL;
        }
        else {
            Puzzle *puzzle = h->tasks[1].puzzle;
         
            h->tasks[1] = h->tasks[h->len];
            long double priority = h->tasks[1].priority;
         
            h->len--;
         
            i = 1;
            while (1) {
                k = i;
                j = 2 * i;
                if (j <= h->len && h->tasks[j].priority < priority) {
                    k = j;
                }
                if (j + 1 <= h->len && h->tasks[j + 1].priority < h->tasks[k].priority) {
                    k = j + 1;
                }
                if (k == i) {
                    break;
                }
                h->tasks[i] = h->tasks[k];
                i = k;
            }
            h->tasks[i] = h->tasks[h->len + 1];
        }

    }
    return puzzle;
}


// code based on: https://rosettacode.org/wiki/Priority_queue#C
// int main () {
//     heap_t *h = (heap_t *)calloc(1, sizeof (heap_t));
//     push(h, 3, "Clear drains");
//     push(h, 4, "Feed cat");
//     push(h, 5, "Make tea");
//     push(h, 1, "Solve RC tasks");
//     push(h, 2, "Tax return");
//     int i;
//     for (i = 0; i < 5; i++) {
//         printf("%s\n", pop(h));
//     }
//     return 0;
// }

///////////////////////////////////////////////////////////
//// Priority Queue
///////////////////////////////////////////////////////////






///////////////////////////////////////////////////////////
//// Global Variables
///////////////////////////////////////////////////////////

bool solution_found = false; // no solution found at the beginning
Queue * queue = (Queue *) calloc(1, sizeof (Queue));




/**
* Print the puzzle matrix.
* @param puzzle Sudoku puzzle data structure.
*/
void print_puzzle(FILE * file,Puzzle * puzzle){
	int n = puzzle->n;
	int i,j;
	for (i = 0; i < n; ++i){
		for (j = 0; j < n; ++j){
			fprintf(file,"%d ", puzzle->matrix[i][j]);
		}
		fprintf(file,"\n");
	}
}

/**
* Print the puzzle matrix.
* @param puzzle Sudoku puzzle data structure.
*/
void debug_puzzle(Puzzle * puzzle){
    if (puzzle == NULL) {
        printf("NULL PUZZLE!");
        return;
    }
    int n = puzzle->n;
    int slots = (n + n - 1);
    char buffer[slots + 1];
    memset(buffer, '-', slots);    // init all positions with '-'
    buffer[slots] = '\0';          // define end of string
    printf("%s\n",buffer);
    printf("Puzzle:\n");
    printf("%s\n",buffer);
	int i, j;
	for (i = 0; i < n; ++i){
		for (j = 0; j < n; ++j){
			printf("%d ", puzzle->matrix[i][j]);
		}
		printf("\n");
	}
    printf("%s\n",buffer);
}

void debug_sucessors(Puzzle * sucessorsArr) {
    int n = NELEMS(sucessorsArr);
    int i;
    for (i = 0; i < n; ++i){
  //      if ((sucessorsArr[i])->n == 0) {  // check if sucessor was initialized with usefull values.
            //debug_puzzle(sucessorsArr[i]);
    //    }
        printf("\n,\n");
	}
}

// size: N of the matrix, the number is the number of the succesor
// int size,
// todo
int getPriority(int size, int number, int level) {
    if (level == 0) {
        return 0;
    } else {
        int p;
        // ainda nao e isto.
        int i;
        for (i = 0; i < level; i++) {
            p += size * number;
        }
        return p - level;
    }
}


/*
//todo: inserir mtual exclusiob
void insertionSort(Task * tasksArr, Task * task) {
  //#pragma omp critical {
    int n = NELEMS(tasksArr);
    int begin = n;
    for (int i = 0; i < n; i++) {
        //if (tasksArr[i].priority < task.priority) {
            begin = i + 1;
            break;
        //}
    }
    if (begin == n) {
        //tasksArr[n] = task;
        // no shift needed
    } else {
        // manually shift 
        for (int j = n; n > begin; j--) {
            tasksArr[j] = tasksArr[j-1];
        }
        //tasksArr[begin - 1] = task;
    }
  //}
}
*/


Puzzle * copy(Puzzle * puzzle) {
    if (puzzle == NULL) {
        return NULL;
    }
    Puzzle * copyPuzzle = malloc(sizeof(Puzzle));
    copyPuzzle->depth = puzzle->depth;
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


void cleanPuzzleArr (Puzzle * puzzlesArr) {
    int n = NELEMS(puzzlesArr);
    int i;
  	// Free memory   
	for (i = 0; i <  n ; i++){
		//cleanPuzzle(puzzlesArr[i]);
	}
    free (puzzlesArr);
}

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

int getAvailablePosition(Puzzle ** arr){
    int n = NELEMS(arr);
    int i;
    for(i = 0; i < n; i++){
        //if(arr[i] == NULL) {
            return i;
        //}
   }
   return -1;
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

// return array with sucessors
bool generateSucessors(Puzzle * puzzle) {
    
    int n = puzzle->n;
    int pos, row, col, i; 
    bool hasSucessors = false;

    find_empty(puzzle, &row, &col);

    for(i = 1; i <= n; i++) {
        Puzzle * sucessor = copy (puzzle);
        sucessor->matrix[row][col] = i;
        puzzle->depth 

        push(queue, 1, sucessor);     // add task to priority queue. // replace 1 by getPriority (state, ...)
        debug_puzzle (sucessor);
    }

    return hasSucessors;
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
    
    push(queue, 1, puzzle);

    
    
    #pragma omp parallel 
    {
        do 
        {
            Puzzle * puzzle = pop(queue);
            if(puzzle != NULL) {

                if(is_valid(puzzle)) {
                    //todo - return without returning
                    solution_found = generateSucessors(puzzle);
                }
            }   
            
        } while(!solution_found);
        
        printf("Done");
    }
    
    
    
    
    //Task * tasksArr = malloc(size * sizeof(Task));
    //for (i = 0; i < n; i++) {
        //sucessorsArr[i] = createEmptyPuzzle(sucessorsArr[i]);
        //    createEmptyTask(&tasksArr[i]);
        //tasksArr[i] = createEmptyTask();
    //}
    
    //generateSucessors (puzzle, sucessorsArr);
    
    //insertionSort (tasks, task);  // need to create a task
    
    //debug_sucessors (sucessorsArr);


    //////////////////////////////////////////////////////////
    ////// END
    //////////////////////////////////////////////////////////
    
    /*
	if(solve(puzzle)){

		// Write solution to .out file.
		char * name_out;
		
		// Split file name
		filename[strlen(filename) - 3 ] = '\0';
		name_out = (char *) malloc(sizeof(char) * (strlen(filename) + 4));
		strcpy(name_out, filename);
		strcat(name_out, "serial.out");

		// Open file in write mode
		file_output = fopen(name_out, "w");
		print_puzzle(file_output, puzzle);
		// Close output file
		fclose(file_output);
	} else {
		printf("No solution\n");
	}

    */

	double end = omp_get_wtime();
	printf("Elapsed time: %f (s)\n", end - start);
	return EXIT_SUCCESS;
}