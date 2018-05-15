# Sudoku Solver
This project presents two different implementations of a Sudoku Solver.  
The solvers are implemented in C language, one version is serial and the other is parallel and uses OpenMP.
	
## Usage
All the commands should be executed on the root directory of the project.  
### Serial Implementation

The **-lm** flag is required when compiling the parallel code to link the math library.  

#### Compile the source code
* Serial
    * `gcc -fopenmp -o sudoku-serial sudoku-serial.c`

* Parallel
    * `gcc -fopenmp -o sudoku-omp sudoku-omp.c -lm`

#### Execute the source code
All the commands can receive the following arguments:  
`input-filename` **required** Name of the input matrix file.on  
`-t` **optional** Indicates that the output should also show the time it took to solve, the time is in seconds.  
`-to` **optional** Indicates that the output should only contain the time it took to solve, the time is in seconds.  
If the `-t`and the `-to` flags are passed as arguments the `-t` will be ignored.

**On Windows**  

* Serial
    * `.\sudoku-serial.exe [input-filename]`  
    * Example: `.\sudoku-serial.exe .\input\9x9.in`
    * Example with flags: `.\sudoku-serial.exe .\input\9x9.in -t`
    * Example with flags: `.\sudoku-serial.exe .\input\9x9.in -to`

* Parallel
    * `.\sudoku-omp.exe [input-filename]`  
    * Example: `.\sudoku-omp.exe .\input\9x9.in` 
	
**On Linux:**  
* Serial
    * `./sudoku-serial [input-filename]`  
    * Example: `./sudoku-serial.exe /input/9x9.in`

* Parallel  
    * `./sudoku-omp.exe [input-filename]`  
    * Example: `./sudoku-omp.exe /input/9x9.in`


#### How to change the number on the parallel version
Use the command line / terminal / powershell to perform the modification.  

Examples showing how to change the number of threads to 2.

**On the command line**  
`set OMP_NUM_THREADS=2`
	
**On the power shell**  
`$env:OMP_NUM_THREADS = 2`

**On the linux terminal**  
`export OMP_NUM_THREADS=2`


#### Input/Ouput format
All the implementations follow the same input and output format.

**Input**  
Accept only 1 command line which should contain the file path that contains the Sudoku instance matrix.  
The format of this file is:  	
* one line with one integer, l = raiz quadrada de N, 2 <= l <= 9 (specifying l avoids the square root in the code...).
* n lines, each with n integers, separated by a space, with values in the interval [0, n], where 0s represent blank positions in the matrix.
	
Input File Example:  
```
2
0 0 3 1
3 0 0 0
4 2 0 0 
1 3 0 2
```
	
	
**Output**  
The output is a matrix in the same format as the input and it's made to the standard stdout and a file with the entrance name .out.
The format of the file is:
* n lines, each with n integers, separated by a space, with values in the interval [1, n].  

In case no solution exists, the program will output "No Solution".  

Output File Example: 
```
2 4 3 1
3 1 2 4
4 2 1 3
1 3 4 2
```

The format os the output to the standard stdou (most likely the terminal):
With no flags:  
Output File Example: 
```
2 4 3 1
3 1 2 4
4 2 1 3
1 3 4 2
```
With `-t` flag:  
Output File Example: 
```
2 4 3 1
3 1 2 4
4 2 1 3
1 3 4 2
Elapsed time: 0.013000 (s)
```

With `-to` flag:
Output File Example: 
```
Elapsed time: 0.013000 (s)
```

## Documentation
In the [docs](docs/) directory is presented a report, [report-omp.pdf](docs/report-omp.pdf) describing the parallel solution using OpenMP. The report describes how the decomposition of the execution flow was made through the threads, how the synchronization and load balancing was performed and what were the concerns. At the end are presented some results regard the inputs on [inputs](inputs/) directory and a brief discussion.

## Contributors
* [Ricardo Morais](https://github.com/moraispgsi)
* [Artur Esteves](https://github.com/arturesteves)
* [Luís Fonseca](https://github.com/nekrotzar)

## License  
Licensed under MIT. See [LICENSE](LICENSE) for more information. 