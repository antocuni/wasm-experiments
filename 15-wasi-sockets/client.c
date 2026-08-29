// A tiny TCP client using real BSD sockets (socket/connect/send/recv),
// compiled to a WASI Preview 2 component. Connects to the echo server,
// sends a message, and prints what comes back.
//
// Build with wasi-sdk's `wasm32-wasip2-clang` (see README / Makefile).
//
// Run with:
//   wasmtime run -S inherit-network -S tcp client.wasm 127.0.0.1 8080 "hello sockets"

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
    const char *msg = argc > 3 ? argv[3] : "hello from a WASI socket";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "bad host: %s\n", host);
        return 1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }
    fprintf(stderr, "connected to %s:%d\n", host, port);

    size_t len = strlen(msg);
    if (send(fd, msg, len, 0) < 0) { perror("send"); return 1; }
    fprintf(stderr, "sent: %s\n", msg);

    char buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) { perror("recv"); return 1; }
    buf[n] = '\0';
    printf("echo reply: %s\n", buf);

    close(fd);
    return 0;
}
