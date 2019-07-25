#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#   include <io.h>
#   include <fcntl.h>
#endif

// Default key, if none is given
#define DEFAULTKEY 0xBADC0FFEE0DDF00D // 0x0000ab16badd06fell

// From: https://en.wikipedia.org/wiki/Xorshift#xorshift*
uint64_t XorShiftStar64(uint64_t seed) {
    static uint64_t k = DEFAULTKEY;
    if(seed)
        k = seed;
    k ^= k >> 12;
    k ^= k << 25;
    k ^= k >> 27;
    return k * 0x2545F4914F6CDD1D;
}

uint8_t hex2dec(uint8_t hex) {
    if(hex >= 'a' && hex <= 'z')
        return hex - 'a' + 10;
    else if(hex >= 'A' && hex <= 'Z')
        return hex - 'A' + 10;
    else if(hex >= '0' && hex <= '9')
        return hex - '0';
    return hex;
}

void morfi(uint64_t blk, int blkLen) {
    uint8_t *pBlk = (uint8_t *)&blk + blkLen;
    blk ^= XorShiftStar64(0);
    while(--pBlk >= (uint8_t *)&blk) // Little-endian
        putchar(*pBlk);
}

int main(int argc, char* argv[]) {
    uint64_t blk = 0;
    int blkLen = 0, c;
 
    if(argv[1]) {
        char* pSeed = argv[1];
        uint64_t seed = 0;

        while(*pSeed)
            seed = (seed << 4) | hex2dec(*pSeed++);
        XorShiftStar64(seed);
    }
 
    #ifdef _WIN32
        // On Windows, the default mode of stdin is O_TEXT.
        // This will cause EOF when a 0x1a(^Z) is reached.
        setmode(fileno(stdin), O_BINARY);
        // This turns \n into \r\n, causing troubles
        setmode(fileno(stdout), O_BINARY);
    #endif

    while((c = getchar()) != EOF) {
        blk = (blk << 8) | (uint8_t)c; // Fill a block
        blkLen = ++blkLen % 8;
        if(!blkLen) { // A block is full
            morfi(blk, 8);
            blk = 0;
        }
    }
    if(blkLen) // The last block is not fully filled
        morfi(blk, blkLen);

    #ifdef _WIN32
        // Avoid strange console problems
        setmode(fileno(stdin), O_TEXT);
        // Write all data out before becoming weird
        fflush(stdout);
        setmode(fileno(stdout), O_TEXT);
    #endif
    return 0;
}
