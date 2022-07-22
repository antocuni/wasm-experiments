#include <stdio.h>
#include <emscripten.h>

const char *myname = "antonio";

EMSCRIPTEN_KEEPALIVE
void say_hello(void) {
    printf("hello from C, %s\n", myname);
}

EMSCRIPTEN_KEEPALIVE
double mypow(double base, int exp) {
    double res = 1;
    for(int i=0; i<exp; i++)
        res *= base;
    return res;
}

EMSCRIPTEN_KEEPALIVE
void print_str(char *s) {
    printf("printing string: %s\n", s);
}


EMSCRIPTEN_KEEPALIVE
const char* get_myname(void) {
    return myname;
}
