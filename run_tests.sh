#!/bin/bash

echo "Solve all sudoku puzzles."

for filename in input/*.in
do
	echo "=========================================="
	echo "Puzzle: $filename "
	./sudoku-serial "$filename"
	echo "=========================================="
done