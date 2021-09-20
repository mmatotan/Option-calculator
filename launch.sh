#!/bin/bash
gcc -I/usr/include/python3.9 -o option_calculator option_calculator.c -lpython3.9 -lm -pthread -g -fopenmp
