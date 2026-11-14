#include <stdio.h>
#include <unistd.h>


int main(void)
{
    char buf[256];
    char out[256];  //  тк разный размер, записываю в другой буфер
    ssize_t n;  // ssize_t - может быть отриц. (то есть ошибка.) size_t - просто размер
    int prev_space = 0;

    while ((n = read(0, buf, sizeof(buf))) > 0) {
        size_t k = 0;
        for (ssize_t i = 0; i < n; i++) {
            char c = buf[i];
            if (c == ' ') {
                if (prev_space)
                    continue;
                prev_space = 1;
            } else {
                prev_space = 0;
            }
            out[k++] = c;
        }
        if (k > 0 && write(1, out, k) != (ssize_t)k)
            perror("write");
    }
    if (n == -1)
        perror("read");

    return 0;
}
