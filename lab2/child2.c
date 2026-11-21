// child2 ждёт сигнала от child1 (семафор sem_mid),
// читает строку из общей памяти (buf_mid), убирает повторные
// пробелы и кладёт результат в buf_out,
// после чего сигналит parent (семафор sem_out).
//
// argv[1] имя сегмента общей памяти
// argv[2] имя семафора sem_mid
// argv[3] имя семафора sem_out

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
        print_str(2, "usage: child2 <shm> <sem_mid> <sem_out>\n");
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
    sem_t *sem_mid = sem_open(argv[2], 0);
    sem_t *sem_out = sem_open(argv[3], 0);
    if (sem_mid == SEM_FAILED || sem_out == SEM_FAILED) {
        print_err("sem_open");
        return 1;
    }

    int prev_space = 0;
    for (;;) {
        // child1 положил строку в buf_mid
        while (sem_wait(sem_mid) == -1) {
            if (errno == EINTR) continue;
            print_err("sem_wait");
            return 1;
        }

        if (shm_ptr->done) break;   // parent закончил работу

        size_t len = shm_ptr->len_mid;
        if (len > BUF_SIZE) len = BUF_SIZE;

        // убираем повторные пробелы
        size_t k = 0;
        for (size_t i = 0; i < len; i++) {
            char c = shm_ptr->buf_mid[i];
            if (c == ' ') {
                if (prev_space) continue;
                prev_space = 1;
            } else {
                prev_space = 0;
            }
            shm_ptr->buf_out[k++] = c;
        }
        shm_ptr->len_out = k;

        //  buf_out лежит результат
        sem_post(sem_out);
    }

    return 0;
}
