#!/bin/sh

gcc -o ./bin/const_finder ./const_finder.c -lm
./bin/const_finder 64
