//gcc $ME -o $ME.out && ./$ME.out
//gcc -g $ME -o $ME.out && gdb ./$ME.out

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// state[0] is the output
uint8_t s[289];

static inline void fill(int start, const unsigned char data[10]) {
    int idx = 1;
    
    for(int i = 0; i < 10; i++) {
        unsigned char b = data[i];
        for(int j = 0; j < 8; j++) {
            s[idx++] = b & 1;
            b = b >> 1;
        }
    }
}

static inline void lsr(int start, int end) {
    for(int i = end; i > start; i--)
        s[i] = s[i - 1];
}

void trivium_iterate() {
    uint8_t t1 = s[66] ^ s[93];
    uint8_t t2 = s[162] ^ s[177];
    uint8_t t3 = s[243] ^ s[288];
    
    s[0] = t1 ^ t2 ^ t3;
    
    t1 ^= (s[91] & s[92]) ^ s[171];
    t2 ^= (s[175] & s[176]) ^ s[264];
    t3 ^= (s[286] & s[287]) ^ s[69];
    
    lsr(1, 93);
    s[1] = t1;
    lsr(94, 177);
    s[94] = t2;
    lsr(178, 288);
    s[178] = t3;
}

void trivium_initialize(const unsigned char key[10], const unsigned char iv[10]) {
    memset(s, 0, 288 * sizeof(uint8_t));
    fill(1, key);
    fill(94, iv);
    s[286] = s[287] = s[288] = 1;
    
    for(int i = 0; i < 4 * 288; i++)
        trivium_iterate();
}

int main(void) {
    unsigned char key[10] = "HelloWorld";
    unsigned char iv[10] = "HelloWorld";
    trivium_initialize(key, iv);
    for(int i = 0; i < 100; i++) {
        trivium_iterate();
        putchar('0' + s[0]);
    }
    return 0;
}