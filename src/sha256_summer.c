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

bool isBigEndian;
long fileSize;               // The length of the file in bytes
long fileWordSize;           // The length of the file in 32 bit words 
int bitsInLastBlock;         // The number of bits in the last block.
bool lastBlockSizeOverflow;
int blocksNeeded;
int paddingNeeded;           // Bits of padding needed without length encoding.

// Working registers that hold the intermediate hash.  From the FIPS paper:
// Index 0: a
// Index 1: b
// Index 2: c
// Index 3: d
// Index 4: e
// Index 5: f
// Index 6: g
// Index 7: h
uint32_t workingRegisters[8];

// Temporary values
uint32_t T1;
uint32_t T2;

/**
 * Program main
 */
void main(int argc, char *argv[]) {

    checkProgramArgValidity(argc);
    openAndAnalyzeFile(filePointer, argv[1]);

    // Initialize working registers with default values
    //for (int i = 0; i < 8; i++) {
    //    workingRegisters[i] = squareConst[i];
    //}

    printf("File Size: %ld bytes\n", fileSize);

    //shaProcessData(argv[1]);

    //free(workingRegisters);
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

///**
// * Converts the message buffer from an array of unsigned integers to
// * an array of 512 bit message blocks.  Pads the last block with 1 '1'
// * and enough zeroes to make 512-64 (448) bits, and adds 64 bits of
// * message length encoding to the last 64 bits of the last block.
// */
//void generateMessageBlocks(uint32_t *messageBuffer) {
//    int bitsInLastBlock = fileSize % 512;
//    bool lastBlockSizeOverflow = ((512 - bitsInLastBlock) < 64);
//    
//    int blocksNeeded;
//    int paddingNeeded;
//
//    // We need 64 bits of space in the last block to encode the message size.
//    // If there's not 64 bits available, we pad the last block with actual data,
//    // make a new block after that, and pad that out 448 bits to give 64 bits of
//    // space for message size data.
//    if (!lastBlockSizeOverflow) {
//        blocksNeeded = ((fileSize + 64) / 512) + 1;
//        paddingNeeded = 512 - bitsInLastBlock - 64; 
//    } else {
//        blocksNeeded = ((fileSize + 64) / 512) + 2;
//        paddingNeeded = 512 - bitsInLastBlock + 448;
//        bitsInLastBlock = 0;  // Indicating that the last actual block is all
//                              // padding and message length encoding.
//    }
//
//    printf("Message blocks needed: %d\nBits in the last block: %d\nPadding"
//           " needed (without length encoding): %d\n", blocksNeeded, 
//           bitsInLastBlock, paddingNeeded);
//}

/**
 * Actually performs the SHA-256 calculation.
 * Reads the file in block for block and performs the SHA-256 algorithm
 * on each block.  Updates the working registers.  By the end of the
 * function, the working registers are updated to the final hash.
 */
//void shaProcessData(char* filePath) {
//    bool fileEndReached = false;
//    long bytesRemaining = fileSize;
//    int blocksRemaining = blocksNeeded;
//    int BLOCK_SIZE = 16;
//    uint32_t block[BLOCK_SIZE];
//  
//    // We do error checking for the file when we open and analyze it elsewhere 
//    filePointer = fopen(filePath, "rb");
//
//    // Vars used in the below while loop to track where we're at in the file
//    int bytesReadThisBlock = 0;
//    int objectsReadLastRead;
//    int placeInBlock = 0;
//
//    while (!fileEndReached) { 
//
//        // TODO: Instead of doing file math ourselves, I think we can use a while loop and feof checking after
//        // each word read.
//        
//        placeInBlock = 0;
//        objectsReadLastRead = 1; // Set to a nonzero value, since fread will return 0 on EOF
//
//        for (int i = 0; i < BLOCK_SIZE; i++) {
//            block[i] = 0;
//        }
//                                 
//        // Read in a blocks worth of data
//        while (placeInBlock < BLOCK_SIZE && objectsReadLastRead > 0) {
//            //objectsReadLastRead = fread(&block[placeInBlock], sizeof(uint32_t), 1, filePointer);
//            objectsReadLastRead = 1;
//            fread(&block[placeInBlock], sizeof(uint32_t), 1, filePointer);
//            printf("fread return: %d\n", objectsReadLastRead);
//            if (objectsReadLastRead > 0) {
//                placeInBlock++;
//                bytesReadThisBlock += (objectsReadLastRead * 32);
//            }    
//        }
//
//        if (!isBigEndian) {
//            for(int i = 0; i < BLOCK_SIZE; i++) {
//                block[i] = __builtin_bswap32(block[i]);
//            }
//        }
//
//        for (int i = 0; i < BLOCK_SIZE; i++) {
//            printf("0x%08x ", block[i]);
//        }
//        printf("\n");
//    }
//
//    fclose(filePointer);
//}

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

/**
 * Opens and analyzes the file passed in as the argument to 
 * this program. Calculates the number of blocks required to 
 * hash the file, the amount of padding needed, and whether 
 * an extra block is needed with just padding and length 
 * information.
 *
 * @param filePointer pointer to the FILE object to load
 * @param filePath path to the file on the system
 */
void openAndAnalyzeFile(FILE* filePointer, char* filePath) {
    filePointer = fopen(filePath, "rb");
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

    /* TODO: Do this one block at a time instead of reading in the entire file,
          to save on memory.  
          *** Having difficulty with fread, lets get has working then optimize. *** */

    messageBuffer = malloc(sizeof(uint32_t) * fileWordSize);
    fread(messageBuffer, sizeof(uint32_t), fileWordSize, filePointer);
    fclose(filePointer);

    bool isBigEndian = checkEndianness();

    // TODO: Have to swap the endianness on a block for block basis
    if (checkEndianness) {
        for(int i = 0; i < fileWordSize; i++) {
            messageBuffer[i] = __builtin_bswap32(messageBuffer[i]);
        }
    }

    bitsInLastBlock = fileSize % 512;
    lastBlockSizeOverflow = ((512 - bitsInLastBlock) < 64);

    // We need 64 bits of space in the last block to encode the message size.
    // If there's not 64 bits available, we pad the last block with actual data,
    // make a new block after that, and pad that out 448 bits to give 64 bits of
    // space for message size data.
    if (!lastBlockSizeOverflow) {
        blocksNeeded = ((fileSize + 64) / 512) + 1;
        paddingNeeded = 512 - bitsInLastBlock - 64; 
    } else {
        blocksNeeded = ((fileSize + 64) / 512) + 2;
        paddingNeeded = 512 - bitsInLastBlock + 448;
        bitsInLastBlock = 0;  // Indicating that the last actual block is all
                              // padding and message length encoding.
    }

    printf("Message blocks needed: %d\nBits in the last block: %d\nPadding"
           " needed (without length encoding): %d\n", blocksNeeded, 
           bitsInLastBlock, paddingNeeded);
}


// SHA-256 primitive functions
/**
 * Lowercase sigma zero function
 */
uint32_t lowSig0(uint32_t x) {

    // Rotate 7 bits to the right (since we know we are using a 32 bit word, 
    // we shift the word seven bits right, and then or that with the word
    // shifted 25 (32 - 7) bits to the left.
    uint32_t a = (x >> 7) | (x << 25);  // ROTR 7
    uint32_t b = (x >> 18) | (x << 14); // ROTR 18
    uint32_t c = x >> 3;                        // SHR 3

    return (a ^ b ^ c);
}

/**
 * Lowercase sigma one function
 */
uint32_t lowSig1(uint32_t x) {
    
    uint32_t a = (x >> 17) | (x << 15);  // ROTR 17
    uint32_t b = (x >> 19) | (x << 13);  // ROTR 19
    uint32_t c = x >> 10;                        // SHR 10

    return (a ^ b ^ c);
}

/**
 * Uppercase sigma zero function
 */
uint32_t upSig0(uint32_t x) {
    
    uint32_t a = (x >> 2) | (x << 30);
    uint32_t b = (x >> 13) | (x << 19);
    uint32_t c = (x >> 22) | (x << 10);

    return (a ^ b ^ c);
}

/**
 * Uppercase sigma one function
 */
uint32_t upSig1(uint32_t x) {

    uint32_t a = (x >> 6) | (x << 26);
    uint32_t b = (x >> 11) | (x << 21);
    uint32_t c = (x >> 25) | (x << 7);

    return (a ^ b ^ c);
}

/**
 * SHA-256 choice primitive function.
 * The value of the x bit "chooses" whether the resultant bit on
 * the output should be the value of y (if x is zero) or the value
 * of z (if x is 1)
 */
uint32_t choice(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

/**
 * SHA-256 majority primitive function.
 * The value of each bit in the result is based on the majority value
 * of the three bits in that place between the inputs.  For each bit,
 * if at least two inputs have a zero, the result will be zero.  If at
 * least two bits have a one, the result will be one.
 */
uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}
