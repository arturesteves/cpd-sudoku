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
	int root_n;
	int n;
	int ** matrix;
};
typedef struct Puzzle Puzzle;

struct Task {
	int priority;
	Puzzle * puzzle;
};
typedef struct Task Task;


///////////////////////////////////////////////////////////
//// Global Variables
///////////////////////////////////////////////////////////
bool solutionFound = false; // no solution found at the beginning


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
	for (int i = 0; i < n; ++i){
		for (int j = 0; j < n; ++j){
			printf("%d ", puzzle->matrix[i][j]);
		}
		printf("\n");
	}
    printf("%s\n",buffer);
}

void debug_sucessors(Puzzle * sucessorsArr) {
    int n = NELEMS(sucessorsArr);
    for (int i = 0; i < n; ++i){
  //      if ((sucessorsArr[i])->n == 0) {  // check if sucessor was initialized with usefull values.
            //debug_puzzle(sucessorsArr[i]);
    //    }
        printf("\n,\n");
	}
}

// size: N of the matrix, the number is the number of the succesor
// int size,
int getPriority(int size, int number, int level) {
    if (level == 0) {
        return 0;
    } else {
        int p;
        // ainda nao e isto.
        for (int i = 0; i < level; i++) {
            p += size * number;
        }
        return p - level;
    }
}


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

Task * retriveHighestPriorityTask (Task * tasksArr) {
    // retira a task com exclusao mutua e faz um shift de todo o array para o lado esquerdo
}

Puzzle * copy(Puzzle * puzzle) {
    if (puzzle == NULL) {
        return NULL;
    }
    Puzzle * copyPuzzle = malloc(sizeof(Puzzle));
    copyPuzzle->root_n = puzzle->root_n;
    copyPuzzle->n = puzzle->n;
    copyPuzzle->matrix = (int**) malloc(puzzle->n * sizeof(int*));          // alloc space for matrix
    // manual copy
	for (int i = 0; i < puzzle->n; ++i){
        copyPuzzle->matrix[i] = (int * )malloc(puzzle->n * sizeof(int));    // alloc space
        for (int j = 0; j < puzzle->n; ++j){
            copyPuzzle->matrix[i][j] = puzzle->matrix[i][j];                // copy values
        }
	}
    return copyPuzzle;
}


void cleanPuzzleArr (Puzzle * puzzlesArr) {
    int n = NELEMS(puzzlesArr);
  	// Free memory   
	for (int i = 0; i <  n ; i++){
		//cleanPuzzle(puzzlesArr[i]);
	}
    free (puzzlesArr);
}

void cleanPuzzle (Puzzle * puzzle) {
    if (puzzle != NULL) {
        int n = puzzle->n;
        // Free memory   
        for (int i = 0; i <  n ; i++){
            free(puzzle->matrix[i]);
        }
        free(puzzle->matrix);
        free(puzzle);
    }
}

// return array with sucesors
void generateSucessors(Puzzle * puzzle, Puzzle * sucessorsArr) {
    int n = puzzle->n;
    int pos;
    Puzzle state;
    for(int i = 0; i < n; i++) {
       // state = copy (puzzle);  
        for(int j = 0; j < n; j++) {
            //if(state->matrix[i][j] == 0) {
                // empty position
                //state->matrix[i][j] = i;
                //pos = getAvailablePosition (sucessorsArr);
                if (pos != -1) {
                    sucessorsArr[pos] = state;
                }
                //cleanPuzzle(state); // free memory
            //}
        }
    }
}

int getAvailablePosition(Puzzle * arr){
    int n = NELEMS(arr);
    for(int i = 0; i < n; i++){
        //if(arr[i] == NULL) {
            return i;
        //}
   }
   return -1;
}

Puzzle * createEmptyPuzzle (Puzzle puzzle) {
    //Puzzle * puzzle = malloc(sizeof(Puzzle));
    puzzle->root_n = 0;
    puzzle->n = 0;
    puzzle->matrix = NULL;//(int**) malloc(sizeof(int*));          // alloc space for matrix
    return puzzle;
}

Task * createEmptyTask(Task task) {
    //Task * task = malloc(sizeof(Task));
    task->priority = 0;
    task->puzzle = NULL; // (int*) malloc (sizeof(int));  // incompatible point
    return task;
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
	

    //////////////////////////////////////////////////////////
    ////// START
    //////////////////////////////////////////////////////////

    Puzzle * copyPuzzle = copy (puzzle);
    if (copyPuzzle == NULL) {
        printf ("ERROR.");
        return -1;
    }
    printf ("Copy with success.\n");
    debug_puzzle (puzzle);
    debug_puzzle (copyPuzzle);
    copyPuzzle->matrix[0][0] = 2;
    debug_puzzle (puzzle);
    debug_puzzle (copyPuzzle);


    printf("\n\nRoot State:\n");
    debug_puzzle(puzzle);


    // init array of sucessors and array of \s
    Puzzle * sucessorsArr = malloc(n * sizeof(Puzzle));
    //Task * tasksArr = malloc(size * sizeof(Task));
    for (int i = 0; i < size; i++) {
        createEmptyPuzzle(sucessorsArr[i]);
    //    createEmptyTask(&tasksArr[i]);
        //tasksArr[i] = createEmptyTask();
    }
    
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