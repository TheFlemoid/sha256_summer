#!/bin/sh

OUTPUT_BINARY=./bin/square_const_finder

if [ -f "$OUTPUT_BINARY" ]; then
    rm -f $OUTPUT_BINARY
fi

gcc -o $OUTPUT_BINARY ./square_const_finder.c -lm

if [ "$?" -eq 0 ]; then
    $OUTPUT_BINARY 8
else
    echo "Build failed."
fi
