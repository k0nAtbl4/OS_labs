#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


int main(void)
{
    int pipe1[2];   // parent -> child1 (текст)
    int pipe2[2];   // child2  -> parent (результат)
    int pipe3[2];   // child1  -> child2 (обработанный текст)

    if (pipe(pipe1) == -1) { perror("pipe1"); exit(EXIT_FAILURE); }
    if (pipe(pipe2) == -1) { perror("pipe2"); exit(EXIT_FAILURE); }
    if (pipe(pipe3) == -1) { perror("pipe3"); exit(EXIT_FAILURE); }

    pid_t child1 = fork();
    if (child1 == -1) { perror("child 1 die =("); exit(EXIT_FAILURE); }

    if (child1 == 0)
    {
        /* child1: читает pipe1, пишет в pipe3 */
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(pipe3[0]);

        /* stdin <- pipe1[0], stdout -> pipe3[1] */
        if (dup2(pipe1[0], 0) == -1) { perror("dup2"); exit(EXIT_FAILURE); }
        if (dup2(pipe3[1], 1) == -1) { perror("dup2"); exit(EXIT_FAILURE); }

        execv("./child1", (char *[]){ "./child1", NULL });
        perror("execv");   // если execv не удался
        exit(EXIT_FAILURE);
    }

    pid_t child2 = fork();
    if (child2 == -1) { perror("child 2 die =("); exit(EXIT_FAILURE); }

    if (child2 == 0)
    {
        /* child2: читает pipe3, пишет в pipe2 */
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe3[1]);
        close(pipe2[0]);

        /* stdin <- pipe3[0], stdout -> pipe2[1] */
        if (dup2(pipe3[0], 0) == -1) { perror("dup2"); exit(EXIT_FAILURE); }
        if (dup2(pipe2[1], 1) == -1) { perror("dup2"); exit(EXIT_FAILURE); }

        execv("./child2", (char *[]){ "./child2", NULL });
        perror("execv");   // если execv не удался
        exit(EXIT_FAILURE);
    }

    /* родитель: пишет в pipe1, читает из pipe2 */
    close(pipe1[0]);
    close(pipe3[0]);
    close(pipe3[1]);
    close(pipe2[1]);

    printf("введите строку\n> ");
    fflush(stdout);

    char line[256];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);

        size_t written = 0;
        while (written < len) {
            ssize_t w = write(pipe1[1], line + written, len - written);
            if (w == -1) { perror("write"); break; }
            written += (size_t)w;
        }
        printf("> ");
        fflush(stdout);
    }
    if (ferror(stdin))
        perror("fgets");

    close(pipe1[1]);   

    // читаем результат от child2 по pipe2
    printf("Результат:\n");
    fflush(stdout);

    char buf[256];
    ssize_t n;
    while ((n = read(pipe2[0], buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, (size_t)n, stdout);
        fflush(stdout);
    }
    if (n == -1)
        perror("read");

    close(pipe2[0]);
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}
