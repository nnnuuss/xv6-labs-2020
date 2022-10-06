#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int fd[2];
    char buf[15];
    pipe(fd);
    if (fork()==0){
        int len = read(fd[0], buf, 15);
        buf[len]=0;
        printf("%d: received %s\n",getpid(), buf);
        char message[] = "pong";
        write(fd[1], message, strlen(message));
        close(fd[1]);
    }else{
        char message[] = "ping";
        write(fd[1], message, strlen(message));
        close(fd[1]);
        wait((int*)0);
        int len = read(fd[0], buf, 15);
        buf[len]=0;
        printf("%d: received %s\n",getpid(), buf);
    }
    exit(0);
}