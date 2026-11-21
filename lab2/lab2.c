
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>



#include <sys/fcntl.h>
#include <wait.h>
#include <semaphore.h>
#include <sys/mman.h>

#include "help.h"

int main(void)
{
    

    // создание общей памяти: shm_open -> ftruncate -> mmap

    // имя сегмента не константное: уникально для каждого запуска
    char shm_name[64] = "/lab3_shm_";
    long pid = getpid();
    char digits[24];
    int nd = 0;
    do {
        digits[nd++] = (char)('0' + pid % 10);
        pid /= 10;
    } while (pid > 0);
    int si = (int)strlen(shm_name);
    int nd_all = nd;
    while (nd > 0)
        shm_name[si++] = digits[--nd];
    shm_name[si] = '\0';

    // имя семафора — тоже не константное
    char sem_in_name[64] = "/lab3_sem_in_";
    int sn = (int)strlen(sem_in_name);
    for (int d = nd_all - 1; d >= 0; d--)
        sem_in_name[sn++] = digits[d];
    sem_in_name[sn] = '\0';

    char sem_mid_name[64] = "/lab3_sem_mid_";
    int sm = (int)strlen(sem_mid_name);
    for (int d = nd_all - 1; d >= 0; d--)
        sem_mid_name[sm++] = digits[d];
    sem_mid_name[sm] = '\0';

    char sem_out_name[64] = "/lab3_sem_out_";
    int so = (int)strlen(sem_out_name);
    for (int d = nd_all - 1; d >= 0; d--)
        sem_out_name[so++] = digits[d];
    sem_out_name[so] = '\0';

    //создаём именованный сегмент общей памяти (файл в /dev/shm)  ((держиться в ram))
    int shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd == -1) {
        print_err("shm_open");
        exit(EXIT_FAILURE);
    }

    // задаём размер сегмента
    if (ftruncate(shm_fd, (off_t)SHM_SIZE) == -1) {
        print_err("ftruncate");
        close(shm_fd);
        shm_unlink(shm_name);
        exit(EXIT_FAILURE);
    }

    // отображаем сегмент в адресное пространство процесса
    struct Shared *shm_ptr = mmap(NULL, (size_t)SHM_SIZE, PROT_READ | PROT_WRITE,
                                  MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        print_err("mmap");
        close(shm_fd);
        shm_unlink(shm_name);
        exit(EXIT_FAILURE);
    }
    close(shm_fd); 

    memset(shm_ptr, 0, (size_t)SHM_SIZE);   // очищаем общую память

    // сигнал child1, что в buf_in лежит свежая строка
    sem_t *sem_in = sem_open(sem_in_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_in == SEM_FAILED) {
        print_err("sem_open");
        exit(EXIT_FAILURE);
    }

    //  сигнал child2, что в buf_mid лежит результат
    sem_t *sem_mid = sem_open(sem_mid_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_mid == SEM_FAILED) {
        print_err("sem_open");
        exit(EXIT_FAILURE);
    }

    //  сигнал parent, что в buf_out лежит результат
    sem_t *sem_out = sem_open(sem_out_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_out == SEM_FAILED) {
        print_err("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t child1 = fork();
    if (child1 == -1) { print_err("child 1 die =("); exit(EXIT_FAILURE); }

    if (child1 == 0)
    {
    
        // передаём child1 имена shm и семафоров
        char *args[] = { (char *)"./child1", shm_name, sem_in_name, sem_mid_name, NULL };
        execv("./child1", args);
        print_err("execv");   // если execv не удался
        exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();
    if (child2 == -1) { print_err("child 2 die =("); exit(EXIT_FAILURE); }

    if (child2 == 0)
    {
        

        // передаём child2 имена shm и семафоров
        char *args[] = { (char *)"./child2", shm_name, sem_mid_name, sem_out_name, NULL };
        execv("./child2", args);
        print_err("execv");   // если execv не удался
        exit(EXIT_FAILURE);
    }



    print_str(1, "введите строку\n> ");

    char line[256];
    int r;
    while ((r = read_line(0, line, sizeof(line))) != -1) {
        if (r == -2) {
            print_err("read");
            break;
        }
        size_t len = strlen(line);

        // кладём строку в общую память
        memcpy(shm_ptr->buf_in, line, len);
        shm_ptr->len_in = len;

        // сообщаем child1, что строка готова
        sem_post(sem_in);

        // ждём результат от child2
        while (sem_wait(sem_out) == -1) {
            if (errno == EINTR) continue;
            print_err("sem_wait");
            break;
        }

        // печатаем результат
        size_t out_len = shm_ptr->len_out;
        if (out_len > BUF_SIZE) out_len = BUF_SIZE;
        size_t written = 0;
        while (written < out_len) {
            ssize_t w = write(1, shm_ptr->buf_out + written, out_len - written);
            if (w == -1) { print_err("write"); break; }
            written += (size_t)w;
        }

        print_str(1, "> ");
    }


    // говорим детям чайлдамчто данных больше не будет
    shm_ptr->done = 1;
    sem_post(sem_in);
    sem_post(sem_mid);

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    // освобождаем общую память
    munmap(shm_ptr, (size_t)SHM_SIZE);
    shm_unlink(shm_name);
    sem_close(sem_in);
    sem_unlink(sem_in_name);
    sem_close(sem_mid);
    sem_unlink(sem_mid_name);
    sem_close(sem_out);
    sem_unlink(sem_out_name);

    print_str(1, "\n");
    return 0;
}
