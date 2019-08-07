#include <stdint.h>

// sizeof() = 0x184 / 0x1b0 / ???
typedef struct _regdata {
    // Search binary in Recovery.dat works.
    uint32_t version; // 1
    uint32_t sha1Len; // 20
    uint32_t signatureLen; // 96 on Windows XP, 140 on Windows 10, ??? on other Windows
    uint32_t blkLen; // 256

    uint8_t sha1[20]; // SHA1 hash of the recovery cert
    uint8_t signature[signatureLen]; // ??? (incl. user SID in binary)
    uint8_t blk[256]; // RSA'ed + reversed Unicode password, pkcs1 padded
} regdata;

