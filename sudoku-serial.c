#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]){

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
	int puzzle[n][n];


	// Read matrix from the file
	int row = 0, column = 0;
	int cursor;

	while( (cursor = getc(file)) != EOF ){
		switch(cursor){
			case ' ':
				break;
			case '\n':
				row++;
				column = 0;
				break;
			default:
				puzzle[row][column] = cursor - '0';
				column++;
				break;
		}
	}


	// Print parsed matrix
	printf("Input matrix:\n");
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			printf("%d ", puzzle[i][j]);
		}
		printf("\n");
	}


	// Close file
	fclose(file);
	return EXIT_SUCCESS;
}