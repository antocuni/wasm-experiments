// A tiny TCP echo server using real BSD sockets, compiled to a WASI Preview 2
// component. It creates its own listening socket (socket/bind/listen/accept),
// which is only possible on the `wasm32-wasip2` target using the wasi-sockets
// proposal exposed through wasi-libc's POSIX shim.
//
// NOTE: This must be built with wasi-sdk's `wasm32-wasip2-clang`, NOT `zig cc`.
// `zig cc` bundles its own outdated wasi-libc headers (which it force-injects
// ahead of any --sysroot), and those only expose the Preview 1 socket subset
// (accept/recv/send/shutdown on a *host-provided* fd). socket()/bind()/
// listen()/connect() live only in the wasip2 sysroot. See the README.
//
// Run with:
//   wasmtime run -S inherit-network -S tcp server.wasm 127.0.0.1 8080

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 8080;

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { perror("socket"); return 1; }

    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "bad host: %s\n", host);
        return 1;
    }

    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(lfd, 1) < 0) { perror("listen"); return 1; }

    fprintf(stderr, "echo server listening on %s:%d\n", host, port);

    int cfd = accept(lfd, NULL, NULL);
    if (cfd < 0) { perror("accept"); return 1; }
    fprintf(stderr, "accepted a connection\n");

    char buf[1024];
    for (;;) {
        ssize_t n = recv(cfd, buf, sizeof(buf), 0);
        if (n < 0) { perror("recv"); break; }
        if (n == 0) { fprintf(stderr, "peer closed\n"); break; }

        ssize_t off = 0;
        while (off < n) {
            ssize_t w = send(cfd, buf + off, (size_t)(n - off), 0);
            if (w < 0) { perror("send"); goto done; }
            off += w;
        }
    }
done:
    close(cfd);
    close(lfd);
    fprintf(stderr, "server done\n");
    return 0;
}
