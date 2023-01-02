void foo(void);

int myadd(int a, int b) {
    if (a>0) {
        foo();
        return 42;
    }
    return (a+1) * (b+2);
}
