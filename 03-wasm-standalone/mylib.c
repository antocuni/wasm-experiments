#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
int myadd(int a, int b) {
    return a+b;
}
