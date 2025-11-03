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

const int SHA_BLOCK_SIZE_BYTES = 64;

FILE* filePointer;

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
// These are initialized to the square constants on program instantiation
uint32_t workingRegisters[8] = {squareConst[0], squareConst[1], squareConst[2],
                                squareConst[3], squareConst[4], squareConst[5],
                                squareConst[6], squareConst[7]};

// Temp registers, used to temporarily hold the values of the working registers
// during processing.
uint32_t tempRegisters[8];

// Temporary values, used during compression.
uint32_t T1;
uint32_t T2;

/**
 * Program main
 */
void main(int argc, char *argv[]) {
    checkProgramArgValidity(argc);
    analyzeFile(filePointer, argv[1]);
    shaProcessFile(filePointer, argv[1]);
    printWorkingRegisters();
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
void analyzeFile(FILE* filePointer, char* filePath) {
    filePointer = fopen(filePath, "rb");
    if (filePointer == NULL) {
        printf("Error opening file: %s\nExiting.\n\n", filePath);
        exit(3);
    }

    // Seek to the end of the file.  This is used to determine the size of 
    // the file.
    if (fseek(filePointer, 0, SEEK_END) != 0) {
        printf("Error reading file: %s\nExiting.\n\n", filePath);
        exit(3);
    }

    // The length of the file in bytes
    fileSize = ftell(filePointer);
    rewind(filePointer);
    fclose(filePointer);

    // The length of the file in 32 bit words.  We may add an additional
    // word at the end if the last byte is truncated by integer division.
    fileWordSize = (fileSize / 4);
    if ((fileSize % 4) != 0) {
        fileWordSize++;
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

    //printf("File Size: %ld bytes\n", fileSize);
    //printf("File word size: %ld\n", fileWordSize);

    //printf("Message blocks needed: %d\nBits in the last block: %d\nPadding"
    //       " needed (without length encoding): %d\n\n", blocksNeeded, 
    //       bitsInLastBlock, paddingNeeded);
}

/**
 * Performs the SHA-256 algorithm on the param file.
 *
 * @param filePointer file pointer to the file to process
 * @param filePath path to the file to process
 */
void shaProcessFile(FILE* filePointer, char* filePath) {
    filePointer = fopen(filePath, "rb");
    long long bytesRemaining = fileSize;
    int bytesRead = 0;

    bool eofReached = false;
    bool fileStopByteAdded = false;
    bool fileSizeEncodingAdded = false;
    bool lastBlock = false;

    // Buffer for reading the file in
    uint8_t fileReadBuffer[512];

    MsgBlock msgBlock;
    MsgSchedule msgSchedule;

    // Read the file in block by block, processing each block as it is read
    // Six possible outcomes in the read phase:
    //  1. SHA_BLOCK_SIZE can be read without reaching EOF: in which case just read the bytes
    //     in and process them.
    //  2. File ends before SHA_BLOCK_SIZE, but 'file_end + 1 byte (file stop indicator) + 8 bytes
    //     (file size encoding)' is less than SHA_BLOCK_SIZE: in which case read the file in, append
    //     0x80 to the byte immediately following EOF, and process it, flag that this is the last block.
    //  3. File ends at SHA_BLOCK_SIZE EXACTLY: in which case read the file in and process it.  We'll
    //     need one more block where the first byte is a file stop indicator.
    //  4. File ends before SHA_BLOCK_SIZE, but 'file_end + 1 byte (file stop indicator) + 8 bytes
    //     (file size encoding) is greater than SHA_BLOCK_SIZE: in which case read the file in, append
    //     0x80 to the byte immediately following EOF, and process it.  We'll need one more block with
    //     file size encoding only.
    //  5. EOF already reached, but file stop byte has not been appended: generate a block with byte 0
    //     being the file stop byte (0x80).  This occured because case 3 was hit previously.  Flag that
    //     this is the last block.
    //  6. EOF already reached, file stop byte has already been appended: generate a block of all 0x00,
    //     and flag that this is the last block.  This occured because case 4 was hit previously.
    //     This block is only to include file size encoding.  Flag that this is the last block.
    do {
        bytesRead = 0;

        if (bytesRemaining > SHA_BLOCK_SIZE_BYTES) {
            // Cases one, just read the file in
            //printf("Here in case 1\n");
            for (int i = 0; i < SHA_BLOCK_SIZE_BYTES; i++) {
                fread(&fileReadBuffer[i], sizeof(uint8_t), 1, filePointer);
                bytesRead++;
                bytesRemaining--;
            }
        }else if (!eofReached && ((SHA_BLOCK_SIZE_BYTES - bytesRemaining) > 8)) {
            // Cases two, read the file in, apply the stop bit as needed
            //printf("Here in case 2\n");
            for (int i = 0; i < SHA_BLOCK_SIZE_BYTES; i++) {
                if (bytesRemaining > 0) {
                    fread(&fileReadBuffer[i], sizeof(uint8_t), 1, filePointer);
                    bytesRead++;
                    bytesRemaining--;
                }else if (!fileStopByteAdded) {
                    eofReached = true;
                    fileReadBuffer[i] = 0x80;
                    fileStopByteAdded = true;
                    bytesRead++;
                    bytesRemaining--;
                }else {
                    fileReadBuffer[i] = 0x00;
                    bytesRemaining--;
                }
            }
            fileSizeEncodingAdded = true;
            lastBlock = true;
        }else if (!eofReached && ((SHA_BLOCK_SIZE_BYTES - bytesRemaining) == 0)) {
            // Case three, just read in the file, make a block, and process it
            //printf("Here in case 3\n");
            for (int i = 0; i < SHA_BLOCK_SIZE_BYTES; i++) {
                fread(&fileReadBuffer[i], sizeof(uint8_t), 1, filePointer);
                bytesRead++;
                bytesRemaining--;
            }
            eofReached = true;
        }else if (!eofReached && ((SHA_BLOCK_SIZE_BYTES - bytesRemaining) < 8)) {
            // Case four, we have enough room for the file stop indicator, but not
            // enough room for file size encoding.  Read the file, append the file
            // stop indicator, but fileSizeEncoding will be handled in the next block.
            //printf("Here in case 4\n");
            for (int i = 0; i < SHA_BLOCK_SIZE_BYTES; i++) {
                if (bytesRemaining > 0) {
                    fread(&fileReadBuffer[i], sizeof(uint8_t), 1, filePointer);
                    bytesRead++;
                    bytesRemaining--;
                }else if (!fileStopByteAdded) {
                    eofReached = true;
                    fileReadBuffer[i] = 0x80;
                    fileStopByteAdded = true;
                    bytesRead++;
                    bytesRemaining--;
                }else {
                    fileReadBuffer[i] = 0x00;
                    bytesRemaining--;
                }
            }
            eofReached = true;
        }else if (eofReached && !fileStopByteAdded) {
            // Case five, we've previously hit case three, add the file stop byte to
            // the first byte of the block, and read in zeros for everything else.
            //printf("Here in case 5\n");
            fileReadBuffer[0] = 0x80;
            bytesRemaining--;
            fileStopByteAdded = true;
            for (int i = 1; i < SHA_BLOCK_SIZE_BYTES; i++) {
                fileReadBuffer[i] = 0x00;
                bytesRemaining--;
            }
            fileSizeEncodingAdded = true;
            lastBlock = true;
        }else if (eofReached && fileStopByteAdded) {
            // Case six, we've previously hit case four, we just need a block of
            // all zeros at this point
            //printf("Here in case 6\n");
            for (int i = 0; i < SHA_BLOCK_SIZE_BYTES; i++) {
                fileReadBuffer[i] = 0x00;
                bytesRemaining--;
            }
            fileSizeEncodingAdded = true;
            lastBlock = true;
        }

        generateMsgBlock(fileReadBuffer, bytesRead, lastBlock, &msgBlock);
        generateMsgSchedule(&msgBlock, &msgSchedule);
        shaProcessMsgSchedule(&msgSchedule);

    }while (!eofReached && !fileStopByteAdded && !fileSizeEncodingAdded);

    fclose(filePointer);
}

/**
 * Generates a 512 bit (16 word) message block from the param byte buffer.
 * If the this block is the last one in the message, appends the message
 * length as an unsigned 64 bit integer to the end of the block.
 *
 * @param byteBuffer Byte buffer to use
 * @param bufferLength length of the param byte buffer in bytes
 * @param lastBlock true if this is the last block of the message, otherwise false
 * @param MsgBlock pointer to fill out for result
 */
void generateMsgBlock(uint8_t* byteBuffer, int bufferLength, bool lastBlock, 
        MsgBlock *msgBlock) {

    // We know we're setting all registers in the block every time, so no need to
    // initialize anything to 0.

    int byteCount = 0;
    int wordCount = 0;
    if (lastBlock) {
        // Since this is the last block, only fill the first 14 words
        for (int i = 0; i < 14; i++) {
            for (int j = 0; j < 4; j++) {
                if (byteCount < bufferLength) {
                    // If there are bytes remaining, bitshift them into the word
                    msgBlock->blockWords[wordCount] = 
                        (msgBlock->blockWords[wordCount] << 8) | byteBuffer[byteCount];
                    byteCount++;
                } else {
                    // Otherwise just bitshift zeros into the word
                    msgBlock->blockWords[wordCount] = (msgBlock->blockWords[wordCount] << 8);
                }
            }
            wordCount++;
        }

        // Since this is the last block, the last two words (8 bytes/64 bits) are the 
        // original length of the file in bits, as an unsigned 64 bit integer.
        msgBlock->blockWords[14] = 0x00;
        msgBlock->blockWords[15] = 0x00;

        uint64_t fileLengthEnc = (uint64_t)fileSize * 8;
        msgBlock->blockWords[14] = (fileLengthEnc >> 32) | msgBlock->blockWords[14];
        msgBlock->blockWords[15] = fileLengthEnc | msgBlock->blockWords[15];
    } else {
        // If this isn't the last block, just copy the byte buffer into the message block
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 4; j++) {
                msgBlock->blockWords[wordCount] = 
                    (msgBlock->blockWords[wordCount] << 8) | byteBuffer[byteCount];
                byteCount++;
            }
        wordCount++;
        }
    }
}

