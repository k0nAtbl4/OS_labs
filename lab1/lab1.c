#include <ctype.h>
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

    // pipe[1] - запись, pipe[0] - чтение

    if (pipe(pipe1) == -1) { perror("pipe1"); exit(EXIT_FAILURE); }
    if (pipe(pipe2) == -1) { perror("pipe2"); exit(EXIT_FAILURE); }
    if (pipe(pipe3) == -1) { perror("pipe3"); exit(EXIT_FAILURE); }

    
    pid_t child1 = fork();
    if (child1 == -1) { perror("child 1 die =("); exit(EXIT_FAILURE); }

    if (child1 == 0)
    {
        
        close(pipe1[1]);                   
        close(pipe2[0]);                   
        close(pipe2[1]);                   
        close(pipe3[0]);                   

        char buf[256];
        ssize_t n;
        while ((n = read(pipe1[0], buf, sizeof(buf))) > 0) {
            
            for (size_t i = 0; i < (size_t)n; i++) {
                buf[i] = (char)toupper((unsigned char)buf[i]);
            }
            if (write(pipe3[1], buf, (size_t)n) != n) // если записалось не все - еррор
                perror("write");
        }
        if (n == -1)
            perror("read");

        close(pipe1[0]);
        close(pipe3[1]);                   // EOF для child2
        exit(0);
    }

    
    pid_t child2 = fork();
    if (child2 == -1) { perror("child 2 die =("); exit(EXIT_FAILURE); }

    if (child2 == 0)
    {
        
        close(pipe1[0]);                   // не нужны
        close(pipe1[1]);
        close(pipe3[1]);                   // писать в pipe3 не нужно
        

        char buf[256];
        char out[256];  //  тк разный размер, записываю в другой буфер
        ssize_t n;  // ssize_t - может быть отриц. (то есть ошибка.) size_t - просто размер
        int prev_space = 0;                
        while ((n = read(pipe3[0], buf, sizeof(buf))) > 0) {
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
            if (k > 0 && write(pipe2[1], out, k) != (ssize_t)k)
                perror("write");
        }
        if (n == -1)
            perror("read");

        close(pipe3[0]);
        close(pipe2[1]);                   
        exit(0);
    }

    
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
