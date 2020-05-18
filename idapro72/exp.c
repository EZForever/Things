//gcc -O2 $ME -lcrypto -o $ME.out && ./$ME.out
//gcc -g $ME -lcrypto -o $ME.out && gdb ./$ME.out

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// apt install libssl-dev
#include <openssl/sha.h>

#define PWDLEN 11
//#define OLD_ALGORITHM
#define OFF_BY_ONE

#ifdef OLD_ALGORITHM

uint32_t xseed;

void xsrand(uint32_t seed) {
	xseed = seed;
}

uint32_t xrand(void) {
	xseed = xseed * 214013 + 2531011;
	return (xseed >> 16) & 0x7fff;
}

#endif

// innoextract --show-password setup.exe
const char HASH_HEX[] = "53e5e63cb55ccf591b1ecc5d34c72e559f52908a";
const char SALT_HEX[] = "50617373776f7264436865636b48617368f78d0356f47737af";

uint8_t HASH[SHA_DIGEST_LENGTH];
uint8_t SALT[(sizeof(SALT_HEX) - 1) / 2];

void hex_to_bin(uint8_t out[], const char in[], size_t out_size) {
	for(int i = 0; i < out_size; i++)
		sscanf(&in[i * 2], "%2hhx", &out[i]);
}

const char CHARS[] = "abcdefghijkmpqrstuvwxyzABCDEFGHJKLMPQRSTUVWXYZ23456789";

#if 1

int main(int argc, char *argv[]) {
	hex_to_bin(HASH, HASH_HEX, SHA_DIGEST_LENGTH);
	hex_to_bin(SALT, SALT_HEX, (sizeof(SALT_HEX) - 1) / 2);
	
    SHA_CTX sha1_template;
    SHA1_Init(&sha1_template);
    SHA1_Update(&sha1_template, SALT, sizeof(SALT));
    
    SHA_CTX ctx;
    uint16_t buf[PWDLEN] = {0};
    uint8_t hash[SHA_DIGEST_LENGTH];
    for(uint32_t n = argc > 1 ? atoi(argv[1]) : 0; n < 256; n++) {
		uint64_t pos = n << 24;
		printf("Checking block %d/256 (seeds %#x-%#x)\n", n + 1, pos, pos + (1 << 24) - 1);
		
		for(uint64_t seed = pos; seed < pos + (1 << 24); seed++) {
#			ifdef OLD_ALGORITHM
			xsrand((uint32_t)seed);
#			ifdef OFF_BY_ONE
			xrand();
#			endif
			for(int i = 0; i < PWDLEN; i++)
				buf[i] = (uint16_t)CHARS[(size_t)(xrand() / (double)(1 << 15) * (sizeof(CHARS)  - 1))];
#			else
			srand48((uint32_t)seed);
#			ifdef OFF_BY_ONE
			drand48();
#			endif
			for(int i = 0; i < PWDLEN; i++)
				buf[i] = (uint16_t)CHARS[(size_t)(drand48() * (sizeof(CHARS) - 1))];
#			endif
			
			memcpy(&ctx, &sha1_template, sizeof(SHA_CTX));
			SHA1_Update(&ctx, buf, sizeof(buf));
			SHA1_Final(hash, &ctx);
			if(!memcmp(hash, HASH, SHA_DIGEST_LENGTH)) {
				printf("FOUND: seed = %#x, passwd = \"", seed);
				for(int i = 0; i < PWDLEN; i++)
					putchar(buf[i]);
				puts("\"");
				return 0;
			}
		}
    }
    puts("Nothing found. Whoops.");
    return 0;
}

#else

int main() {
	hex_to_bin(HASH, HASH_HEX, SHA_DIGEST_LENGTH);
	hex_to_bin(SALT, SALT_HEX, (sizeof(SALT_HEX) - 1) / 2);
	
    SHA_CTX sha1_template;
    SHA1_Init(&sha1_template);
    SHA1_Update(&sha1_template, SALT, sizeof(SALT));
    
    uint16_t buf[PWDLEN] = {0};
	srand48(0x3885ED18);
	drand48();
	for(int i = 0; i < PWDLEN; i++) {
		buf[i] = (uint16_t)CHARS[(size_t)(drand48() * (sizeof(CHARS) - 1))];
		putchar((char)buf[i]);
	}
	putchar('\n');
	
	SHA_CTX ctx;
    uint8_t hash[SHA_DIGEST_LENGTH];
	memcpy(&ctx, &sha1_template, sizeof(SHA_CTX));
	SHA1_Update(&ctx, buf, sizeof(buf));
	SHA1_Final(hash, &ctx);
	if(!memcmp(hash, HASH, SHA_DIGEST_LENGTH))
		puts("OK");
	
	return 0;
}

#endif