/**
 * Only the first 16 registers of the message block contain actual data, the other 48
 * registers are initialized to values based on the first 16 registers.  Given a block
 * with 16 registers filled out, this fills out the rest of the schedule.
 *
 * @param msgBlock object to generate the schedule for
 * @param msgSchedule object to fill out
 */
void generateMsgSchedule(MsgBlock *msgBlock, MsgSchedule *msgSchedule) {
    // We know we'll be filling out the whole schedule, so we don't need to initialize
    // anything to 0.
    // The first 16 words in the schedule are just the message block
    for (int i = 0; i < 16; i++) {
        msgSchedule->scheduleWords[i] = msgBlock->blockWords[i];
    }
    // The remaining 48 words are generated based on the first 16
    for (int i = 16; i < 64; i++) {
        msgSchedule -> scheduleWords[i] = lowSig1(msgSchedule->scheduleWords[i - 2]) + 
                                          msgSchedule->scheduleWords[i - 7] + 
                                          lowSig0(msgSchedule->scheduleWords[i - 15]) +
                                          msgSchedule->scheduleWords[i - 16];
    }
}

/**
 * Performs the SHA-256 algorithm on the param message schedule, updating the 
 * working registers.
 *
 * @param MsgSchedule pointer of data to execute on
 */
void shaProcessMsgSchedule(MsgSchedule *msgSchedule) {
    // Copy all the working registers to the temp registers, to add that
    // data back in after performing compression.
    for (int i = 0 ; i < 8; i++) {
        tempRegisters[i] = workingRegisters[i];
    }

    for (int i = 0; i < 64; i++) {
        T1 = upSig1(workingRegisters[4]) + 
             choice(workingRegisters[4], workingRegisters[5], workingRegisters[6]) + 
             workingRegisters[7] + cubicConst[i] + msgSchedule->scheduleWords[i];
        T2 = upSig0(workingRegisters[0]) + 
             majority(workingRegisters[0], workingRegisters[1], workingRegisters[2]);

        // Shift all registers to the right one place (h registers falls off)
        for (int j = 7; j > 0; j--) {
            workingRegisters[j] = workingRegisters[j - 1];
        }

        workingRegisters[0] = T1 + T2;
        workingRegisters[4] = workingRegisters[4] + T1;
    }

    // Add the temp registers (registers before compression) back into 
    // the working registers
    for(int i = 0; i < 8; i++) {
        workingRegisters[i] = workingRegisters[i] + tempRegisters[i];
    }
}

/**
 * Prints the working registers in hex form
 */
void printWorkingRegisters() {
    for (int i = 0; i < 8; i++) {
        printf("%08x", workingRegisters[i]);
    }
    printf("\n");
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

// SHA-256 primitive functions
/**
 * Lowercase sigma zero function
 */
uint32_t lowSig0(uint32_t x) {

    // Rotate 7 bits to the right (since we know we are using a 32 bit word, 
    // we shift the word seven bits right, and then OR that with the word
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
uint32_t majority(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

