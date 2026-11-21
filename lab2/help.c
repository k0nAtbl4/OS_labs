#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "help.h"

// печать строки в дескриптор (вместо printf)
void print_str(int fd, const char *s)
{
    size_t len = strlen(s);
    size_t written = 0;
    while (written < len) {
        ssize_t w = write(fd, s + written, len - written);
        if (w <= 0) break;
        written += (size_t)w;
    }
}

// печать ошибки в stderr (вместо perror)
void print_err(const char *what)
{
    print_str(2, what);
    print_str(2, ": ");
    print_str(2, strerror(errno));
    print_str(2, "\n");
}

// чтение строки из fd до '\n' или EOF (вместо fgets).
// Возвращает длину строки, -1 — EOF, -2 — ошибка.
int read_line(int fd, char *buf, size_t cap)
{
    size_t n = 0;
    char c;

    while (n + 1 < cap) {
        ssize_t r = read(fd, &c, 1);
        if (r == 0) {
            if (n == 0) return -1;
            break;
        }
        if (r < 0) {
            if (errno == EINTR) continue;
            return -2;
        }
        buf[n++] = c;
        if (c == '\n') break;
    }
    buf[n] = '\0';
    return (int)n;
}
