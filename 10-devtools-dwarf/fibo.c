void abort(void) {
    __builtin_trap();
}

int fibo(int n) {
    int a = 1;
    int b = 1;
    int i = 0;
    while(n--) {
        int tmp = b;
        b = a + b;
        a = tmp;
    }
    return a;
}
