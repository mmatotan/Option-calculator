#!/bin/bash
gcc -std=c99 $(~/opt/python-3.8.5/bin/python3-config --cflags --embed) -o option_calculator option_calculator.c $(~/opt/python-3.8.5/bin/python3.8-config --embed --ldflags) -Xlinker -export-dynamic
./option_calculator
