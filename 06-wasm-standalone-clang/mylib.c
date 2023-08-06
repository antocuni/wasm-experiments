void some_external_function(int* p);

int foo(int a, int b) {
    if (a > 0)
        some_external_function((int*)a);
    else
        some_external_function((int*)b);

    some_external_function((int*)0);
    /* int c = a+b; */
    /* some_external_function(&c); */
    /* return c; */
}
