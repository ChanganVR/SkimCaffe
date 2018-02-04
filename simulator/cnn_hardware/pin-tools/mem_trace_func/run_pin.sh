#!/bin/bash
# CXX=g++-5 make
make
rm ooo-dump.out.gz
rm ooo-dump.out
mkfifo ooo-dump.out 
#---------------------------------

gzip -c < ooo-dump.out > ooo-dump.out.gz & 
pin -t obj-intel64/MemAddr.so -- ../src/sudoku/a.out  4 ../src/sudoku/inputs/4by4.txt  

