/**
 * File:       const_finder.c
 * Author:     Franklyn Dahlberg
 * Created:    02 August, 2025
 * Copyright:  2025 (c) Franklyn Dahlberg
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * There are 64 constants used in the SHA-256 hashing algorithm that are used
 * to "add data" to the message that is being hashed.  These constants are defined
 * as the first 32 bits of the franctional component of the cubed root of the first
 * 64 prime numbers.  These are of course constants and are just defined in the 
 * algorithm, but I thought it would be fun to derive them myself.
 */

// Function declarations
bool isPrime(int testInt);
uint32_t determineShaConstant(int prime);

/**
 * Program main
 */
void main(int argc, char *argv[]) {

    int constantsFound = 0;
    int constantsNeeded = 0;

    if (argc != 2) {
        printf("Pass the number of constants to determine as an argument"
               " to this program.\n");
        printf("\tEg. ./const_finder 64\n");
        printf("Exiting.\n\n");
        exit(2);
    }

    constantsNeeded = atoi(argv[1]);

    int testNum = 2;
	double primeDouble;

    while (constantsFound < primesNeeded) {
        if (isPrime(testNum)) {
            primeDouble = testNum;

            constantsFound++;
            uint32_t shaConstant = determineShaConstant(testNum);
            printf("SHA Constant #%d: Prime %d    Constant:%08x\n", constantsFound, testNum, shaConstant);
        }

        testNum++;
    }
}

/**
 * Determines if the param integer is a prime number.
 *
 * @param testInt integer to test
 * @return true if the param integer is prime, false otherwise
 */
bool isPrime(int testInt) {
    for (int i = 2; i < (testInt / 2) + 1; i++) {
        if (testInt % i == 0) {
            return false;
        }
    }

    return true;
}

/**
 * Determines the SHA constant of the param prime integer.
 *
 * @param prime Prime number to calculate the constant for
 * @return uint32_t of the SHA256 constant of the param prime integer
 */
uint32_t determineShaConstant(int prime) {
	double doublePrime = prime;
	double cubeRoot = cbrt(doublePrime);

    // Get the non fractional component of the cubic root by typecasting the cube root as an integer.
    int nonFracComponent = cubeRoot;     
    double fracComponent = cubeRoot - nonFracComponent;

    // We want the first 32 bits of the fractional component (16 hex chars), and we need to shift
    // fractional component left to get just that amount.  To shift the first x chars of a decimal number,
    // we multiply the number by 10^x.  Similiarly, to shift the first x chars of a hex number, we multiply
    // the number by 16^x.
    double shiftedFracComponent = fracComponent * pow(16, 8);
    uint32_t unsignedFracComp = shiftedFracComponent;

    return unsignedFracComp;
}

