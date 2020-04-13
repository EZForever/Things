#include <windows.h>
#include <cstdint>

static inline void prng(uint16_t* state) {
    *state = 2 * (*state / 181) - 163 * (*state % 181);
}

static inline uint16_t getTimeStamp() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    uint64_t ts;
    ts = (*(uint64_t *)&ft - 0x019db1ded53e8000) / 10000000;
    // 0x019db1ded53e8000 == (FILETIME)(1970-01-01 00:00:00 GMT)
    // ts happens to be the POSIX timestamp of current time
    
    return *(uint16_t *)&ts;
}

void PersiEncrypt(unsigned char *&out, size_t &outLen, const unsigned char *in, size_t inLen, bool doXor) {
    static const uint32_t suffix[3] = { 0 /* ? */, 0x6789dedc, sizeof(suffix) };
    
    uint16_t ts = getTimeStamp();

    outLen = inLen + sizeof(suffix);
    out = new unsigned char[outLen];

    if (doXor) {
        for (size_t i = 0; i < inLen; i++)
            out[i] = in[i] ^ 0xc0;
    } else {
        memcpy(out, in, inLen);
    }

    memcpy(out + inLen, suffix, sizeof(suffix));

    int restLen = outLen;
    int i = restLen - 1;
    while (restLen) {
        prng(&ts);
        out[i] ^= *(uint8_t *)&ts ^ (uint8_t)(outLen - restLen + 3);
        --restLen;
        --i;
    }
}

