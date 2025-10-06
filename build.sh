#!/bin/sh

mkdir -p build
gcc -o build/load_cell_serv.e -pthread src/load_cell_serv.c src/numerical_basics.c src/method_of_least_squares.c -lpigpio -lrt
gcc -o build/load_cell_logger.e -pthread src/load_cell_cli.c src/load_cell_logger.c
gcc -o build/load_cell_actor.e -pthread src/load_cell_cli.c src/load_cell_actor.c
