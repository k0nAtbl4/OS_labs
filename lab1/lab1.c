#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{

    int pipe_fd
    pid_t child1 = fork();
    if (child1 == -1)
    {
        perror("child 1 die =(");
        exit(-22);
    }
    if (child1 == 0){

    }
    else{

    }
}