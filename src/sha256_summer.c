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

// Temp registers, used to temporarily hold the values of the working registers
// during processing.
uint32_t tempRegisters[8];

MsgBlock msgBlock;

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

    shaProcessData();

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

/**
 * Actually performs the SHA-256 calculation.
 * Updates the working registers.  By the end of the function, the working registers are 
 * updated to the final hash.
 */
void shaProcessData() {
    bool fileEndReached = false;
    long bytesRemaining = fileSize;
    int blocksRemaining = blocksNeeded;
    int BLOCK_SIZE = 16;

    int currentBufferIndex = 0;

    // Initialize the working registers to their default values
    for (int i = 0; i < 8; i++) {
        workingRegisters[i] = squareConst[i];
    }

    // While through the file until we get to the last block.  The last block is treated special,
    // as it includes padding and length encoding.
    while (bytesRemaining > 64) {
        for(int i = 0; i < 16; i++) {
            msgBlock.msgSchedule[i] = messageBuffer[currentBufferIndex];
            currentBufferIndex++;
        }
        bytesRemaining -= 64;

        generateMsgSchedule(&msgBlock);

        // Test code
        printf("Message Schedule:\n");
        for (int i = 0; i < 64; i++) {
            printf("%d: 0x%08x\n", i, msgBlock.msgSchedule[i]); 
        }

        printf("\n\n");

        // Process the message schedule here.
    }

    // At this point, we're at either the last or second to last (if no room for length encoding)
    // block.
    int dataEnd = fileWordSize - currentBufferIndex;
    printf("Words remaining: %d\n", dataEnd);
    for (int i = 0; i < dataEnd; i++) {
        msgBlock.msgSchedule[i] = messageBuffer[currentBufferIndex];
        currentBufferIndex++;
    }

    generateMsgSchedule(&msgBlock);

    // Test code
    printf("Message Schedule:\n");
    for (int i = 0; i < 64; i++) {
        printf("%d: 0x%08x\n", i, msgBlock.msgSchedule[i]); 
    }

    printf("\n\n");
}

/**
 * Only the first 16 registers of the message block contain actual data, the other 48
 * registers are initialized to values based on the first 16 registers.  Given a block
 * with the first 16 registers filled out, this fills out the rest of the schedule.
 */
void generateMsgSchedule(MsgBlock *msgBlock) {
    for (int i = 16; i < 64; i++) {
        msgBlock -> msgSchedule[i] = lowSig1(msgBlock -> msgSchedule[i - 2]) + 
                                   msgBlock -> msgSchedule[i - 7] + 
                                   lowSig0(msgBlock -> msgSchedule[i - 15]) +
                                   msgBlock -> msgSchedule[i - 16];
    } 
}

/**
 * Returns true if the system is big endian, otherwise false.
 *
 * NOTE: I have no idea why, but this appears to be backwards.  If I follow this,
 *       the bytes end up in reverse order on the buffer.
 */
bool checkEndianness() {
    int num = 1;
    
    if (*((char *)&num) == 1) {
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

    // The length of the file in 32 bit words.  We may add an additional
    // word at the end if the last byte is truncated by integer division.
    fileWordSize = (fileSize / 4);
    if ((fileSize % 4) != 0) {
        fileWordSize++;
    }

    printf("File word size: %ld\n", fileWordSize);

    /* TODO: Do this one block at a time instead of reading in the entire file to save on memory.  
             Having difficulty with fread, lets get this working then optimize. */

    messageBuffer = malloc(sizeof(uint32_t) * fileWordSize);
    fread(messageBuffer, sizeof(uint32_t), fileWordSize, filePointer);
    fclose(filePointer);

    bool isBigEndian = checkEndianness();

    // Reverse the buffer if the file system is big endian
    if (!isBigEndian) {
        for(int i = 0; i < fileWordSize; i++) {
            messageBuffer[i] = __builtin_bswap32(messageBuffer[i]);
        }
    }

    bitsInLastBlock = (fileSize * 8) % 512;
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
    uint32_t c = x >> 3;                // SHR 3

    return (a ^ b ^ c);
}

/**
 * Lowercase sigma one function
 */
uint32_t lowSig1(uint32_t x) {
    
    uint32_t a = (x >> 17) | (x << 15);  // ROTR 17
    uint32_t b = (x >> 19) | (x << 13);  // ROTR 19
    uint32_t c = x >> 10;                // SHR 10

    return (a ^ b ^ c);
}

/**
 * Uppercase sigma zero function
 */
uint32_t upSig0(uint32_t x) {
    
    uint32_t a = (x >> 2) | (x << 30);   // ROTR 2
    uint32_t b = (x >> 13) | (x << 19);  // ROTR 13
    uint32_t c = (x >> 22) | (x << 10);  // ROTR 22

    return (a ^ b ^ c);
}

/**
 * Uppercase sigma one function
 */
uint32_t upSig1(uint32_t x) {

    uint32_t a = (x >> 6) | (x << 26);   // ROTR 6
    uint32_t b = (x >> 11) | (x << 21);  // ROTR 11
    uint32_t c = (x >> 25) | (x << 7);   // ROTR 25

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
