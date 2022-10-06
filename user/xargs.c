#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define BUF_SIZE 512
char buf[BUF_SIZE], argbuf[40];
char *child_argv[MAXARG];
int
main(int argc, char *argv[]){
    for (int i = 1; i < argc; ++i){
        child_argv[i-1]=(char*)malloc(sizeof(char) * strlen(argv[i]));
        strcpy(child_argv[i-1], argv[i]);
    }
    while(read(0, buf, BUF_SIZE)!=0);
    int len = strlen(buf);
    int now = argc-1, tem = 0;
    for (int i = 0; i < len; ++i){
        if (buf[i]==' '||buf[i]=='\n'){
            argbuf[tem]=0;
            child_argv[now]=(char*)malloc(sizeof(char)*strlen(argbuf));
            strcpy(child_argv[now], argbuf);
            tem=0,now++;
        }else{
            argbuf[tem++]=buf[i];
        }
    }
    // for (int i = 0; i < now; ++i)printf("%s ", child_argv[i]);
    // printf("\n");
    if (fork()==0){
        exec(argv[1], child_argv);
    }else{
        wait((int*)0);
    }

    exit(0);
}