#!/bin/sh

mkdir -p build
gcc -o build/load_cell_serv.e -pthread src/load_cell_serv.c src/numerical_basics.c src/method_of_least_squares.c src/ads1115.c -lpigpio -lrt -lm
gcc -o build/load_cell_logger.e -pthread src/load_cell_cli.c src/load_cell_logger.c
gcc -o build/load_cell_actor.e -pthread src/load_cell_cli.c src/load_cell_actor.c
gcc -o build/load_cell_visual.e src/teststandui.c src/load_cell_cli.c $(pkg-config --cflags --libs gtk+-3.0) -lm
gcc -o build/output_rewrite.e src/output_rewrite.c
