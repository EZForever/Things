#include <stdint.h>

// All data below are little-endian

// sizeof() == 0x600
typedef struct _userkey {
    // Confirmed via disass
    uint32_t userkeyMagic; // 0x4252534D == 'MSRB'
    uint32_t version; // 1

    // Exported with CryptExportKey

    // PUBLICKEYSTRUC structure
    // https://technet.microsoft.com/zh-cn/learning/aa387453(v=vs.100)
    uint8_t bType; // 7 == PRIVATEKEYBLOB
    uint8_t bVersion; // 2
    uint16_t reserved; // 0
    uint32_t aiKeyAlg; // 0x0000a400 == CALG_RSA_KEYX

    // RSAPUBKEY structure
    // https://technet.microsoft.com/zh-cn/learning/aa387685(v=vs.100)
    uint32_t magic; // 0x32415352 == 'RSA2' == RSA private key
    uint32_t bitlen; // 0x00000800 == 2048
    uint32_t publicExponent; // e == 0x00010001 == 65537

    // Public key
    uint8_t modulus[2048 / 8]; // n

    // Private key
    uint8_t prime1[2048 / 16]; // p
    uint8_t prime2[2048 / 16]; // q
    uint8_t exponent1[2048 / 16]; // d mod (p ¨C 1)
    uint8_t exponent2[2048 / 16]; // d mod (q ¨C 1)
    uint8_t coefficient[2048 / 16]; // (inverse of q) mod p
    uint8_t privateExponent[2048 / 8]; // d

    uint8_t padding[0x164]; // 0
} userkey;

