#include <stdint.h>

// sizeof() = 0x1b0
typedef struct _regdata {
    // These four stays constant. So search "01 00 00 00 14 00 00 00 8C 00 00 00 00 01 00 00" in Recovery.dat works.
    uint32_t version; // 1
    uint32_t sha1Len; // 20
    uint32_t signatureLen; // 140
    uint32_t blkLen; // 256

    uint8_t sha1[20]; // SHA1 hash of the recovery cert (?)
    uint8_t signature[140]; // ??? (incl. user SID in binary)
    uint8_t blk[256]; // RSA'ed + reversed Unicode password, pkcs1 padded
} regdata;

