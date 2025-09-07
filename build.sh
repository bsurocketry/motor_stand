#!/bin/sh

mkdir -p build
gcc -o build/load_cell_serv.e -pthread src/load_cell_serv.c -lpigpio -lrt
gcc -o build/load_cell_logger.e src/load_cell_cli.c src/load_cell_logger.c
