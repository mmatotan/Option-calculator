#!/bin/bash
gcc -I/usr/include/python3.8 -o option_calculator option_calculator.c -lpython3.8 -lm
./option_calculator