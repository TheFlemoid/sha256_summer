/**
 * File:       sha256_summer.c
 * Author:     Franklyn Dahlberg
 * Created:    03 August, 2025
 * Copyright:  2025 (c) Franklyn Dahlberg
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "sha256_summer.h"

FILE* filePointer;
uint32_t *messageBuffer;

long fileSize;     // The length of the file in bytes
long fileWordSize; // The length of the file in 32 bit words 
                   // (also the size of the message buffer)

/**
 * Program main
 */
void main(int argc, char *argv[]) {
    
    checkProgramArgValidity(argc);

    openAndReadFile(filePointer, argv[1]);

    printf ("Bytes read:\n");
    for (int i = 0; i < fileWordSize; i++) {
        printf("%02x ", messageBuffer[i]);
    }

    printf("\n");
    printf("File Size: %ld bytes\n", fileSize);

    free(messageBuffer);
}

/**
 * Checks the arguments provided to the program runtime and verifies they
 * are valid.  If they are invalid, exit the program.
 */
void checkProgramArgValidity(int argc) {
    if (argc != 2) {
        printf("Pass the absolute or relative path to the file to hash as" 
                " an argument to this program.\n");
        printf("\tEg. ./sha256_summer /path/to/file\n");
        printf("Exiting.\n\n");
        exit(2);
    }
}

/**
 * Returns true if the system is big endian, otherwise false.
 */
bool checkEndianness() {
    uint32_t num = 1;
    char *ptr = (char*)&num;

    if (*ptr == 1) {
        printf("Little endian system detected.\n");
        return false;
    } else {
        printf("Big endian system detected.\n");
        return true;
    }
}

void openAndReadFile(FILE* filePointer, char* filePath) {
    filePointer = fopen(filePath, "r");
    if (filePointer == NULL) {
        printf("Error opening file: %s\nExiting.\n\n", filePath);
        exit(3);
    }

    // Seek to the end of the file.  This is used to determine the size of 
    // the message buffer.
    if (fseek(filePointer, 0, SEEK_END) != 0) {
        printf("Error reading file: %s\nExiting.\n\n", filePath);
        exit(3);
    }

    // The length of the file in bytes
    fileSize = ftell(filePointer);
    rewind(filePointer);

    // The length of the file in 32 bit words.  We add an additional word at
    // the end in case of truncation due to integer division.
    fileWordSize = (fileSize / 4) + 1;
    messageBuffer = malloc(sizeof(uint32_t) * fileWordSize);

    fread(messageBuffer, sizeof(uint32_t), fileWordSize, filePointer);
    fclose(filePointer);

    bool isBigEndian = checkEndianness();
    if (checkEndianness) {
        for(int i = 0; i < fileWordSize; i++) {
            messageBuffer[i] = __builtin_bswap32(messageBuffer[i]);
        }
    }
}
