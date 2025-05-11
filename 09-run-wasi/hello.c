#include <stdio.h>
#include <stdlib.h>

int main(void) {
    //printf("%s", "hello from WASI\n");
    const char *msg = malloc(100);
    puts("hello from WASI\n");
}
