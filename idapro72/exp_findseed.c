//gcc -O2 $ME -o $ME.out
//gcc -g $ME -o $ME.out

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//#define OLD_ALGORITHM
//#define OFF_BY_ONE

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

const char CHARS[] = "abcdefghijkmpqrstuvwxyzABCDEFGHJKLMPQRSTUVWXYZ23456789";

// exp_findseed <passwd> [start_blk = 0]
int main(int argc, char *argv[]) {
    char buf[strlen(argv[1])];
    memset(buf, 0, sizeof(buf));
    
    for(uint32_t n = argc > 2 ? atoi(argv[2]) : 0; n < 256; n++) {
		uint64_t pos = n << 24;
		printf("Checking block %d/256 (seeds %#x-%#x)\n", n + 1, pos, pos + (1 << 24) - 1);
		
		for(uint64_t seed = pos; seed < pos + (1 << 24); seed++) {
#			ifdef OLD_ALGORITHM
			xsrand((uint32_t)seed);
#			ifdef OFF_BY_ONE
			xrand();
#			endif
			for(int i = 0; i < sizeof(buf); i++)
				buf[i] = CHARS[(size_t)(xrand() / (double)(1 << 15) * (sizeof(CHARS)  - 1))];
#			else
			srand48((uint32_t)seed);
#			ifdef OFF_BY_ONE
			drand48();
#			endif
			for(int i = 0; i < sizeof(buf); i++)
				buf[i] = CHARS[(size_t)(drand48() * (sizeof(CHARS) - 1))];
#			endif
			
			if(!memcmp(buf, argv[1], sizeof(buf))) {
				printf("FOUND: seed = %#x, passwd = \"", seed);
				for(int i = 0; i < sizeof(buf); i++)
					putchar(buf[i]);
				puts("\"");
				return 0;
			}
		}
    }
    puts("Nothing found. Whoops.");
    return 0;
}

