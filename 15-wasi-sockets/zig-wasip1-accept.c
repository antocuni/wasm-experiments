// This is what `zig cc --target=wasm32-wasi` CAN compile: the WASI Preview 1
// socket *subset*. On wasip1, wasi-libc only exposes accept/recv/send/shutdown,
// operating on a socket fd that the *host* provides. There is no socket(),
// bind(), listen() or connect() -- those were added only on the wasip2 target.
//
// Historically wasmtime could hand the guest a preopened listening socket via
// `-S tcplisten=... -S preview2=n`, and this program would accept() on fd 3.
// That legacy path was removed in wasmtime 47+, so this file exists purely to
// document that `zig cc` builds the wasip1 subset but cannot do real sockets.
//
//   zig cc --target=wasm32-wasi -c zig-wasip1-accept.c   # compiles fine

#include <unistd.h>
#include <sys/socket.h>

#define LISTEN_FD 3

int main(void) {
    int conn = accept(LISTEN_FD, 0, 0);
    if (conn < 0) return 1;

    char buf[256];
    ssize_t n = recv(conn, buf, sizeof(buf), 0);
    if (n > 0) send(conn, buf, n, 0);

    close(conn);
    return 0;
}
