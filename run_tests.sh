#!/bin/bash
PROGRAM=$1
NUM_THREADS=${2:-4}


if [[ -n "$PROGRAM" ]]; then
	#Set number of threads
	
	printf "\n*******************************************\n"
	printf "\tSolve all sudoku puzzles.\n\n"

	numfiles=(input/*.in)
	numfiles=${#numfiles[@]}
	echo "$numfiles PUZZLES" 

	if [[ "$PROGRAM" == "sudoku-serial" ]]; then
		echo "MODE: serial"
	elif [[ "$PROGRAM" == "sudoku-omp" ]]; then
		export OMP_NUM_THREADS="$NUM_THREADS"

		echo "MODE: omp parallel"
		echo "NUMBER OF THREADS: $NUM_THREADS"
	fi
	printf "*******************************************\n\n"

	COUNT=1
	for filename in input/*.in
	do
		echo "TEST: $COUNT"
		echo "FILE: $filename "
		COUNT=$((COUNT+1))
		./"$PROGRAM" "$filename"
		echo "=========================================="
	done
else
	echo "Error passing arguments: must pass program name as argument, e.g sudoku-serial ."
fi

