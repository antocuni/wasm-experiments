#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
int myadd(int a, int b) {
    return a+b;
}

EMSCRIPTEN_KEEPALIVE
int foo(int a, int b) {
    return myadd(a*2, b*3);
}
