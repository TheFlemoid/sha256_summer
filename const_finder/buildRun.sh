#!/bin/sh

OUTPUT_BINARY=./bin/const_finder

if [ -f "$OUTPUT_BINARY" ]; then
    rm -f $OUTPUT_BINARY
fi

gcc -o $OUTPUT_BINARY ./const_finder.c -lm

if [ "$?" -eq 0 ]; then
    ./bin/const_finder 64
else
    echo "Build failed."
fi
