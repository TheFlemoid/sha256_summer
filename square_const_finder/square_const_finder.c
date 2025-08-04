/**
 * File:       square_const_finder.c
 * Author:     Franklyn Dahlberg
 * Created:    03 August, 2025
 * Copyright:  2025 (c) Franklyn Dahlberg
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * There are 8 square root constants used in the SHA-256 hashing algorithm that 
 * are used to initialize the state registers during compression.  These 
 * constants are defined as the first 32 bits of the fractional component of 
 * the square root of the first 8 prime numbers.  These are of course constants
 * and are just defined in the algorithm, but I thought it would be fun to 
 * derive them myself.
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
        printf("\tEg. ./square_const_finder 8\n");
        printf("Exiting.\n\n");
        exit(2);
    }

    constantsNeeded = atoi(argv[1]);

    int testNum = 2;
	double primeDouble;

    while (constantsFound < constantsNeeded) {
        if (isPrime(testNum)) {
            primeDouble = testNum;

            constantsFound++;
            uint32_t shaConstant = determineShaConstant(testNum);
            printf("SHA Constant %2d: Prime %-3d    Constant: %08x\n", constantsFound, testNum, shaConstant);
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
	double squareRoot = sqrt(doublePrime);

    // Get the non fractional component of the cubic root by typecasting the cube root as an integer.
    int nonFracComponent = squareRoot;     
    double fracComponent = squareRoot - nonFracComponent;

    // We want the first 32 bits of the fractional component (8 hex chars), and we need to shift the
    // fractional component left to get just that amount.  To shift the first x chars of a decimal number,
    // we multiply the number by 10^x.  Similiarly, to shift the first x chars of a hex number, we multiply
    // the number by 16^x.
    double shiftedFracComponent = fracComponent * pow(16, 8);
    uint32_t unsignedFracComp = shiftedFracComponent;

    return unsignedFracComp;
}

