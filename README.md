# SHA-256 Summer
---

An implementation of the SHA-256 algorithm, written in plain C.

I wanted to learn about how the SHA-256 algorithm works, and I thought the best way to do that would be to write an implementation myself.  This only works on files, not on input from stdin.  You can compile this by simply running (from the `/src/` directory):
```
gcc -o build/sha256_summer ./sha256_summer.c
```

The SHA-256 algorithm relies on a number of different constants, known as the `square constants` and the `cubic constants`.  These are used as starting values for various registers.  The constants are spelled out in the FIPS definition of the SHA algorithms (which you can find [here](res/ref/NIST.FIPS.180-4.pdf), however they also defined as the first 32 bits of the fractional component of the cubed (for cubic constants) or square (for square constants) root of the first N prime numbers.  I thought it'd be fun to derive these myself, and you can find implementations of that in the `square_const_finder` and `cubic_cont_finder` directories.

For large (multi GB) files, you'll find that my implementation is quite a bit slower then the production implementation provided by `sha256sum`, found on most UNIX machines.  That implementation has clearly been optimized significantly more then mine has, and while I'm not 100% sure where my bottleneck is, I believe it's in one of two places:
    
1. Conditional Branching: I do a lot of conditional branching in my implementation, especially in my main `do..while` loop where I'm reading and processing data.  This was done because I tried to be memory conscious.  I only ever retain at most one block (512 bytes) of file data in memory at a time.  To achieve this, my program keeps track of how much data is left in the file, and does some branching based on much data is left to process.  Since this happens in my main processing loop, this conditional branch gets hit a lot, especially for large files.  If I wanted to optimize this implementation, I think this would be the first place I'd start.
2. File IO: As stated previously, to be memory concious I only read one block of file data at a time (I believe my total memory footprint is less than 2KB).  This means my main loop boils down to `read 512 bytes of data -> process that data -> update the working registers -> repeat`.  I'm not positive, but I think some of the slowdown could be waiting on file reads, since I'm waiting to read the data from the filesystem on every loop.  This implementation could be sped up by performing the file reads in the background on another thread while we're processing each block.

Outside of the two whitepapers found [here](/res/ref/), I also got a lot of the information to make this from [in3rsha's excellent SHA-256 animation found here](https://github.com/in3rsha/sha256-animation).
