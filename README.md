# Option-calculator
## About
This program is a part of a bachelor's thesis which can be found as a pdf file in this repository.
## Description
This program is used as a demonstration of parallelized option calculation in real time. Variables needed for option calculation are downloaded via an API in a Python subprogram and passed to the C program which then calculates the values of options. It's main purpose is to visualize the speedup in performace in HPC machines, where using multiple cores, in a context of a program, threads, is faster than using the single core or thread. To run the following Python dependencies are required: numpy, scipy, yahoo-fin, pandas. To run the program, run the ./launch.sh command in linux terminal to compile the program and then run the compiled file.
## Abstract of the bachelor's thesis
In this bachelor's thesis high performance computing meets the world of finance on an example of computing the financial derivative options using a program that applies the parallelization of code on a larger number of processors of a supercomputer. The results of program execution are compared with serial code execution on one processor.
