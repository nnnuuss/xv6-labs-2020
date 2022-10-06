#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int fd[2];
    pipe(fd);
    if (fork()==0){
        int prime, tem;
        while(1){
            if(read(fd[0], &prime, sizeof(int))!=0){
                close(fd[1]);
                printf("prime %d\n", prime);
                int p[2];
                pipe(p);
                if (fork()!=0){
                    while(read(fd[0], &tem, sizeof(int))!=0){
                        if (tem%prime!=0)write(p[1], &tem, sizeof(int));
                    }
                    close(fd[0]);
                    close(p[1]);
                    wait((int*)0);
                    exit(0);
                }else{
                    close(fd[0]);
                    fd[0]=p[0];
                    fd[1]=p[1];
                    close(p[1]);                    
                }
            }
            else {
                close(fd[0]);break;
            }
        }
    }else{
        close(fd[0]);
        for (int i = 2; i <= 35; ++i)
            write(fd[1], &i, sizeof(int));
        close(fd[1]);
        wait((int*)0);
    }
    exit(0);
}