typedef __externref_t Point;

#define IMPORT(name) \
    __attribute__((import_module("point"), import_name(#name)))

IMPORT(alloc_Point) Point alloc_Point(int x, int y);
IMPORT(get_x)       int   get_x(Point p);
IMPORT(get_y)       int   get_y(Point p);
IMPORT(set_x)       void  set_x(Point p, int v);
IMPORT(set_y)       void  set_y(Point p, int v);

__attribute__((import_module("host"), import_name("print_i32")))
void print_i32(int v);

__attribute__((export_name("run")))
int run(void) {
    Point p = alloc_Point(3, 4);
    print_i32(get_x(p));
    print_i32(get_y(p));

    set_x(p, 10);
    set_y(p, 20);
    print_i32(get_x(p));
    print_i32(get_y(p));

    return get_x(p) + get_y(p);
}
