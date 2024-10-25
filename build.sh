#!/bin/sh
eval cc main.c -Wall -Wextra -Wpedantic -Werror $(pkg-config --libs --cflags raylib) -o game &&\
    ./game && rm game
# chmod 777 build.sh
