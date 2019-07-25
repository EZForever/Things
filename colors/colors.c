#include <stdio.h>

int main(void) {
    int i;
    printf("\e[37;1m");
    for(i = 0; i < 256; i++) {
        if(!(i <= 16 ? i % 4 : (i - 16) % 6))
            puts("\e[40m");
        printf("\e[48;5;%dm %3d ", i, i);
    }
    puts("\e[0m\n");
    return 0;
}