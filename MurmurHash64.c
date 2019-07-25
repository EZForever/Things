#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

unsigned long long MurmurHash64(uint64_t key, uint32_t seed) {
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h[2] = {seed, 0};
    uint32_t *k = (unsigned int *)&key;

    k[0] *= m; k[0] ^= k[0] >> r; k[0] *= m;
    h[0] *= m; h[0] ^= k[0];
    k[1] *= m; k[1] ^= k[1] >> r; k[1] *= m;
    h[1] *= m; h[1] ^= k[1];

    h[0] ^= h[1] >> 18; h[0] *= m;
    h[1] ^= h[0] >> 22; h[1] *= m;
    h[0] ^= h[1] >> 17; h[0] *= m;
    h[1] ^= h[0] >> 19; h[1] *= m; 

    return *(uint64_t *)h;
}

int main(void) {
    uint32_t seed;
    uint16_t data[4];
    uint64_t *key = (uint64_t *)data;
    srand(seed = time(0));
    for(int i = 0; i < 4; i++)
        data[i] = rand();
    printf("%016llx %08x -> %016llx\n", *key, seed, MurmurHash64(*key, seed));
    return 0;
}
