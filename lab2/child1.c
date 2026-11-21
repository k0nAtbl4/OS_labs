// child1 ждёт сигнала от parent (семафор sem_in),
// читает строку из общей памяти (buf_in), переводит её
// в верхний регистр и кладёт результат в buf_mid,
// после чего сигналит child2 (семафор sem_mid).
//
// argv[1] имя сегмента общей памяти
// argv[2] имя семафора sem_in
// argv[3] имя семафора sem_mid

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "help.h"

int main(int argc, char **argv)
{
    if (argc < 4) {
        print_str(2, "usage: child1 <shm> <sem_in> <sem_mid>\n");
        return 1;
    }

    // подключаемся к сегменту общей памяти, который создал parent
    int fd = shm_open(argv[1], O_RDWR, 0600);
    if (fd == -1) {
        print_err("shm_open");
        return 1;
    }
    struct Shared *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, fd, 0);
    close(fd);
    if (shm_ptr == MAP_FAILED) {
        print_err("mmap");
        return 1;
    }

    // открываем семафоры (их уже создал parent)
    sem_t *sem_in  = sem_open(argv[2], 0);
    sem_t *sem_mid = sem_open(argv[3], 0);
    if (sem_in == SEM_FAILED || sem_mid == SEM_FAILED) {
        print_err("sem_open");
        return 1;
    }

    for (;;) {
        
        while (sem_wait(sem_in) == -1) {
            if (errno == EINTR) continue;
            print_err("sem_wait");
            return 1;
        }

        if (shm_ptr->done) break;   // parent закончил работу

        size_t len = shm_ptr->len_in;
        if (len > BUF_SIZE) len = BUF_SIZE;

        // переводим строку в верхний регистр
        for (size_t i = 0; i < len; i++) {
            char c = shm_ptr->buf_in[i];
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            shm_ptr->buf_mid[i] = c;
        }

        shm_ptr->len_mid = len;

        //  в buf_mid лежит результат
        sem_post(sem_mid);
    }

    return 0;
}
