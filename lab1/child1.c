#include <ctype.h>
#include <stdio.h>
#include <unistd.h>


int main(void)
{
    char buf[256];
    ssize_t n;

    while ((n = read(0, buf, sizeof(buf))) > 0) {

        for (size_t i = 0; i < (size_t)n; i++) {
            buf[i] = (char)toupper((unsigned char)buf[i]);
        }
        if (write(1, buf, (size_t)n) != n) // если записалось не все - еррор
            perror("write");
    }
    if (n == -1)
        perror("read");

    return 0;
}
